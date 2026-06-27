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
    const Brushes::IBrush *brush, const BrushSettingsService *brushSettings,
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
    const bool forceSquare = brush_ && (brush_->getType() == ::MapEditor::Brushes::BrushType::Spawn);
    auto offsets = brushSettings_->getBrushOffsets(forceSquare);
    const bool autoBorder = brushSettings_->getAutoBorder();
    if (offsets != cachedOffsets_ || autoBorder != cachedAutoBorder_) {
      needsRegen_ = true;
      cachedAutoBorder_ = autoBorder;
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
  if (!brushSettings_ || !brush_) {
    positions.push_back(anchor_);
    cachedOffsets_ = {{0, 0}};
    return positions;
  }

  const bool forceSquare = brush_->getType() == ::MapEditor::Brushes::BrushType::Spawn;
  cachedOffsets_ = brushSettings_->getBrushOffsets(forceSquare);

  const bool isWallLikePerimeter = brush_->getType() == ::MapEditor::Brushes::BrushType::Wall ||
                                   brush_->getType() == ::MapEditor::Brushes::BrushType::WallDecoration;

  if (isWallLikePerimeter && cachedOffsets_.size() > 1) {
    auto hasOffset = [&](int dx, int dy) {
      for (const auto &[ox, oy] : cachedOffsets_) {
        if (ox == dx && oy == dy)
          return true;
      }
      return false;
    };

    auto isPerimeter = [&](int dx, int dy) {
      return !hasOffset(dx - 1, dy) || !hasOffset(dx + 1, dy) ||
             !hasOffset(dx, dy - 1) || !hasOffset(dx, dy + 1);
    };

    positions.reserve(cachedOffsets_.size());
    for (const auto &[dx, dy] : cachedOffsets_) {
      if (isPerimeter(dx, dy)) {
        positions.emplace_back(anchor_.x + dx, anchor_.y + dy, anchor_.z);
      }
    }
    if (positions.empty()) {
      positions.push_back(anchor_);
    }
    return positions;
  }

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
  intent.context.isDragging = false;
  intent.context.forcePlace = false;
  intent.context.brushSettings = const_cast<BrushSettingsService*>(brushSettings_);
  intent.positions = getPlacementPositions();

  if (brushSettings_ && !brushSettings_->getAutoBorder()) {
    tiles_.reserve(intent.positions.size());
    for (const auto &pos : intent.positions) {
      Domain::Tile tempTile(pos);
      if (const auto *existingTile = map_->getTile(pos)) {
        if (const auto *ground = existingTile->getGround()) {
          tempTile.setGround(ground->clone());
        }
        for (const auto &item : existingTile->getItems()) {
          if (item) {
            tempTile.addItem(item->clone());
          }
        }
      }
      const_cast<Brushes::IBrush *>(brush_)->draw(const_cast<Domain::ChunkedMap &>(*map_), &tempTile, intent.context);

      PreviewTileData previewTile(pos.x - anchor_.x, pos.y - anchor_.y, pos.z - anchor_.z);
      appendTileItems(previewTile, tempTile);
      if (!previewTile.empty()) {
        bounds_.expand(previewTile.relativePosition);
        tiles_.push_back(std::move(previewTile));
      }
    }
    return;
  }

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
