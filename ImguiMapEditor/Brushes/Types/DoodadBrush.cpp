#include "DoodadBrush.h"

#include "BrushUtils.h"
#include "Brushes/Behaviors/WeightedSelection.h"
#include "Brushes/BrushRegistry.h"
#include "Brushes/Types/DoodadPlacementPlanner.h"
#include "Brushes/Types/DoodadRedoBorderPlanner.h"
#include "Brushes/Types/CarpetBrush.h"
#include "Brushes/Types/GroundBrush.h"
#include "Brushes/Types/TableBrush.h"
#include "Brushes/Types/WallBrush.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Item.h"
#include "Domain/ItemType.h"
#include "Domain/Tile.h"
#include "Utils/PositionUtils.h"
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

namespace MapEditor::Brushes {

namespace {

void rebuildTileWithBrush(Domain::ChunkedMap &map,
                          const Domain::Position &position, IBrush *brush) {
  if (auto *groundBrush = dynamic_cast<GroundBrush *>(brush)) {
    groundBrush->rebuildTile(map, position);
  } else if (auto *wallBrush = dynamic_cast<WallBrush *>(brush)) {
    wallBrush->rebuildTile(map, position);
  }
}

void rebuildBrushesAtPosition(Domain::ChunkedMap &map, BrushRegistry &registry,
                              const Domain::Position &position) {
  const auto *tile = map.getTile(position);
  if (!tile) {
    return;
  }

  std::unordered_set<const IBrush *> rebuiltBrushes;
  const auto rebuildBrush = [&](const Domain::Item *item) {
    if (!item) {
      return;
    }

    // Resolve owner metadata
    if (const auto ownerId = item->getOwnerBrushId(); ownerId != InvalidBrushId) {
      if (auto *brush = registry.getBrushById(ownerId)) {
        if (rebuiltBrushes.insert(brush).second) {
          rebuildTileWithBrush(map, position, brush);
        }
      }
    }

    // Resolve legacy item bindings
    for (auto *brush : registry.getBrushesForItem(item->getServerId())) {
      if (!brush || !rebuiltBrushes.insert(brush).second) {
        continue;
      }

      rebuildTileWithBrush(map, position, brush);
    }
  };

  if (tile->getGround()) {
    rebuildBrush(tile->getGround());
  }

  for (const auto &item : tile->getItems()) {
    rebuildBrush(item.get());
  }
}

void recordRedoBorderTouch(
    std::vector<DoodadRedoBorderTouch> &touches,
    std::unordered_map<Domain::Position, size_t> &touchIndexes,
    const Domain::Position &position, const Domain::ItemType *itemType) {
  const auto [it, inserted] = touchIndexes.emplace(position, touches.size());
  if (inserted) {
    touches.push_back({.position = position});
  }

  auto &touch = touches[it->second];
  if (itemType && itemType->isGround()) {
    touch.placedGround = true;
  } else if (itemType && itemType->is_wall) {
    touch.placedWall = true;
  }
}

} // namespace

DoodadBrush::DoodadBrush(std::string name, uint32_t lookId,
                         BrushRegistry &registry, bool draggable)
    : BrushBase(std::move(name), lookId, draggable), registry_(registry) {}

void DoodadBrush::draw(Domain::ChunkedMap &map, Domain::Tile *tile,
                       const DrawContext &ctx) {
  if (!tile) {
    return;
  }

  const auto plan =
      buildPlacementPlan(tile->getPosition(), ctx.brushSettings,
                         static_cast<size_t>(ctx.variation), &map,
                         true);
  if (plan.layout.empty()) {
    return;
  }

  applyPlacementPlan(map, tile->getPosition(), plan, ctx);
}

void DoodadBrush::undraw(Domain::ChunkedMap &map, Domain::Tile *tile) {
  undraw(map, tile, {});
}

void DoodadBrush::undraw(Domain::ChunkedMap &map, Domain::Tile *tile,
                         EraseOptions options) {
  if (!tile) {
    return;
  }

  bool removedAny = false;
  DoodadRedoBorderTouch redoTouch{.position = tile->getPosition()};
  auto groundOptions = options;
  groundOptions.preserveComplexItems = false;
  if (tile->getGround() &&
      shouldEraseDoodadItem(tile->getGround(), groundOptions)) {
    redoTouch.placedGround = true;
    tile->removeGround();
    removedAny = true;
  }

  removedAny = tile->removeItemsIf([this, &redoTouch, options](const Domain::Item *item) {
                 if (!shouldEraseDoodadItem(item, options)) {
                   return false;
                 }
                 const auto *itemType = item->getType();
                 if (itemType && itemType->is_wall) {
                   redoTouch.placedWall = true;
                 }
                 return true;
               }) > 0 ||
               removedAny;

  if (!removedAny || !redoBorders_) {
    return;
  }

  const auto redoPositions = buildDoodadRedoBorderPositions(
      std::span<const DoodadRedoBorderTouch>(&redoTouch, 1));
  for (const auto &position : redoPositions) {
    rebuildBrushesAtPosition(map, registry_, position);
  }
}

bool DoodadBrush::ownsItem(const Domain::Item *item) const {
  if (!item) {
    return false;
  }

  const auto brushId = registry_.getBrushId(this);
  if (const auto ownerBrushId = item->getOwnerBrushId();
      ownerBrushId != InvalidBrushId) {
    return brushId != InvalidBrushId && ownerBrushId == brushId;
  }

  return ownedItemIds_.contains(item->getServerId());
}

bool DoodadBrush::shouldEraseDoodadItem(const Domain::Item *item,
                                        EraseOptions options) const {
  if (!item) {
    return false;
  }
  if (options.preserveComplexItems && item->isComplex()) {
    return false;
  }
  if (options.matchingBrushOnly) {
    return ownsItem(item);
  }

  if (const auto ownerBrushId = item->getOwnerBrushId();
      ownerBrushId != InvalidBrushId) {
    if (auto *ownerBrush = registry_.getBrushById(ownerBrushId);
        ownerBrush && ownerBrush->getType() == BrushType::Doodad) {
      return true;
    }
  }

  for (auto *brush : registry_.getBrushesForItem(item->getServerId())) {
    if (brush && brush->getType() == BrushType::Doodad) {
      return true;
    }
  }

  return false;
}

void DoodadBrush::addAlternative(DoodadAlternative alternative) {
  for (const auto &single : alternative.getSingleItems()) {
    ownedItemIds_.insert(static_cast<uint16_t>(single.itemId));
    registry_.registerItemBinding(static_cast<uint16_t>(single.itemId), this);
    if (lookId_ == 0) {
      lookId_ = single.itemId;
    }
  }

  for (const auto &composite : alternative.getComposites()) {
    for (const auto &tile : composite.tiles) {
      for (const auto &item : tile.items) {
        ownedItemIds_.insert(static_cast<uint16_t>(item.itemId));
        registry_.registerItemBinding(static_cast<uint16_t>(item.itemId), this);
        if (lookId_ == 0) {
          lookId_ = item.itemId;
        }
      }
    }
  }

  alternatives_.push_back(std::move(alternative));
}

uint16_t DoodadBrush::getPreviewItemId() const {
  return static_cast<uint16_t>(lookId_);
}

DoodadBrush::DoodadLayout
DoodadBrush::buildPreviewTiles(const Domain::Position &anchor,
                               const Services::BrushSettingsService *brushSettings,
                               const Domain::ChunkedMap *map,
                               std::optional<uint32_t> seed) const {
  return buildPlacementLayout(anchor, brushSettings, activeVariation_, map, true,
                              seed);
}

DoodadBrush::DoodadLayout
DoodadBrush::buildPlacementLayout(const Domain::Position &center,
                                  const Services::BrushSettingsService *brushSettings,
                                  size_t preferredVariation,
                                  const Domain::ChunkedMap *map,
                                  bool forcePlace,
                                  std::optional<uint32_t> seed) const {
  return buildPlacementPlan(center, brushSettings, preferredVariation, map,
                            forcePlace, seed)
      .layout;
}

DoodadBrush::PlacementPlan
DoodadBrush::buildPlacementPlan(const Domain::Position &center,
                                const Services::BrushSettingsService *brushSettings,
                                size_t preferredVariation,
                                const Domain::ChunkedMap *map,
                                bool forcePlace,
                                std::optional<uint32_t> seed) const {
  return DoodadPlacementPlanner::buildPlan(
      {.brush = *this,
       .center = center,
       .brushSettings = brushSettings,
       .preferredVariation = preferredVariation,
       .map = map,
       .forcePlace = forcePlace,
       .seed = seed});
}

std::vector<Domain::Position>
DoodadBrush::getPlacementPositions(const Domain::Position &center,
                                   const Services::BrushSettingsService *brushSettings,
                                   size_t preferredVariation,
                                   const Domain::ChunkedMap *map,
                                   bool forcePlace,
                                   std::optional<uint32_t> seed) const {
  std::vector<Domain::Position> positions;
  std::unordered_set<int64_t> uniquePositions;

  for (const auto &tile :
       buildPlacementLayout(center, brushSettings, preferredVariation, map,
                            forcePlace, seed)) {
    const Domain::Position absolutePosition(
        center.x + tile.relativePosition.x, center.y + tile.relativePosition.y,
        static_cast<int16_t>(center.z + tile.relativePosition.z));
    if (!uniquePositions.insert(encodeDoodadPosition(absolutePosition)).second) {
      continue;
    }

    positions.push_back(absolutePosition);
  }

  return positions;
}

DoodadBrush::ErasePlan
DoodadBrush::buildErasePlan(const Domain::Position &center,
                            const Services::BrushSettingsService *brushSettings,
                            size_t preferredVariation,
                            const Domain::ChunkedMap *map, bool forcePlace,
                            std::optional<uint32_t> seed,
                            EraseOptions options) const {
  ErasePlan plan;
  if (!map) {
    return plan;
  }

  auto rawLayout = DoodadPlacementPlanner::generateRawStamp(
      *this, brushSettings, preferredVariation, seed);
  std::vector<Domain::Position> positions;
  positions.reserve(rawLayout.size());
  for (const auto &tile : rawLayout) {
    positions.push_back({
        center.x + tile.relativePosition.x,
        center.y + tile.relativePosition.y,
        static_cast<int16_t>(center.z + tile.relativePosition.z)
    });
  }
  if (positions.empty()) {
    positions.push_back(center);
  }
  positions = Utils::dedupeAndSortPositions(std::move(positions));

  plan.positions = positions;

  for (const auto &position : positions) {
    const auto *tile = map->getTile(position);
    if (!tile) {
      continue;
    }

    DoodadRedoBorderTouch redoTouch{.position = position};
    bool removesAny = false;

    auto groundOptions = options;
    groundOptions.preserveComplexItems = false;
    if (tile->getGround() &&
        shouldEraseDoodadItem(tile->getGround(), groundOptions)) {
      redoTouch.placedGround = true;
      removesAny = true;
    }

    for (const auto &item : tile->getItems()) {
      if (!shouldEraseDoodadItem(item.get(), options)) {
        continue;
      }
      if (const auto *itemType = item->getType();
          itemType && itemType->is_wall) {
        redoTouch.placedWall = true;
      }
      removesAny = true;
    }

    if (removesAny && redoBorders_) {
      plan.redoTouches.push_back(redoTouch);
    }
  }

  plan.affectedPositions = plan.positions;
  if (redoBorders_) {
    for (const auto &position :
         buildDoodadRedoBorderPositions(plan.redoTouches)) {
      plan.affectedPositions.push_back(position);
    }
  }
  plan.affectedPositions = Utils::dedupeAndSortPositions(std::move(plan.affectedPositions));
  return plan;
}

void DoodadBrush::applyPlacementLayout(Domain::ChunkedMap &map,
                                       const Domain::Position &center,
                                       const DoodadLayout &layout,
                                       const DrawContext &ctx) const {
  applyPlacementPlan(map, center, {.layout = layout}, ctx);
}

void DoodadBrush::applyPlacementPlan(Domain::ChunkedMap &map,
                                     const Domain::Position &center,
                                     const PlacementPlan &plan,
                                     const DrawContext &ctx) const {
  std::vector<DoodadRedoBorderTouch> redoTouches;
  if (redoBorders_) {
    redoTouches = plan.redoTouches;
  }
  const bool hasPlannedRedoTouches = !redoTouches.empty();
  std::unordered_map<Domain::Position, size_t> fallbackRedoTouchIndexes;

  for (const auto &layoutTile : plan.layout) {
    const Domain::Position absolutePosition(
        center.x + layoutTile.relativePosition.x,
        center.y + layoutTile.relativePosition.y,
        static_cast<int16_t>(center.z + layoutTile.relativePosition.z));
    auto *targetTile = map.getOrCreateTile(absolutePosition);
    if (!targetTile) {
      continue;
    }

    if (removeOptionalBorder_ && targetTile->hasOptionalBorder()) {
      targetTile->setOptionalBorder(false);
      targetTile->markDirty();
    }

    for (const auto &previewItem : layoutTile.items) {
      if (previewItem.itemId == 0) {
        continue;
      }

      auto item = Types::createTypedItem(ctx, static_cast<uint16_t>(previewItem.itemId),
                                         previewItem.subtype);
      if (!item) {
        continue;
      }

      const auto *itemType = item->getType();
      if (redoBorders_ && !hasPlannedRedoTouches) {
        recordRedoBorderTouch(redoTouches, fallbackRedoTouchIndexes,
                              absolutePosition, itemType);
      }

      if (itemType && itemType->isGround()) {
        targetTile->setGround(std::move(item));
      } else {
        if (itemType && itemType->is_wall) {
          const auto serverId = item->getServerId();
          const auto ownerBrushId = item->getOwnerBrushId();
          targetTile->removeItemsIf([serverId, ownerBrushId](const Domain::Item *existing) {
            if (!existing) {
              return false;
            }
            const auto *existingType = existing->getType();
            if (!existingType || !existingType->is_wall) {
              return false;
            }
            if (existing->getServerId() == serverId) {
              return true;
            }
            return ownerBrushId != InvalidBrushId &&
                   existing->getOwnerBrushId() == ownerBrushId;
          });
        }
        targetTile->addItem(std::move(item));
      }
    }
  }

  if (!redoBorders_) {
    return;
  }

  for (const auto &position : buildDoodadRedoBorderPositions(redoTouches)) {
    rebuildBrushesAtPosition(map, registry_, position);
  }
}

const DoodadAlternative *DoodadBrush::selectAlternative(size_t preferredIndex) const {
  if (alternatives_.empty()) {
    return nullptr;
  }
  if (preferredIndex < alternatives_.size()) {
    return &alternatives_[preferredIndex];
  }

  const auto index =
      WeightedSelection::randomRange(registry_.getRng(), 0, static_cast<uint32_t>(alternatives_.size() - 1));
  return &alternatives_[index];
}

bool DoodadBrush::tileHasOwnItem(const Domain::Tile *tile) const {
  if (!tile) {
    return false;
  }
  if (tile->getGround() && ownsItem(tile->getGround())) {
    return true;
  }
  for (const auto &item : tile->getItems()) {
    if (item && ownsItem(item.get())) {
      return true;
    }
  }
  return false;
}

} // namespace MapEditor::Brushes
