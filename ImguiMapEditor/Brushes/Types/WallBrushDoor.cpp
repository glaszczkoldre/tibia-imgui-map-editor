#include "WallBrush.h"

#include "Brushes/BrushRegistry.h"
#include "Brushes/Types/BrushUtils.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Item.h"
#include "Domain/Tile.h"
#include <algorithm>
#include <functional>

namespace MapEditor::Brushes {

namespace {

Domain::Item *resolveDoorItem(Domain::Tile &tile,
                              const Domain::Item *preferredItem,
                              const WallBrush &brush) {
  if (preferredItem) {
    for (const auto &item : tile.getItems()) {
      if (item.get() == preferredItem &&
          brush.findDoorForItem(item->getServerId()).has_value()) {
        return item.get();
      }
    }
  }

  for (auto it = tile.getItems().rbegin(); it != tile.getItems().rend(); ++it) {
    if (*it && brush.findDoorForItem((*it)->getServerId()).has_value()) {
      return it->get();
    }
  }

  return nullptr;
}

} // namespace

bool WallBrush::canApplyDoor(const Domain::Tile &tile, DoorType type, bool open,
                             bool preferLocked) const {
  for (const auto &item : tile.getItems()) {
    if (!item) {
      continue;
    }

    const auto *itemBrush = resolveWallBrushForItem(*item, registry_);
    if (!itemBrush || itemBrush->getType() == BrushType::WallDecoration ||
        !connectsTo(itemBrush)) {
      continue;
    }

    const auto alignment = findAlignmentForItem(item->getServerId());
    if (!alignment.has_value()) {
      continue;
    }

    if (selectDoorItem(*alignment, type, open, preferLocked).has_value()) {
      return true;
    }
  }

  return false;
}

bool WallBrush::applyDoor(Domain::ChunkedMap &map, Domain::Tile &tile,
                          DoorType type, bool open, bool preferLocked,
                          BrushId ownerBrushId) const {
  const auto pos = tile.getPosition();
  const auto resolvedOwnerBrushId =
      ownerBrushId != InvalidBrushId ? ownerBrushId : registry_.getBrushId(this);
  auto *baseWallItem = [&]() -> Domain::Item * {
    for (const auto &item : tile.getItems()) {
      if (!item) {
        continue;
      }

      const auto *itemBrush = resolveWallBrushForItem(*item, registry_);
      if (!itemBrush || itemBrush->getType() == BrushType::WallDecoration ||
          !connectsTo(itemBrush)) {
        continue;
      }

      if (const auto alignment = findAlignmentForItem(item->getServerId());
          alignment && selectDoorItem(*alignment, type, open, preferLocked).has_value()) {
        return item.get();
      }
    }

    return nullptr;
  }();

  if (!baseWallItem) {
    return false;
  }

  const auto alignment = findAlignmentForItem(baseWallItem->getServerId());
  if (!alignment.has_value()) {
    return false;
  }

  const auto baseDoor = selectDoorItem(*alignment, type, open, preferLocked);
  if (!baseDoor.has_value()) {
    return false;
  }

  Types::updateItemVisuals(*baseWallItem, registry_,
                           static_cast<uint16_t>(baseDoor->getItem()),
                           resolvedOwnerBrushId);

  updateConsecutiveDecorations(
      tile, baseWallItem,
      [alignment = *alignment, type, open, preferLocked](
          const WallBrush &decorationBrush, const Domain::Item &) -> uint16_t {
        if (const auto decorationDoor =
                decorationBrush.getDoorItemForAlign(alignment, type, open,
                                                    preferLocked);
            decorationDoor.has_value()) {
          return static_cast<uint16_t>(decorationDoor->getItem());
        }

        return 0;
      });

  tile.markDirty();
  rebuildAround(map, pos);
  map.markChanged();
  return true;
}

bool WallBrush::removeDoor(Domain::ChunkedMap &map, Domain::Tile &tile,
                           const Domain::Item *preferredItem) const {
  auto *targetItem = resolveDoorItem(tile, preferredItem, *this);
  if (!targetItem) {
    return false;
  }

  const auto currentDoor = findDoorForItem(targetItem->getServerId());
  if (!currentDoor) {
    return false;
  }

  const auto alignment =
      findAlignmentForItem(targetItem->getServerId()).value_or(currentDoor->alignment);
  const auto replacementId = selectWallItem(alignment);
  if (replacementId == 0) {
    return false;
  }

  const auto ownerBrushId = targetItem->getOwnerBrushId() != InvalidBrushId
                                ? targetItem->getOwnerBrushId()
                                : registry_.getBrushId(this);
  Types::updateItemVisuals(*targetItem, registry_, replacementId, ownerBrushId);

  updateConsecutiveDecorations(
      tile, targetItem,
      [alignment](const WallBrush &decorationBrush,
                  const Domain::Item &) -> uint16_t {
        return decorationBrush.getWallItemForAlign(alignment);
      });

  tile.markDirty();
  rebuildAround(map, tile.getPosition());
  map.markChanged();
  return true;
}

bool WallBrush::switchDoor(Domain::ChunkedMap &map, Domain::Tile &tile,
                           const Domain::Item *preferredItem,
                           bool preferLocked) const {
  auto *targetItem = resolveDoorItem(tile, preferredItem, *this);

  if (!targetItem) {
    return false;
  }

  const auto currentDoor = findDoorForItem(targetItem->getServerId());
  if (!currentDoor) {
    return false;
  }

  const auto alignment =
      findAlignmentForItem(targetItem->getServerId()).value_or(currentDoor->alignment);
  const auto replacement =
      selectDoorItem(alignment, currentDoor->type, !currentDoor->isOpen, preferLocked);
  if (!replacement || replacement->getItem() == 0) {
    return false;
  }

  const auto replacementId = static_cast<uint16_t>(replacement->getItem());
  Types::updateItemVisuals(*targetItem, registry_, replacementId,
                           targetItem->getOwnerBrushId());

  updateConsecutiveDecorations(
      tile, targetItem,
      [alignment, replacement, preferLocked](const WallBrush &decorationBrush,
                                             const Domain::Item &) -> uint16_t {
        if (const auto decorationDoor =
                decorationBrush.getDoorItemForAlign(alignment, replacement->type,
                                                    replacement->isOpen,
                                                    preferLocked);
            decorationDoor.has_value()) {
          return static_cast<uint16_t>(decorationDoor->getItem());
        }

        return 0;
      });

  tile.markDirty();
  rebuildAround(map, tile.getPosition());
  map.markChanged();
  return true;
}

std::optional<DoorNode> WallBrush::findDoorForItem(uint16_t itemId) const {
  std::optional<DoorNode> foundDoor;
  visitWallRedirectChain([&](const WallBrush &brush) {
    if (const auto it = brush.doorNodesByItemId_.find(itemId);
        it != brush.doorNodesByItemId_.end()) {
      foundDoor = it->second;
      return true;
    }
    return false;
  });
  return foundDoor;
}

std::optional<DoorNode> WallBrush::getDoorItemForAlign(WallAlign align,
                                                       DoorType type, bool open,
                                                       bool preferLocked) const {
  return selectDoorItem(align, type, open, preferLocked);
}

std::optional<DoorNode> WallBrush::selectDoorItem(WallAlign align,
                                                  DoorType type, bool open,
                                                  bool preferLocked) const {
  std::optional<DoorNode> bestMatch;
  int bestRank = -1;

  visitWallRedirectChain([&](const WallBrush &brush) {
    const auto &doors = brush.doorNodes_[static_cast<size_t>(align)];

    for (const auto &door : doors) {
      if (door.type != type) {
        continue;
      }

      const int rank = (door.isOpen == open)
                           ? ((!preferLocked || door.isLocked) ? 3 : 2)
                           : 1;
      if (rank > bestRank) {
        bestMatch = door;
        bestRank = rank;
        if (bestRank == 3) {
          return true;
        }
      }
    }

    return false;
  });

  return bestMatch;
}

} // namespace MapEditor::Brushes
