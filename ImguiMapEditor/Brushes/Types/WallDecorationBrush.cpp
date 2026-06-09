#include "WallDecorationBrush.h"
#include "WallBrush.h"
#include "BrushUtils.h"
#include "Domain/Tile.h"
#include "Domain/Item.h"

namespace MapEditor::Brushes {

void WallDecorationBrush::draw(Domain::ChunkedMap &, Domain::Tile *tile,
                               const DrawContext &ctx) {
  if (!tile) {
    return;
  }

  tile->removeItemsIf([this](const Domain::Item *item) { return ownsItem(item); });

  for (size_t index = 0; index < tile->getItemCount(); ++index) {
    auto *item = tile->getItem(index);
    if (!item) {
      continue;
    }

    const auto *baseBrush = WallBrush::resolveWallBrushForItem(*item, getBrushRegistry());
    if (!baseBrush || baseBrush->getType() == BrushType::WallDecoration) {
      continue;
    }

    const auto alignment = baseBrush->getAlignmentForItem(item->getServerId());
    if (!alignment.has_value()) {
      continue;
    }

    uint16_t decorationId = 0;
    if (const auto door = baseBrush->findDoorForItem(item->getServerId());
        door.has_value()) {
      if (const auto decorationDoor =
              getDoorItemForAlign(*alignment, door->type, door->isOpen,
                                  door->isLocked);
          decorationDoor.has_value()) {
        decorationId = static_cast<uint16_t>(decorationDoor->getItem());
      }
    } else {
      decorationId = getWallItemForAlign(*alignment);
    }

    if (decorationId == 0) {
      continue;
    }

    tile->insertItem(index + 1, Types::createTypedItem(ctx, decorationId));
    ++index;
  }
}

void WallDecorationBrush::undraw(Domain::ChunkedMap &, Domain::Tile *tile) {
  if (!tile) {
    return;
  }

  tile->removeItemsIf([this](const Domain::Item *item) { return ownsItem(item); });
}

} // namespace MapEditor::Brushes
