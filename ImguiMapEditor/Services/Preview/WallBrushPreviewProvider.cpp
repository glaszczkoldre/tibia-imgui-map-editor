#include "WallBrushPreviewProvider.h"

#include "Brushes/Types/WallBrush.h"
#include "Domain/ChunkedMap.h"
#include "Services/BrushSettingsService.h"

namespace MapEditor::Services::Preview {

namespace {

bool isPerimeterOffset(std::span<const std::pair<int, int>> offsets, int x,
                       int y) {
  auto hasOffset = [&](int dx, int dy) {
    for (const auto &[ox, oy] : offsets) {
      if (ox == dx && oy == dy)
        return true;
    }
    return false;
  };
  return !hasOffset(x - 1, y) || !hasOffset(x + 1, y) || !hasOffset(x, y - 1) ||
         !hasOffset(x, y + 1);
}

} // namespace

WallBrushPreviewProvider::WallBrushPreviewProvider(
    const Brushes::WallBrush *wallBrush, BrushSettingsService *brushSettings,
    const Domain::ChunkedMap *map)
    : wallBrush_(wallBrush), brushSettings_(brushSettings), map_(map) {
  buildPreview();
}

void WallBrushPreviewProvider::updateCursorPosition(
    const Domain::Position &cursor) {
  if (cursor != anchor_) {
    anchor_ = cursor;
    needsRegen_ = true;
  }
}

const std::vector<PreviewTileData> &
WallBrushPreviewProvider::getTiles() const {
  if (brushSettings_) {
    auto currentOffsets = getPerimeterOffsets();
    if (currentOffsets != cachedOffsets_) {
      needsRegen_ = true;
    }
  }
  if (needsRegen_) {
    buildPreview();
  }
  return tiles_;
}

PreviewStyle WallBrushPreviewProvider::getStyle() const {
  return PreviewStyle::Ghost;
}

std::vector<std::pair<int, int>>
WallBrushPreviewProvider::getPerimeterOffsets() const {
  if (!brushSettings_) {
    return {{0, 0}};
  }

  auto allOffsets = brushSettings_->getBrushOffsets();
  if (allOffsets.size() <= 1) {
    return allOffsets;
  }

  std::vector<std::pair<int, int>> perimeterOffsets;
  perimeterOffsets.reserve(allOffsets.size());
  for (const auto &[dx, dy] : allOffsets) {
    if (isPerimeterOffset(allOffsets, dx, dy)) {
      perimeterOffsets.emplace_back(dx, dy);
    }
  }
  return perimeterOffsets.empty() ? std::vector<std::pair<int, int>>{{0, 0}}
                                  : perimeterOffsets;
}

void WallBrushPreviewProvider::buildPreview() const {
  tiles_.clear();
  bounds_ = PreviewBounds();
  needsRegen_ = false;

  auto offsets = getPerimeterOffsets();
  cachedOffsets_ = offsets;

  if (wallBrush_ && map_) {
    tiles_ = wallBrush_->buildPreviewTiles(*map_, anchor_, offsets);
    for (const auto &tile : tiles_) {
      bounds_.expand(tile.relativePosition);
    }
    return;
  }

  uint16_t itemId = wallBrush_ ? wallBrush_->getPreviewItemId() : 0;

  for (const auto &[dx, dy] : offsets) {
    PreviewTileData tile(dx, dy, 0);
    if (itemId != 0) {
      tile.addItem(itemId);
    }
    tiles_.push_back(std::move(tile));
    bounds_.expand(dx, dy, 0);
  }
}

} // namespace MapEditor::Services::Preview
