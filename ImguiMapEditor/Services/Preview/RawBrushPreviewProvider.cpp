#include "RawBrushPreviewProvider.h"
#include "Services/BrushSettingsService.h"

namespace MapEditor {
namespace Services {
namespace Preview {

RawBrushPreviewProvider::RawBrushPreviewProvider(
    uint32_t itemId, uint16_t subtype, BrushSettingsService *brushSettings)
    : itemId_(itemId), subtype_(subtype), brushSettings_(brushSettings) {
  buildPreview();
}

bool RawBrushPreviewProvider::isActive() const { return itemId_ > 0; }

Domain::Position RawBrushPreviewProvider::getAnchorPosition() const {
  return anchor_;
}

bool RawBrushPreviewProvider::checkSettingsChanged() const {
  if (!brushSettings_) {
    return false;
  }
  // Compare actual offsets, not just count (fixes 2x1 vs 1x2 issue)
  auto offsets = brushSettings_->getBrushOffsets();
  return offsets != cachedOffsets_;
}

const std::vector<PreviewTileData> &RawBrushPreviewProvider::getTiles() const {
  // Check if brush settings changed since last build
  if (checkSettingsChanged()) {
    needsRegen_ = true;
  }

  // Lazy regeneration if needed
  if (needsRegen_) {
    const_cast<RawBrushPreviewProvider *>(this)->buildPreview();
  }
  return tiles_;
}

PreviewBounds RawBrushPreviewProvider::getBounds() const { return bounds_; }

void RawBrushPreviewProvider::updateCursorPosition(
    const Domain::Position &cursor) {
  anchor_ = cursor;
}

void RawBrushPreviewProvider::regenerate() { buildPreview(); }

void RawBrushPreviewProvider::buildPreview() {
  tiles_.clear();
  bounds_ = PreviewBounds();
  needsRegen_ = false;

  if (itemId_ == 0) {
    cachedOffsets_.clear();
    return;
  }

  // Get brush offsets from settings service, or use single tile
  std::vector<std::pair<int, int>> offsets;

  if (brushSettings_) {
    offsets = brushSettings_->getBrushOffsets();
  } else {
    // No brush settings, default to single tile
    offsets.emplace_back(0, 0);
  }

  // Cache actual offsets for change detection
  cachedOffsets_ = offsets;

  // Create a preview tile for each offset
  for (const auto &[dx, dy] : offsets) {
    PreviewTileData tile(dx, dy, 0); // Relative position
    tile.addItem(itemId_, subtype_);
    tiles_.push_back(std::move(tile));

    // Update bounds
    bounds_.expand(dx, dy, 0);
  }
}

} // namespace Preview
} // namespace Services
} // namespace MapEditor
