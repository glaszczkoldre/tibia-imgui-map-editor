#include "TableBrush.h"

#include "Brushes/BrushRegistry.h"
#include "Brushes/Helpers/AlignedBrushHelpers.h"
#include "Brushes/Types/BrushUtils.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Item.h"
#include "Domain/Tile.h"
#include "Services/Brushes/TableLookupService.h"
#include <array>
#include <algorithm>

namespace MapEditor::Brushes {

namespace {

DrawContext makeBorderContext(BrushRegistry &registry, const IBrush *owner) {
  DrawContext ctx;
  ctx.clientData = registry.getClientDataService();
  ctx.brushRegistry = &registry;
  ctx.ownerBrushId = registry.getBrushId(owner);
  return ctx;
}

} // namespace

TableBrush::TableBrush(std::string name, uint32_t lookId,
                       BrushRegistry &registry)
    : BrushBase(std::move(name), lookId, true), registry_(registry) {}

void TableBrush::draw(Domain::ChunkedMap &map, Domain::Tile *tile,
                      const DrawContext &ctx) {
  if (!tile) {
    return;
  }

  placeAloneTile(*tile, ctx);
  rebuildAround(map, tile->getPosition());
}

void TableBrush::undraw(Domain::ChunkedMap &map, Domain::Tile *tile) {
  if (!tile) {
    return;
  }
  eraseFromTile(*tile);
  rebuildAround(map, tile->getPosition());
}

bool TableBrush::ownsItem(const Domain::Item *item) const {
  return item && ownedItemIds_.contains(item->getServerId());
}

void TableBrush::addAlignedItem(TableAlign align, uint16_t itemId,
                                uint32_t chance) {
  itemsByAlign_[static_cast<size_t>(align)].emplace_back(itemId,
                                                         chance == 0 ? 1u : chance);
  ownedItemIds_.insert(itemId);
  registry_.registerItemBinding(itemId, this);
  if (lookId_ == 0) {
    lookId_ = itemId;
  }
}

uint16_t TableBrush::getPreviewItemId() const {
  const auto horizontal = selectItem(TableAlign::Horizontal);
  return horizontal != 0 ? horizontal : lookId_;
}

void TableBrush::placeAloneTile(Domain::Tile &tile,
                                const DrawContext &ctx) const {
  eraseFromTile(tile);

  const auto itemId = selectItem(TableAlign::Alone);
  if (itemId != 0) {
    tile.addItem(Types::createTypedItem(ctx, itemId));
  }
}

void TableBrush::eraseFromTile(Domain::Tile &tile) const {
  tile.removeItemsIf([this](const Domain::Item *item) { return ownsItem(item); });
}

void TableBrush::rebuildAround(Domain::ChunkedMap &map,
                               const Domain::Position &center) const {
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      rebuildTile(map, {center.x + dx, center.y + dy, center.z});
    }
  }
}

uint16_t TableBrush::selectItem(TableAlign align) const {
  return Helpers::selectWeightedItem(
      itemsByAlign_[static_cast<size_t>(align)]);
}

void TableBrush::rebuildTile(Domain::ChunkedMap &map,
                             const Domain::Position &pos) const {
  auto *tile = map.getTile(pos);
  if (!tile || !tileHasBrush(tile)) {
    return;
  }

  const auto neighborMask =
      Helpers::computeOwnedNeighborMask(map, *this, pos);
  static const Services::Brushes::TableLookupService tableLookupService;
  const auto align = tableLookupService.getTableType(neighborMask);
  auto itemId = selectItem(align);
  if (itemId == 0) {
    itemId = selectItem(TableAlign::Alone);
  }
  if (itemId == 0) {
    return;
  }

  const auto ownerBrushId = registry_.getBrushId(this);
  auto ownedItems = Helpers::collectOwnedItems(*tile, *this);
  if (!ownedItems.empty()) {
    Types::updateItemVisuals(*ownedItems.front(), registry_, itemId,
                             ownerBrushId);
    tile->removeItemsIf([this, keep = ownedItems.front()](const Domain::Item *item) {
      return ownsItem(item) && item != keep;
    });
    return;
  }

  tile->addItem(Types::createTypedItem(makeBorderContext(registry_, this), itemId));
}

bool TableBrush::tileHasBrush(const Domain::Tile *tile) const {
  if (!tile) {
    return false;
  }
  for (const auto &item : tile->getItems()) {
    if (item && ownsItem(item.get())) {
      return true;
    }
  }
  return false;
}

} // namespace MapEditor::Brushes
