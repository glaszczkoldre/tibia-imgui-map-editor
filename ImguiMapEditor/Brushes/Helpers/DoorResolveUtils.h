#pragma once
#include "Domain/Tile.h"
#include "Domain/Item.h"
#include "Brushes/Types/WallBrush.h"
#include "Brushes/BrushRegistry.h"

namespace MapEditor::Brushes {

struct ResolvedDoorTarget {
  WallBrush *wallBrush = nullptr;
  Domain::Item *item = nullptr;
};

struct ResolvedConstDoorTarget {
  const WallBrush *wallBrush = nullptr;
  const Domain::Item *item = nullptr;
};

inline const Domain::Item *resolveTileItem(const Domain::Tile *tile,
                                           const Domain::Item *preferredItem) {
  if (!tile) {
    return nullptr;
  }

  if (preferredItem) {
    for (const auto &item : tile->getItems()) {
      if (item.get() == preferredItem) {
        return item.get();
      }
    }
  }

  for (auto it = tile->getItems().rbegin(); it != tile->getItems().rend(); ++it) {
    if (*it) {
      return it->get();
    }
  }

  return nullptr;
}

inline Domain::Item *resolveMutableTileItem(Domain::Tile *tile,
                                            const Domain::Item *preferredItem) {
  return const_cast<Domain::Item *>(resolveTileItem(tile, preferredItem));
}

inline ResolvedConstDoorTarget resolveDoorTarget(const Domain::Tile *tile,
                                                 const BrushRegistry *registry,
                                                 const Domain::Item *preferredItem) {
  if (!tile || !registry) {
    return {};
  }

  const auto resolveItem = [registry](const Domain::Item *item) -> const WallBrush * {
    if (!item) {
      return nullptr;
    }

    for (const auto *brush : registry->getBrushesForItem(item->getServerId())) {
      const auto *wallBrush = dynamic_cast<const WallBrush *>(brush);
      if (wallBrush &&
          wallBrush->findDoorForItem(item->getServerId()).has_value()) {
        return wallBrush;
      }
    }

    return nullptr;
  };

  if (preferredItem) {
    for (const auto &item : tile->getItems()) {
      if (item.get() != preferredItem) {
        continue;
      }

      if (const auto *wallBrush = resolveItem(item.get())) {
        return {.wallBrush = wallBrush, .item = item.get()};
      }
      break;
    }
  }

  for (auto it = tile->getItems().rbegin(); it != tile->getItems().rend(); ++it) {
    if (!*it) {
      continue;
    }

    if (const auto *wallBrush = resolveItem(it->get())) {
      return {.wallBrush = wallBrush, .item = it->get()};
    }
  }

  return {};
}

inline ResolvedDoorTarget resolveDoorTarget(Domain::Tile *tile,
                                            BrushRegistry *registry,
                                            const Domain::Item *preferredItem) {
  auto constResult = resolveDoorTarget(static_cast<const Domain::Tile *>(tile),
                                       static_cast<const BrushRegistry *>(registry),
                                       preferredItem);
  return {
    .wallBrush = const_cast<WallBrush *>(constResult.wallBrush),
    .item = const_cast<Domain::Item *>(constResult.item)
  };
}

} // namespace MapEditor::Brushes
