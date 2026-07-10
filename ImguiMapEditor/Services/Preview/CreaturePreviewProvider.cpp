#include "CreaturePreviewProvider.h"
#include "Services/BrushSettingsService.h"
#include <spdlog/spdlog.h>

namespace MapEditor::Services::Preview {

CreaturePreviewProvider::CreaturePreviewProvider(
    const std::string &creatureName, const BrushSettingsService *brushSettings,
    uint8_t direction)
    : creatureName_(creatureName), brushSettings_(brushSettings), direction_(direction) {
  buildPreview();
  spdlog::debug("[CreaturePreviewProvider] Created for creature: {}",
                creatureName);
}

bool CreaturePreviewProvider::isActive() const {
  return !creatureName_.empty();
}

Domain::Position CreaturePreviewProvider::getAnchorPosition() const {
  return anchor_;
}

const std::vector<PreviewTileData> &CreaturePreviewProvider::getTiles() const {
  // Check for changes in brush settings before returning
  if (checkSettingsChanged()) {
    buildPreview();
    needsRegen_ = false;
  }
  return tiles_;
}

PreviewBounds CreaturePreviewProvider::getBounds() const { return bounds_; }

void CreaturePreviewProvider::updateCursorPosition(
    const Domain::Position &cursor) {
  anchor_ = cursor;
}

void CreaturePreviewProvider::regenerate() {
  buildPreview();
  needsRegen_ = false;
}

void CreaturePreviewProvider::buildPreview() const {
  tiles_.clear();
  bounds_ = PreviewBounds{};

  if (creatureName_.empty()) {
    return;
  }

  // Creature brush is always 1x1
  std::vector<std::pair<int, int>> offsets = {{0, 0}};

  // Cache offsets for change detection
  cachedOffsets_ = offsets;

  // Build preview tiles
  for (const auto &[dx, dy] : offsets) {
    PreviewTileData tile(dx, dy, 0);
    tile.creature_name = creatureName_;
    tile.creature_direction = direction_;
    tiles_.push_back(std::move(tile));
    bounds_.expand(dx, dy, 0);
  }

  spdlog::debug("[CreaturePreviewProvider] Built preview with {} tiles",
                tiles_.size());
}

bool CreaturePreviewProvider::checkSettingsChanged() const {
  return false;
}

} // namespace MapEditor::Services::Preview
