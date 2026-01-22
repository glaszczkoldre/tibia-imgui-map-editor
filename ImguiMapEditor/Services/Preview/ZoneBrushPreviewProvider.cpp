#include "ZoneBrushPreviewProvider.h"
#include "Services/BrushSettingsService.h"

namespace MapEditor::Services::Preview {

ZoneBrushPreviewProvider::ZoneBrushPreviewProvider(
    uint32_t color, BrushSettingsService *brushSettings)
    : color_(color), brushSettings_(brushSettings) {
  buildPreview();
}

bool ZoneBrushPreviewProvider::isActive() const { return color_ != 0; }

Domain::Position ZoneBrushPreviewProvider::getAnchorPosition() const {
  return anchor_;
}

bool ZoneBrushPreviewProvider::checkSettingsChanged() const {
  if (!brushSettings_) {
    return false;
  }
  auto offsets = brushSettings_->getBrushOffsets();
  return offsets != cachedOffsets_;
}

const std::vector<PreviewTileData> &ZoneBrushPreviewProvider::getTiles() const {
  if (checkSettingsChanged()) {
    needsRegen_ = true;
  }

  if (needsRegen_) {
    const_cast<ZoneBrushPreviewProvider *>(this)->buildPreview();
  }
  return tiles_;
}

PreviewBounds ZoneBrushPreviewProvider::getBounds() const { return bounds_; }

void ZoneBrushPreviewProvider::updateCursorPosition(
    const Domain::Position &cursor) {
  anchor_ = cursor;
}

void ZoneBrushPreviewProvider::regenerate() { buildPreview(); }

void ZoneBrushPreviewProvider::buildPreview() {
  tiles_.clear();
  bounds_ = PreviewBounds();
  needsRegen_ = false;

  if (color_ == 0) {
    cachedOffsets_.clear();
    return;
  }

  // Get brush offsets from settings service, or use single tile
  std::vector<std::pair<int, int>> offsets;

  if (brushSettings_) {
    offsets = brushSettings_->getBrushOffsets();
  } else {
    offsets.emplace_back(0, 0);
  }

  cachedOffsets_ = offsets;

  // Create a preview tile for each offset with zone color
  for (const auto &[dx, dy] : offsets) {
    PreviewTileData tile(dx, dy, 0);
    tile.zone_color = color_;
    tiles_.push_back(std::move(tile));

    bounds_.expand(dx, dy, 0);
  }
}

} // namespace MapEditor::Services::Preview
