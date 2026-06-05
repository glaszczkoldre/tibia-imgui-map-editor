#include "AutoborderBrushPreviewProvider.h"

#include "Brushes/Core/IBrush.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Item.h"
#include "Domain/Tile.h"
#include "Services/BrushSettingsService.h"

namespace MapEditor::Services::Preview {

namespace {

void appendTileItems(PreviewTileData &previewTile, const Domain::Tile &tile) {
  if (const auto *ground = tile.getGround()) {
    previewTile.addItem(ground->getServerId(), ground->getCount());
  }

  for (const auto &item : tile.getItems()) {
    if (!item) {
      continue;
    }
    previewTile.addItem(item->getServerId(), item->getCount());
  }
}

} // namespace

AutoborderBrushPreviewProvider::AutoborderBrushPreviewProvider(
    const Brushes::IBrush *brush, BrushSettingsService *brushSettings,
    const Domain::ChunkedMap *map)
    : brush_(brush), brushSettings_(brushSettings), map_(map) {
  buildPreview();
}

bool AutoborderBrushPreviewProvider::isActive() const {
  return brush_ != nullptr && map_ != nullptr;
}

void AutoborderBrushPreviewProvider::updateCursorPosition(
    const Domain::Position &cursor) {
  if (cursor != anchor_) {
    anchor_ = cursor;
    needsRegen_ = true;
  }
}

const std::vector<PreviewTileData> &
AutoborderBrushPreviewProvider::getTiles() const {
  if (brushSettings_) {
    auto offsets = brushSettings_->getBrushOffsets();
    if (offsets != cachedOffsets_) {
      needsRegen_ = true;
    }
  }

  if (needsRegen_) {
    buildPreview();
  }
  return tiles_;
}

PreviewStyle AutoborderBrushPreviewProvider::getStyle() const {
  return brushSettings_ && brushSettings_->getPreviewBorder()
             ? PreviewStyle::Outline
             : PreviewStyle::Ghost;
}

std::vector<Domain::Position>
AutoborderBrushPreviewProvider::getPlacementPositions() const {
  std::vector<Domain::Position> positions;
  if (!brushSettings_) {
    positions.push_back(anchor_);
    cachedOffsets_ = {{0, 0}};
    return positions;
  }

  cachedOffsets_ = brushSettings_->getBrushOffsets();
  positions.reserve(cachedOffsets_.size());
  for (const auto &[dx, dy] : cachedOffsets_) {
    positions.emplace_back(anchor_.x + dx, anchor_.y + dy, anchor_.z);
  }
  return positions;
}

void AutoborderBrushPreviewProvider::buildPreview() const {
  tiles_.clear();
  bounds_ = PreviewBounds();
  needsRegen_ = false;

  if (!brush_ || !map_) {
    return;
  }

  Autoborder::PlacementIntent intent;
  intent.brush = brush_;
  intent.mode = Autoborder::PlacementMode::Draw;
  intent.context.isDragging = true;
  intent.context.forcePlace = false;
  intent.positions = getPlacementPositions();

  const auto diffs = engine_.plan(*map_, intent);
  tiles_.reserve(diffs.size());
  for (const auto &diff : diffs) {
    if (!diff.after) {
      continue;
    }

    PreviewTileData previewTile(diff.position.x - anchor_.x,
                                diff.position.y - anchor_.y,
                                diff.position.z - anchor_.z);
    appendTileItems(previewTile, *diff.after);
    if (previewTile.empty()) {
      continue;
    }

    bounds_.expand(previewTile.relativePosition);
    tiles_.push_back(std::move(previewTile));
  }
}

} // namespace MapEditor::Services::Preview
