#include "DoodadBrushPreviewProvider.h"

#include "Brushes/Types/DoodadBrush.h"
#include "Brushes/Types/DoodadPlacementPlanner.h"
#include "Services/BrushSettingsService.h"

namespace MapEditor::Services::Preview {

DoodadBrushPreviewProvider::DoodadBrushPreviewProvider(
    const Brushes::DoodadBrush &brush, BrushSettingsService *brushSettings,
    const Domain::ChunkedMap *map)
    : brush_(brush), brushSettings_(brushSettings), map_(map) {
  buildPreview();
}

bool DoodadBrushPreviewProvider::isActive() const { return true; }

Domain::Position DoodadBrushPreviewProvider::getAnchorPosition() const {
  return anchor_;
}

bool DoodadBrushPreviewProvider::checkSettingsChanged() const {
  if (!brushSettings_) {
    return false;
  }

  const auto offsets = brushSettings_->getBrushOffsets();
  return offsets != cachedOffsets_;
}

const std::vector<PreviewTileData> &DoodadBrushPreviewProvider::getTiles() const {
  if (checkSettingsChanged()) {
    needsRegen_ = true;
  }

  if (needsRegen_) {
    buildPreview();
  }

  return tiles_;
}

PreviewBounds DoodadBrushPreviewProvider::getBounds() const { return bounds_; }

PreviewStyle DoodadBrushPreviewProvider::getStyle() const {
  return brushSettings_ && brushSettings_->getPreviewBorder()
             ? PreviewStyle::Outline
             : PreviewStyle::Ghost;
}

void DoodadBrushPreviewProvider::updateCursorPosition(const Domain::Position &cursor) {
  if (anchor_ == cursor) {
    return;
  }
  anchor_ = cursor;
  needsRegen_ = true;
}

void DoodadBrushPreviewProvider::regenerate() { buildPreview(); }

void DoodadBrushPreviewProvider::buildPreview() const {
  const auto seed = Brushes::DoodadPlacementPlanner::buildSeed(
      brush_, anchor_, brushSettings_, brush_.getVariation(), false);
  tiles_ = brush_.buildPreviewTiles(anchor_, brushSettings_, map_, seed);
  bounds_ = PreviewBounds();
  needsRegen_ = false;
  cachedOffsets_ = brushSettings_ ? brushSettings_->getBrushOffsets()
                                  : std::vector<std::pair<int, int>>{{0, 0}};

  for (const auto &tile : tiles_) {
    bounds_.expand(tile.relativePosition);
  }
}

} // namespace MapEditor::Services::Preview
