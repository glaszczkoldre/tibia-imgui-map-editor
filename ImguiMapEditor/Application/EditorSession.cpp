#include "EditorSession.h"
#include "../Application.h" // For accessing RenderingManager singleton/service if needed, simplified here
#include "Rendering/Frame/RenderingManager.h"
#include "Services/ClientDataService.h"
#include "Services/Preview/PastePreviewProvider.h"

namespace MapEditor::AppLogic {

// External dependency injection for RenderingManager would be better,
// but for now we assume the caller handles explicit RenderState
// creation/destruction or we access it via a service. Given the plan says
// "Update Consumers", the creation logic in MapTabManager will likely handle
// registering the session with RenderingManager. However, EditorSession needs
// to know its ID.

EditorSession::EditorSession(std::unique_ptr<Domain::MapInstance> document,
                             SessionID session_id)
    : document_(std::move(document)), session_id_(session_id) {
  // Initialize selection adapter with reference to SelectionService
  selection_adapter_.setService(&document_->getSelectionService());
}

EditorSession::~EditorSession() = default;

// NOTE: Most methods are now inlined delegates in the header.
// Only complex logic remains here.

void EditorSession::startPaste(
    const std::vector<Domain::CopyBuffer::CopiedTile> &tiles, bool replace_mode) {
  paste_preview_.clear();
  paste_preview_.reserve(tiles.size());

  // Create clones for the preview
  for (const auto &ct : tiles) {
    if (ct.tile) {
      paste_preview_.emplace_back(ct.relative_pos, ct.tile->clone());
    }
  }

  is_pasting_ = !paste_preview_.empty();
  paste_replace_mode_ = replace_mode;  // Store the mode for later confirmation

  // Create preview provider for the paste operation
  if (is_pasting_) {
    auto provider = std::make_unique<Services::Preview::PastePreviewProvider>(
        paste_preview_);
    preview_service_.setProvider(std::move(provider));
  }
}

void EditorSession::cancelPaste() {
  is_pasting_ = false;
  paste_replace_mode_ = false;
  paste_preview_.clear();
  preview_service_.clearPreview();
}

void EditorSession::confirmPaste(const Domain::Position &target_pos,
                                 bool replace_mode) {
  if (!is_pasting_ || paste_preview_.empty() || !document_)
    return;

  auto *map = document_->getMap();
  if (!map)
    return;

  auto &history_manager = document_->getHistoryManager();
  auto &selection_service = document_->getSelectionService();

  // Start history operation for paste (with selection state)
  std::string op_name = replace_mode ? "Paste (Replace)" : "Paste tiles";
  history_manager.beginOperation(op_name, Domain::History::ActionType::Other, &selection_service);

  // Apply paste directly, recording tile states for undo/redo
  for (const auto &ct : paste_preview_) {
    // Calculate world position
    Domain::Position world_pos{
        target_pos.x + ct.relative_pos.x, target_pos.y + ct.relative_pos.y,
        static_cast<int16_t>(target_pos.z + ct.relative_pos.z)};

    // Validate position
    if (world_pos.x < 0 || world_pos.y < 0 || world_pos.z < 0 ||
        world_pos.z > 15) {
      continue;
    }

    // Record BEFORE state
    history_manager.recordTileBefore(world_pos, map->getTile(world_pos));

    // Get or create target tile
    Domain::Tile *target_tile = map->getTile(world_pos);
    if (!target_tile) {
      auto new_tile = std::make_unique<Domain::Tile>(world_pos);
      map->setTile(world_pos, std::move(new_tile));
      target_tile = map->getTile(world_pos);
    }

    if (!target_tile || !ct.tile)
      continue;

    // REPLACE MODE: Clear destination tile first
    if (replace_mode) {
      target_tile->clearItems();
      target_tile->removeGround(); // Remove existing ground
      // Also remove creature and spawn in replace mode
      target_tile->setCreature(nullptr);
      target_tile->setSpawn(nullptr);
    }

    // Merge/add ground - if source has ground, replace target ground
    if (ct.tile->hasGround()) {
      auto source_ground = ct.tile->getGround()->clone();
      target_tile->addItem(std::move(source_ground));
    }

    // Append all items from source
    for (const auto &item : ct.tile->getItems()) {
      if (item) {
        target_tile->addItem(item->clone());
      }
    }

    // Copy creature if present
    if (ct.tile->hasCreature()) {
      const auto* src_creature = ct.tile->getCreature();
      if (src_creature) {
        auto new_creature = std::make_unique<Domain::Creature>(*src_creature);
        new_creature->deselect(); // Don't inherit selection state from source
        target_tile->setCreature(std::move(new_creature));
      }
    }

    // Copy spawn if present
    if (ct.tile->hasSpawn()) {
      const auto* src_spawn = ct.tile->getSpawn();
      if (src_spawn) {
        auto new_spawn = std::make_unique<Domain::Spawn>(*src_spawn);
        new_spawn->deselect(); // Don't inherit selection state from source
        target_tile->setSpawn(std::move(new_spawn));
      }
    }
  }

  // End operation (captures AFTER states including selection)
  history_manager.endOperation(map, &selection_service);
  document_->setModified(true);

  // Exit paste mode (single-shot)
  cancelPaste();
}

} // namespace MapEditor::AppLogic
