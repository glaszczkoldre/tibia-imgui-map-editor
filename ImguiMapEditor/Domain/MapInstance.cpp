#include "MapInstance.h"
#include "../Services/ClientDataService.h"
#include "Domain/Selection/SelectionEntry.h"
#include "Tile.h"
#include <algorithm>
#include <unordered_set>

namespace MapEditor::Domain {

using namespace Selection;

MapInstance::MapInstance(std::unique_ptr<ChunkedMap> map,
                         Services::ClientDataService *client_data)
    : map_(std::move(map)), client_data_(client_data) {}

MapInstance::~MapInstance() = default;

void MapInstance::setFilePath(const std::filesystem::path &path) {
  file_path_ = path;
}

std::string MapInstance::getDisplayName() const {
  if (file_path_.empty()) {
    return "Untitled";
  }

  std::string name = file_path_.stem().string();
  if (modified_) {
    name += "*";
  }
  return name;
}

void MapInstance::setModified(bool modified) {
  if (modified_ != modified) {
    modified_ = modified;
    if (on_modified_callback_) {
      on_modified_callback_(modified);
    }
  } else if (modified) {
    // Still fire callback if setting true explicitly (e.g. for forced UI
    // update)
    if (on_modified_callback_) {
      on_modified_callback_(modified);
    }
  }
}

void MapInstance::selectRegion(int32_t min_x, int32_t min_y, int32_t max_x,
                               int32_t max_y, int16_t z) {
  if (!map_)
    return;

  // Use SelectionService's selectRegion with all entity types
  selection_service_.selectRegion(map_.get(), min_x, min_y, max_x, max_y, z,
                                  SelectionFilter::all());
}

void MapInstance::clearSelection() { selection_service_.clear(); }

void MapInstance::deleteSelection() {
  if (!map_ || selection_service_.isEmpty())
    return;

  // Get all selection entries to delete specific items
  auto entries = selection_service_.getAllEntries();
  if (entries.empty())
    return;

  // Collect unique positions for undo recording
  struct PositionHash {
    size_t operator()(const Position &p) const {
      return std::hash<uint64_t>{}(p.pack());
    }
  };
  std::unordered_set<Position, PositionHash> affected_positions;
  for (const auto &entry : entries) {
    affected_positions.insert(entry.getPosition());
  }

  // Record BEFORE states for undo (including selection state)
  history_manager_.beginOperation("Delete selection",
                                  History::ActionType::Delete,
                                  &selection_service_);

  // FIX: Clear selection immediately to prevent Use-After-Free.
  // We must deselect items while they are still alive (so visual state can update safely).
  // If we waited until after deletion, syncSelectionState would access freed memory.
  selection_service_.clear();

  for (const auto &pos : affected_positions) {
    history_manager_.recordTileBefore(pos, map_->getTile(pos));
  }

  // Delete based on entry type
  for (const auto &entry : entries) {
    const Position &pos = entry.getPosition();
    Tile *tile = map_->getTile(pos);
    if (!tile)
      continue;

    switch (entry.getType()) {
    case EntityType::Ground:
      // Delete ground
      if (tile->hasGround()) {
        tile->removeGround();
      }
      break;

    case EntityType::Item: {
      // Delete specific item
      const Item *item_ptr = static_cast<const Item *>(entry.entity_ptr);
      if (item_ptr) {
        // Check if it's the ground
        if (tile->getGround() == item_ptr) {
          tile->removeGround();
        } else {
          // Find and remove from items list
          const auto &items = tile->getItems();
          if (auto it = std::ranges::find_if(items,
                                             [item_ptr](const auto &ptr) {
                                               return ptr.get() == item_ptr;
                                             });
              it != items.end()) {
            tile->removeItem(
                static_cast<size_t>(std::distance(items.begin(), it)));
          }
        }
      }
      break;
    }

    case EntityType::Creature:
      // Delete creature
      if (tile->hasCreature()) {
        tile->removeCreature();
      }
      break;

    case EntityType::Spawn:
      // Delete spawn
      if (tile->hasSpawn()) {
        tile->removeSpawn();
      }
      break;
    }
  }

  // End operation (captures AFTER states including selection)
  history_manager_.endOperation(map_.get(), &selection_service_);

  setModified(true);
}

std::string MapInstance::undo() {
  std::string desc = history_manager_.undo(map_.get(), client_data_, &selection_service_);
  if (!desc.empty()) {
    setModified(true);
  }
  return desc;
}

std::string MapInstance::redo() {
  std::string desc = history_manager_.redo(map_.get(), client_data_, &selection_service_);
  if (!desc.empty()) {
    setModified(true);
  }
  return desc;
}

} // namespace MapEditor::Domain
