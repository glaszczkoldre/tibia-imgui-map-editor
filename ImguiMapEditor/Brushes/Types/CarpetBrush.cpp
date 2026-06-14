#include "CarpetBrush.h"

#include "Brushes/BrushRegistry.h"
#include "Brushes/Helpers/AlignedBrushHelpers.h"
#include "Brushes/Types/BrushUtils.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Item.h"
#include "Domain/Tile.h"
#include "Services/Brushes/CarpetLookupService.h"
#include <algorithm>
#include <array>

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

CarpetBrush::CarpetBrush(std::string name, uint32_t lookId,
                         BrushRegistry &registry)
    : BrushBase(std::move(name), lookId, true), registry_(registry) {}

void CarpetBrush::draw(Domain::ChunkedMap &map, Domain::Tile *tile,
                       const DrawContext &ctx) {
  if (!tile) {
    return;
  }

  placeCenterTile(*tile, ctx);
  rebuildAround(map, tile->getPosition());
}

void CarpetBrush::undraw(Domain::ChunkedMap &map, Domain::Tile *tile) {
  if (!tile) {
    return;
  }
  eraseFromTile(*tile);
  rebuildAround(map, tile->getPosition());
}

bool CarpetBrush::ownsItem(const Domain::Item *item) const {
  return item && ownedItemIds_.contains(item->getServerId());
}

void CarpetBrush::addAlignedItem(EdgeType align, uint16_t itemId,
                                 uint32_t chance) {
  itemsByEdge_[static_cast<size_t>(align)].emplace_back(itemId,
                                                        chance == 0 ? 1u : chance);
  ownedItemIds_.insert(itemId);
  registry_.registerItemBinding(itemId, this);
  if (lookId_ == 0) {
    lookId_ = itemId;
  }
}

uint16_t CarpetBrush::getPreviewItemId() const {
  const auto center = selectItem(EdgeType::Center);
  return center != 0 ? center : lookId_;
}

void CarpetBrush::placeCenterTile(Domain::Tile &tile,
                                  const DrawContext &ctx) const {
  eraseFromTile(tile);
  const auto centerId = selectItem(EdgeType::Center);
  if (centerId != 0) {
    tile.addItem(Types::createTypedItem(ctx, centerId));
  }
}

void CarpetBrush::eraseFromTile(Domain::Tile &tile) const {
  tile.removeItemsIf([this](const Domain::Item *item) { return ownsItem(item); });
}

void CarpetBrush::rebuildAround(Domain::ChunkedMap &map,
                                const Domain::Position &center) const {
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      rebuildTile(map, {center.x + dx, center.y + dy, center.z});
    }
  }
}

uint16_t CarpetBrush::selectItem(EdgeType align) const {
  return Helpers::selectWeightedItem(
      itemsByEdge_[static_cast<size_t>(align)]);
}

std::vector<CarpetBrush::PlannedItem>
CarpetBrush::planTile(const Domain::ChunkedMap &map,
                      const Domain::Position &pos) const {
  const auto *tile = map.getTile(pos);
  if (!tile || !tileHasBrush(tile)) {
    return {};
  }

  const auto neighborMask =
      Helpers::computeOwnedNeighborMask(map, *this, pos);
  static const Services::Brushes::CarpetLookupService carpetLookupService;
  const auto packed = carpetLookupService.getCarpetTypes(neighborMask);
  auto types = Services::Brushes::CarpetLookupService::unpack(packed);
  if (types.empty()) {
    types.push_back(EdgeType::Center);
  }

  std::vector<PlannedItem> plan;
  plan.reserve(types.size());
  for (const auto edge : types) {
    const auto id = selectItem(edge);
    if (id == 0) {
      continue;
    }
    plan.push_back(PlannedItem{.itemId = id, .alignment = edge});
  }

  // RME's fallback: if the lookup produced nothing useful, default to a
  // center item so the tile is never empty when it should hold carpet.
  if (plan.empty()) {
    if (const auto id = selectItem(EdgeType::Center); id != 0) {
      plan.push_back(PlannedItem{.itemId = id, .alignment = EdgeType::Center});
    }
  }

  return plan;
}

void CarpetBrush::applyTilePlan(Domain::Tile &tile,
                                const std::vector<PlannedItem> &plan) const {
  auto ownedItems = Helpers::collectOwnedItems(tile, *this);
  const auto ownerBrushId = registry_.getBrushId(this);

  for (size_t i = 0; i < plan.size(); ++i) {
    if (i < ownedItems.size()) {
      Types::updateItemVisuals(*ownedItems[i], registry_, plan[i].itemId,
                               ownerBrushId);
    } else {
      tile.addItem(Types::createTypedItem(makeBorderContext(registry_, this),
                                         plan[i].itemId));
    }
  }

  if (plan.size() < ownedItems.size()) {
    const auto keep = plan.size();
    tile.removeItemsIf([this, &ownedItems, keep](const Domain::Item *item) {
      if (!ownsItem(item)) {
        return false;
      }
      // remove any owned item that wasn't kept in plan
      auto it = std::find(ownedItems.begin(), ownedItems.end(), item);
      if (it == ownedItems.end()) {
        return true;
      }
      const auto index = static_cast<size_t>(std::distance(ownedItems.begin(), it));
      return index >= keep;
    });
  }
}

void CarpetBrush::rebuildTile(Domain::ChunkedMap &map,
                              const Domain::Position &pos) const {
  auto *tile = map.getTile(pos);
  if (!tile || !tileHasBrush(tile)) {
    return;
  }

  applyTilePlan(*tile, planTile(map, pos));
}

bool CarpetBrush::tileHasBrush(const Domain::Tile *tile) const {
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
