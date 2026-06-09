#include "BrushUtils.h"

#include "Brushes/BrushRegistry.h"
#include "Domain/Item.h"
#include "Domain/ItemType.h"
#include "Domain/Tile.h"
#include "Services/ClientDataService.h"

namespace MapEditor::Brushes::Types {

std::unique_ptr<Domain::Item> createTypedItem(const DrawContext &ctx,
                                              uint16_t itemId,
                                              uint16_t subtype) {
  auto applyOwnership = [&ctx](std::unique_ptr<Domain::Item> item) {
    if (item && ctx.ownerBrushId != InvalidBrushId) {
      item->setOwnerBrushId(ctx.ownerBrushId);
    }
    return item;
  };

  if (ctx.brushRegistry) {
    return applyOwnership(ctx.brushRegistry->createItem(itemId, subtype));
  }

  auto item = std::make_unique<Domain::Item>(itemId, subtype);
  if (ctx.clientData) {
    if (const auto *type = ctx.clientData->getItemTypeByServerId(itemId)) {
      item->setType(type);
      item->setClientId(type->client_id);
    }
  }
  return applyOwnership(std::move(item));
}

void updateItemVisuals(Domain::Item &item,
                       Services::ClientDataService *clientData,
                       uint16_t itemId, BrushId ownerBrushId) {
  item.setServerId(itemId);
  item.setOwnerBrushId(ownerBrushId);

  if (clientData) {
    if (const auto *itemType = clientData->getItemTypeByServerId(itemId)) {
      item.setType(itemType);
      item.setClientId(itemType->client_id);
      return;
    }
  }

  item.setType(nullptr);
  item.setClientId(0);
}

void updateItemVisuals(Domain::Item &item, BrushRegistry &registry,
                       uint16_t itemId, BrushId ownerBrushId) {
  updateItemVisuals(item, registry.getClientDataService(), itemId, ownerBrushId);
}

constexpr uint32_t kBlockingItemFlags = static_cast<uint32_t>(Domain::ItemFlag::Unpassable) |
                                        static_cast<uint32_t>(Domain::ItemFlag::BlockPathfinder) |
                                        static_cast<uint32_t>(Domain::ItemFlag::FullTile);

bool itemBlocksPlacement(const Domain::Item *item) {
  if (!item) {
    return false;
  }
  const auto *type = item->getType();
  return type && (type->is_blocking ||
                  (static_cast<uint32_t>(type->flags) & kBlockingItemFlags) != 0);
}

bool tileHasBlockingContents(const Domain::Tile *tile) {
  if (!tile) {
    return false;
  }

  if (itemBlocksPlacement(tile->getGround())) {
    return true;
  }

  for (const auto &item : tile->getItems()) {
    if (itemBlocksPlacement(item.get())) {
      return true;
    }
  }

  return false;
}

} // namespace MapEditor::Brushes::Types
