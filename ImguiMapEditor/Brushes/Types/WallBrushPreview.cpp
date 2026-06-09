#include "WallBrush.h"

#include "Brushes/BrushRegistry.h"
#include "BrushUtils.h"
#include "Services/Brushes/WallLookupService.h"
#include "Services/Preview/PreviewTypes.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Item.h"
#include "Domain/Tile.h"
#include <array>
#include <span>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace MapEditor::Brushes {

namespace {

constexpr std::array<std::tuple<int, int, WallNeighbor>, 4> kWallNeighbors{{
    {0, -1, WallNeighbor::North},
    {-1, 0, WallNeighbor::West},
    {1, 0, WallNeighbor::East},
    {0, 1, WallNeighbor::South},
}};

} // namespace

std::vector<Services::Preview::PreviewTileData> WallBrush::buildPreviewTiles(
    const Domain::ChunkedMap &map, const Domain::Position &anchor,
    std::span<const std::pair<int, int>> offsets) const {
  std::unordered_set<Domain::Position> placedPositions;
  placedPositions.reserve(offsets.size());
  for (const auto &[dx, dy] : offsets) {
    placedPositions.emplace(anchor.x + dx, anchor.y + dy, anchor.z);
  }

  const auto tileMatchesWallGroup = [&](const Domain::Position &pos) {
    if (placedPositions.contains(pos)) {
      return true;
    }

    const auto *tile = map.getTile(pos);
    if (!tile) {
      return false;
    }

    for (const auto &item : tile->getItems()) {
      if (!item || wallHateMeItems_.contains(item->getServerId())) {
        continue;
      }

      const auto *itemBrush = resolveWallBrushForItem(*item, registry_);
      if ((itemBrush && connectsTo(itemBrush)) ||
          findAlignmentForItem(item->getServerId()).has_value()) {
        return true;
      }
    }

    return false;
  };

  std::unordered_set<Domain::Position> affectedPositions;
  affectedPositions.reserve(placedPositions.size() * 9);
  for (const auto &center : placedPositions) {
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        affectedPositions.emplace(center.x + dx, center.y + dy, center.z);
      }
    }
  }

  std::vector<Domain::Position> orderedPositions;
  orderedPositions.reserve(affectedPositions.size());
  for (const auto &pos : affectedPositions) {
    orderedPositions.push_back(pos);
  }
  std::sort(orderedPositions.begin(), orderedPositions.end());

  static const Services::Brushes::WallLookupService lookupService;
  std::vector<Services::Preview::PreviewTileData> previewTiles;
  previewTiles.reserve(orderedPositions.size());

  for (const auto &pos : orderedPositions) {
    const bool isPlacedTile = placedPositions.contains(pos);
    const auto *tile = map.getTile(pos);
    if (!isPlacedTile && !tileMatchesWallGroup(pos)) {
      continue;
    }

    DoorType currentDoorType = DoorType::Undefined;
    bool isOpen = false;
    bool isLocked = false;
    std::optional<WallAlign> currentAlignment;

    if (!isPlacedTile && tile) {
      for (const auto &item : tile->getItems()) {
        if (!item) {
          continue;
        }

        const auto *itemBrush = resolveWallBrushForItem(*item, registry_);
        if (!itemBrush || itemBrush->getType() == BrushType::WallDecoration ||
            !connectsTo(itemBrush)) {
          continue;
        }

        currentAlignment = findAlignmentForItem(item->getServerId());
        if (const auto door = findDoorForItem(item->getServerId())) {
          currentDoorType = door->type;
          isOpen = door->isOpen;
          isLocked = door->isLocked;
        }
        break;
      }
    }

    WallNeighbor neighbors = WallNeighbor::None;
    for (const auto &[dx, dy, bit] : kWallNeighbors) {
      if (tileMatchesWallGroup({pos.x + dx, pos.y + dy, pos.z})) {
        neighbors |= bit;
      }
    }

    const auto fullAlign = lookupService.getFullType(neighbors);
    const auto halfAlign = lookupService.getHalfType(neighbors);
    auto resolvedAlignment = fullAlign;
    uint16_t previewItemId = 0;

    if (currentDoorType != DoorType::Undefined) {
      if (currentAlignment) {
        if (const auto door =
                getDoorItemForAlign(*currentAlignment, currentDoorType, isOpen,
                                    isLocked)) {
          previewItemId = static_cast<uint16_t>(door->getItem());
          resolvedAlignment = *currentAlignment;
        }
      }

      if (previewItemId == 0) {
        if (const auto door =
                getDoorItemForAlign(fullAlign, currentDoorType, isOpen,
                                    isLocked)) {
          previewItemId = static_cast<uint16_t>(door->getItem());
          resolvedAlignment = fullAlign;
        }
      }

      if (previewItemId == 0 && halfAlign != fullAlign) {
        if (const auto door =
                getDoorItemForAlign(halfAlign, currentDoorType, isOpen,
                                    isLocked)) {
          previewItemId = static_cast<uint16_t>(door->getItem());
          resolvedAlignment = halfAlign;
        }
      }
    }

    if (previewItemId == 0) {
      previewItemId = getWallItemForAlign(fullAlign);
      resolvedAlignment = fullAlign;
    }
    if (previewItemId == 0 && halfAlign != fullAlign) {
      previewItemId = getWallItemForAlign(halfAlign);
      resolvedAlignment = halfAlign;
    }
    if (previewItemId == 0) {
      previewItemId = getWallItemForAlign(WallAlign::Horizontal);
      resolvedAlignment = WallAlign::Horizontal;
    }

    if (previewItemId == 0) {
      continue;
    }

    Services::Preview::PreviewTileData previewTile(
        pos.x - anchor.x, pos.y - anchor.y, pos.z - anchor.z);
    previewTile.addItem(previewItemId);

    if (!isPlacedTile && tile) {
      bool collectDecorations = false;
      for (const auto &item : tile->getItems()) {
        if (!item) {
          continue;
        }

        const auto *itemBrush = resolveWallBrushForItem(*item, registry_);
        const auto *decorationBrush = dynamic_cast<const WallBrush *>(itemBrush);
        if (!collectDecorations) {
          if (itemBrush && itemBrush->getType() != BrushType::WallDecoration &&
              connectsTo(itemBrush)) {
            collectDecorations = true;
          }
          continue;
        }

        if (!decorationBrush ||
            decorationBrush->getType() != BrushType::WallDecoration) {
          break;
        }

        uint16_t decorationItemId = 0;
        if (currentDoorType != DoorType::Undefined) {
          if (const auto decorationDoor =
                  decorationBrush->getDoorItemForAlign(
                      resolvedAlignment, currentDoorType, isOpen, isLocked)) {
            decorationItemId = static_cast<uint16_t>(decorationDoor->getItem());
          }
        } else {
          decorationItemId = decorationBrush->getWallItemForAlign(resolvedAlignment);
        }

        if (decorationItemId != 0) {
          previewTile.addItem(decorationItemId);
        }
      }
    }

    previewTiles.push_back(std::move(previewTile));
  }

  return previewTiles;
}

} // namespace MapEditor::Brushes
