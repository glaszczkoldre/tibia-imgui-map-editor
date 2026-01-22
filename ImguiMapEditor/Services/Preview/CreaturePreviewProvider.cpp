#include "CreaturePreviewProvider.h"
#include "Services/BrushSettingsService.h"
#include <spdlog/spdlog.h>

namespace MapEditor::Services::Preview {

CreaturePreviewProvider::CreaturePreviewProvider(
    const std::string &creatureName, BrushSettingsService *brushSettings)
    : creatureName_(creatureName), brushSettings_(brushSettings) {
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
    const_cast<CreaturePreviewProvider *>(this)->regenerate();
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

void CreaturePreviewProvider::buildPreview() {
  tiles_.clear();
  bounds_ = PreviewBounds{};

  if (creatureName_.empty()) {
    return;
  }

  // Get brush positions from settings service
  std::vector<std::pair<int, int>> offsets;
  if (brushSettings_) {
    // Use center position (0,0,0) as reference to get relative offsets
    auto positions = brushSettings_->getBrushPositions({0, 0, 0});
    for (const auto &pos : positions) {
      offsets.emplace_back(pos.x, pos.y);
    }
  } else {
    // Default: single tile at cursor
    offsets.emplace_back(0, 0);
  }

  // Cache offsets for change detection
  cachedOffsets_ = offsets;

  // Build preview tiles
  for (const auto &[dx, dy] : offsets) {
    PreviewTileData tile(dx, dy, 0);
    tile.creature_name = creatureName_;
    tiles_.push_back(std::move(tile));
    bounds_.expand(dx, dy, 0);
  }

  spdlog::debug("[CreaturePreviewProvider] Built preview with {} tiles",
                tiles_.size());
}

bool CreaturePreviewProvider::checkSettingsChanged() const {
  if (!brushSettings_) {
    return false;
  }

  // Get current offsets
  auto positions = brushSettings_->getBrushPositions({0, 0, 0});

  if (positions.size() != cachedOffsets_.size()) {
    return true;
  }

  for (size_t i = 0; i < positions.size(); ++i) {
    if (positions[i].x != cachedOffsets_[i].first ||
        positions[i].y != cachedOffsets_[i].second) {
      return true;
    }
  }

  return false;
}

} // namespace MapEditor::Services::Preview
