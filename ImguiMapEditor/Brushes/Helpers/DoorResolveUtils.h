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

inline Domain::Item *resolveMutableTileItem(Domain::Tile *tile,
                                            const Domain::Item *preferredItem) {
  if (!tile) {
    return nullptr;
  }

  if (preferredItem) {
    for (auto &item : tile->getItems()) {
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

inline ResolvedDoorTarget resolveDoorTarget(const Domain::Tile *tile,
                                            BrushRegistry *registry,
                                            const Domain::Item *preferredItem) {
  if (!tile || !registry) {
    return {};
  }

  const auto resolveItem = [registry](Domain::Item *item) -> WallBrush * {
    if (!item) {
      return nullptr;
    }

    for (auto *brush : registry->getBrushesForItem(item->getServerId())) {
      auto *wallBrush = dynamic_cast<WallBrush *>(brush);
      if (wallBrush &&
          wallBrush->findDoorForItem(item->getServerId()).has_value()) {
        return wallBrush;
      }
    }

    return nullptr;
  };

  if (preferredItem) {
    for (auto &item : tile->getItems()) {
      if (item.get() != preferredItem) {
        continue;
      }

      if (auto *wallBrush = resolveItem(item.get())) {
        return {.wallBrush = wallBrush, .item = item.get()};
      }
      break;
    }
  }

  for (auto it = tile->getItems().rbegin(); it != tile->getItems().rend(); ++it) {
    if (!*it) {
      continue;
    }

    if (auto *wallBrush = resolveItem(it->get())) {
      return {.wallBrush = wallBrush, .item = it->get()};
    }
  }

  return {};
}

} // namespace MapEditor::Brushes
