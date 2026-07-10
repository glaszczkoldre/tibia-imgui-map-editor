#include "BrushController.h"
#include "BrushRegistry.h"
#include "Domain/Item.h"
#include "Domain/Tile.h"
#include "Types/GroundBrush.h"
#include "Types/WallBrush.h"
#include "Types/DoorBrush.h"
#include "Types/OptionalBorderBrush.h"
#include "Types/RawBrush.h"
#include "Services/ClientDataService.h"
#include "Services/Autoborder/AutoborderEngine.h"
#include <algorithm>

namespace MapEditor::Brushes {

namespace {

bool matchesBrushType(const IBrush *brush, BrushType type) {
  if (!brush) {
    return false;
  }

  const auto brushType = brush->getType();
  if (type == BrushType::Wall) {
    return brushType == BrushType::Wall ||
           brushType == BrushType::WallDecoration;
  }

  return brushType == type;
}

const Domain::Item *getTopTileItem(const Domain::Tile &tile) {
  if (tile.getItemCount() == 0) {
    return nullptr;
  }
  return tile.getItem(tile.getItemCount() - 1);
}

IBrush *findItemBrushByType(const Domain::Tile &tile, BrushRegistry *registry,
                            BrushType type, bool collectionOnly = false) {
  if (!registry) {
    return nullptr;
  }

  for (size_t index = tile.getItemCount(); index > 0; --index) {
    const auto *item = tile.getItem(index - 1);
    if (!item) {
      continue;
    }

    for (auto *brush : registry->getBrushesForItem(item->getServerId())) {
      if (!matchesBrushType(brush, type)) {
        continue;
      }
      if (collectionOnly && (!brush->visibleInPalette() || !brush->hasCollection())) {
        continue;
      }
      if (brush) {
        return brush;
      }
    }
  }

  return nullptr;
}

IBrush *findOwnedItemBrushByType(const Domain::Tile &tile, BrushRegistry *registry,
                                 BrushType type, bool collectionOnly = false) {
  if (!registry) {
    return nullptr;
  }

  for (size_t index = tile.getItemCount(); index > 0; --index) {
    const auto *item = tile.getItem(index - 1);
    if (!item || item->getOwnerBrushId() == InvalidBrushId) {
      continue;
    }

    if (auto *brush = registry->getBrushById(item->getOwnerBrushId());
        matchesBrushType(brush, type)) {
      if (!collectionOnly || (brush->visibleInPalette() && brush->hasCollection())) {
        return brush;
      }
    }
  }

  if (const auto *ground = tile.getGround();
      ground && ground->getOwnerBrushId() != InvalidBrushId) {
    if (auto *brush = registry->getBrushById(ground->getOwnerBrushId());
        matchesBrushType(brush, type)) {
      if (!collectionOnly || (brush->visibleInPalette() && brush->hasCollection())) {
        return brush;
      }
    }
  }

  return nullptr;
}

DoorType getDoorTypeForTile(const Domain::Tile &tile, BrushRegistry *registry) {
  if (!registry) {
    return DoorType::Undefined;
  }

  for (size_t index = tile.getItemCount(); index > 0; --index) {
    const auto *item = tile.getItem(index - 1);
    if (!item) {
      continue;
    }

    for (auto *brush : registry->getBrushesForItem(item->getServerId())) {
      auto *wallBrush = dynamic_cast<WallBrush *>(brush);
      if (!wallBrush) {
        continue;
      }

      if (auto door = wallBrush->findDoorForItem(item->getServerId())) {
        return door->type;
      }
    }
  }

  return DoorType::Undefined;
}

DoorType getDoorTypeForItem(const Domain::Item &item, BrushRegistry *registry) {
  if (!registry) {
    return DoorType::Undefined;
  }

  for (auto *brush : registry->getBrushesForItem(item.getServerId())) {
    auto *wallBrush = dynamic_cast<WallBrush *>(brush);
    if (!wallBrush) {
      continue;
    }

    if (auto door = wallBrush->findDoorForItem(item.getServerId())) {
      return door->type;
    }
  }

  return DoorType::Undefined;
}

std::optional<ResolvedBrushSelection> chooseFirst(std::optional<ResolvedBrushSelection> first,
                                                  std::optional<ResolvedBrushSelection> second) {
  return first ? std::move(first) : std::move(second);
}

} // namespace

std::optional<ResolvedBrushSelection> BrushController::makeSelection(
    const IBrush *brush, BrushPickMode selectionMode, std::string displayName) const {
  if (!brush) {
    return std::nullopt;
  }

  if (displayName.empty()) {
    displayName = brush->getName();
  }

  return ResolvedBrushSelection{
      .brush = const_cast<IBrush*>(brush),
      .mode = selectionMode,
      .displayName = std::move(displayName)
  };
}

std::optional<ResolvedBrushSelection> BrushController::selectFlagBrush(
    Domain::TileFlag flag) const {
  switch (flag) {
  case Domain::TileFlag::ProtectionZone:
    return makeSelection(&pzBrush_, BrushPickMode::ProtectionZone);
  case Domain::TileFlag::NoPvp:
    return makeSelection(&noPvpBrush_, BrushPickMode::NoPvp);
  case Domain::TileFlag::NoLogout:
    return makeSelection(&noLogoutBrush_, BrushPickMode::NoLogout);
  case Domain::TileFlag::PvpZone:
    return makeSelection(&pvpZoneBrush_, BrushPickMode::PvpZone);
  case Domain::TileFlag::Refresh:
  case Domain::TileFlag::None:
    return std::nullopt;
  }
  return std::nullopt;
}

std::optional<ResolvedBrushSelection> BrushController::selectBrushById(
    BrushId brushId, BrushPickMode selectionMode) const {
  if (!registry_ || brushId == InvalidBrushId) {
    return std::nullopt;
  }
  return makeSelection(registry_->getBrushById(brushId), selectionMode);
}

std::optional<ResolvedBrushSelection> BrushController::selectPreferredBrushByType(
    BrushType type, BrushPickMode selectionMode, const Domain::Item *preferredItem) const {
  if (!preferredItem || !registry_) {
    return std::nullopt;
  }

  if (auto selection =
          selectBrushById(preferredItem->getOwnerBrushId(), selectionMode);
      selection && matchesBrushType(selection->brush, type)) {
    return selection;
  }

  for (auto *brush : registry_->getBrushesForItem(preferredItem->getServerId())) {
    if (matchesBrushType(brush, type)) {
      return makeSelection(brush, selectionMode);
    }
  }

  return std::nullopt;
}

std::optional<ResolvedBrushSelection> BrushController::selectDoorBrush(
    const Domain::Tile &tile, const Domain::Item *preferredItem) const {
  if (preferredItem) {
    if (auto *doorBrush =
            getDoorBrushForType(getDoorTypeForItem(*preferredItem, registry_))) {
      return makeSelection(doorBrush, BrushPickMode::Door);
    }
  }

  for (size_t index = tile.getItemCount(); index > 0; --index) {
    const auto *item = tile.getItem(index - 1);
    if (item) {
      if (auto selection =
              selectBrushById(item->getOwnerBrushId(), BrushPickMode::Door);
          selection && selection->brush &&
          selection->brush->getType() == BrushType::Door) {
        return selection;
      }
    }
  }

  if (auto *doorBrush = getDoorBrushForType(getDoorTypeForTile(tile, registry_))) {
    return makeSelection(doorBrush, BrushPickMode::Door);
  }

  return std::nullopt;
}

std::optional<ResolvedBrushSelection> BrushController::selectGroundBrush(
    const Domain::Tile &tile) const {
  if (!registry_ || !tile.hasGround()) {
    return std::nullopt;
  }

  if (auto selection =
          selectBrushById(tile.getGroundBrushId(), BrushPickMode::Ground)) {
    return selection;
  }

  for (auto *brush : registry_->getBrushesForItem(tile.getGround()->getServerId())) {
    if (brush && brush->getType() == BrushType::Ground) {
      return makeSelection(brush, BrushPickMode::Ground);
    }
  }

  return std::nullopt;
}

std::optional<ResolvedBrushSelection> BrushController::selectRawBrush(
    const Domain::Tile &tile, const Domain::Item *preferredItem) const {
  if (preferredItem) {
    return ResolvedBrushSelection{
        .mode = BrushPickMode::Raw,
        .displayName = "RAW Item",
        .rawItemId = preferredItem->getServerId(),
    };
  }

  if (const auto *item = getTopTileItem(tile)) {
    return ResolvedBrushSelection{
        .mode = BrushPickMode::Raw,
        .displayName = "RAW Item",
        .rawItemId = item->getServerId(),
    };
  }
  if (tile.hasGround()) {
    return ResolvedBrushSelection{
        .mode = BrushPickMode::Raw,
        .displayName = "RAW Item",
        .rawItemId = tile.getGround()->getServerId(),
    };
  }
  return std::nullopt;
}

std::optional<ResolvedBrushSelection> BrushController::selectHouseExitBrush(
    const Domain::Tile &tile) const {
  auto resolveHouseId = [this, &tile]() -> uint32_t {
    if (tile.getHouseExitHouseId() != 0) {
      return tile.getHouseExitHouseId();
    }

    if (tile.getHouseId() != 0) {
      return tile.getHouseId();
    }

    if (!map_) {
      return 0;
    }

    for (const auto &[houseId, house] : map_->getHouses()) {
      if (house && house->entry_position == tile.getPosition()) {
        return houseId;
      }
    }

    return 0;
  };

  if (tile.getHouseExitBrushId() != InvalidBrushId) {
    auto selection =
        selectBrushById(tile.getHouseExitBrushId(), BrushPickMode::HouseExit);
    if (selection) {
      selection->houseExitHouseId = resolveHouseId();
    }
    return selection;
  }

  if (const auto houseId = resolveHouseId(); houseId != 0) {
    auto selection = makeSelection(&houseExitBrush_, BrushPickMode::HouseExit);
    if (selection) {
      selection->houseExitHouseId = houseId;
    }
    return selection;
  }

  return std::nullopt;
}

std::optional<ResolvedBrushSelection> BrushController::selectWaypointBrush(
    const Domain::Tile &tile) const {
  if (tile.getWaypointBrushId() != InvalidBrushId) {
    auto selection =
        selectBrushById(tile.getWaypointBrushId(), BrushPickMode::Waypoint);
    if (selection) {
      if (const auto *waypoint = map_->getWaypointAt(tile.getPosition())) {
        selection->waypointName = waypoint->name;
      }
    }
    return selection;
  }
  if (const auto *waypoint = map_->getWaypointAt(tile.getPosition())) {
    auto selection = makeSelection(&waypointBrush_, BrushPickMode::Waypoint);
    if (selection) {
      selection->waypointName = waypoint->name;
    }
    return selection;
  }
  return std::nullopt;
}

std::optional<ResolvedBrushSelection> BrushController::selectOptionalBorderBrush(
    const Domain::Tile &tile) const {
  if (auto selection = selectBrushById(tile.getOptionalBorderBrushId(),
                                       BrushPickMode::OptionalBorder)) {
    return selection;
  }

  if (!tile.hasOptionalBorder() || !tile.hasGround() || !registry_) {
    return std::nullopt;
  }

  for (auto *brush : registry_->getBrushesForItem(tile.getGround()->getServerId())) {
    auto *groundBrush = dynamic_cast<GroundBrush *>(brush);
    if (groundBrush && groundBrush->hasOptionalBorderRule()) {
      return makeSelection(&optionalBorderBrush_,
                           BrushPickMode::OptionalBorder);
    }
  }
  return std::nullopt;
}

std::optional<ResolvedBrushSelection> BrushController::selectCollectionBrush(
    const Domain::Tile &tile, const Domain::Item *preferredItem) const {
  if (auto selection = selectPreferredBrushByType(BrushType::Wall,
                                                  BrushPickMode::Collection,
                                                  preferredItem);
      selection && selection->brush && selection->brush->visibleInPalette() &&
      selection->brush->hasCollection()) {
    return selection;
  }
  if (auto selection = selectPreferredBrushByType(BrushType::Table,
                                                  BrushPickMode::Collection,
                                                  preferredItem);
      selection && selection->brush && selection->brush->visibleInPalette() &&
      selection->brush->hasCollection()) {
    return selection;
  }
  if (auto selection = selectPreferredBrushByType(BrushType::Carpet,
                                                  BrushPickMode::Collection,
                                                  preferredItem);
      selection && selection->brush && selection->brush->visibleInPalette() &&
      selection->brush->hasCollection()) {
    return selection;
  }
  if (auto selection = selectPreferredBrushByType(BrushType::Doodad,
                                                  BrushPickMode::Collection,
                                                  preferredItem);
      selection && selection->brush && selection->brush->visibleInPalette() &&
      selection->brush->hasCollection()) {
    return selection;
  }
  if (auto selection = selectPreferredBrushByType(BrushType::Raw,
                                                  BrushPickMode::Collection,
                                                  preferredItem);
      selection && selection->brush && selection->brush->visibleInPalette() &&
      selection->brush->hasCollection()) {
    return selection;
  }

  const auto selectCollectionType =
      [&](BrushType type) -> std::optional<ResolvedBrushSelection> {
    if (auto *brush =
            findOwnedItemBrushByType(tile, registry_, type, true)) {
      return makeSelection(brush, BrushPickMode::Collection);
    }
    if (auto *brush = findItemBrushByType(tile, registry_, type, true)) {
      return makeSelection(brush, BrushPickMode::Collection);
    }
    return std::nullopt;
  };

  for (const auto type : {BrushType::Wall, BrushType::Table, BrushType::Carpet,
                          BrushType::Doodad, BrushType::Raw}) {
    if (auto selection = selectCollectionType(type)) {
      return selection;
    }
  }

  if (auto selection = selectGroundBrush(tile)) {
    if (selection->brush && selection->brush->visibleInPalette() &&
        selection->brush->hasCollection()) {
      selection->mode = BrushPickMode::Collection;
      return selection;
    }
  }

  return std::nullopt;
}

std::optional<ResolvedBrushSelection> BrushController::selectSpecificMode(
    const Domain::Tile &tile, BrushPickMode pickMode, const Domain::Item *preferredItem) const {
  switch (pickMode) {
  case BrushPickMode::Smart:
    return std::nullopt;
  case BrushPickMode::Raw:
    return selectRawBrush(tile, preferredItem);
  case BrushPickMode::Ground:
    return selectGroundBrush(tile);
  case BrushPickMode::Doodad:
    return chooseFirst(
        selectPreferredBrushByType(BrushType::Doodad,
                                   BrushPickMode::Doodad,
                                   preferredItem),
        chooseFirst(
        makeSelection(
            findOwnedItemBrushByType(tile, registry_, BrushType::Doodad),
            BrushPickMode::Doodad),
        makeSelection(
            findItemBrushByType(tile, registry_, BrushType::Doodad),
            BrushPickMode::Doodad)));
  case BrushPickMode::Collection:
    return selectCollectionBrush(tile, preferredItem);
  case BrushPickMode::Door:
    return selectDoorBrush(tile, preferredItem);
  case BrushPickMode::Wall:
    return chooseFirst(
        selectPreferredBrushByType(BrushType::Wall, BrushPickMode::Wall, preferredItem),
        chooseFirst(
        makeSelection(
            findOwnedItemBrushByType(tile, registry_, BrushType::Wall),
            BrushPickMode::Wall),
        makeSelection(
            findItemBrushByType(tile, registry_, BrushType::Wall),
            BrushPickMode::Wall)));
  case BrushPickMode::Carpet:
    return chooseFirst(
        selectPreferredBrushByType(BrushType::Carpet,
                                   BrushPickMode::Carpet,
                                   preferredItem),
        chooseFirst(
        makeSelection(
            findOwnedItemBrushByType(tile, registry_, BrushType::Carpet),
            BrushPickMode::Carpet),
        makeSelection(
            findItemBrushByType(tile, registry_, BrushType::Carpet),
            BrushPickMode::Carpet)));
  case BrushPickMode::Table:
    return chooseFirst(
        selectPreferredBrushByType(BrushType::Table, BrushPickMode::Table, preferredItem),
        chooseFirst(
        makeSelection(
            findOwnedItemBrushByType(tile, registry_, BrushType::Table),
            BrushPickMode::Table),
        makeSelection(
            findItemBrushByType(tile, registry_, BrushType::Table),
            BrushPickMode::Table)));
  case BrushPickMode::Creature:
    if (tile.getCreatureBrushId() != InvalidBrushId) {
      return selectBrushById(tile.getCreatureBrushId(),
                             BrushPickMode::Creature);
    }
    if (!registry_ || !tile.hasCreature()) {
      return std::nullopt;
    }
    return makeSelection(
        registry_->getBrushForCreature(tile.getCreature()->name),
        BrushPickMode::Creature);
  case BrushPickMode::Spawn:
    if (tile.getSpawnBrushId() != InvalidBrushId) {
      return selectBrushById(tile.getSpawnBrushId(), BrushPickMode::Spawn);
    }
    if (!tile.hasSpawn()) {
      return std::nullopt;
    }
    return makeSelection(&spawnBrush_, BrushPickMode::Spawn);
  case BrushPickMode::House:
    if (tile.getHouseBrushId() != InvalidBrushId) {
      auto selection =
          selectBrushById(tile.getHouseBrushId(), BrushPickMode::House);
      if (selection) {
        selection->houseId = tile.getHouseId();
      }
      return selection;
    }
    if (!tile.isHouseTile()) {
      return std::nullopt;
    }
    if (auto selection = makeSelection(&houseBrush_, BrushPickMode::House)) {
      selection->houseId = tile.getHouseId();
      return selection;
    }
    return std::nullopt;
  case BrushPickMode::HouseExit:
    return selectHouseExitBrush(tile);
  case BrushPickMode::Waypoint:
    return selectWaypointBrush(tile);
  case BrushPickMode::OptionalBorder:
    return selectOptionalBorderBrush(tile);
  case BrushPickMode::ProtectionZone:
    return tile.hasFlag(Domain::TileFlag::ProtectionZone)
               ? chooseFirst(
                     selectBrushById(
                         tile.getZoneBrushId(
                             Domain::TileFlag::ProtectionZone),
                         BrushPickMode::ProtectionZone),
                     selectFlagBrush(Domain::TileFlag::ProtectionZone))
               : std::nullopt;
  case BrushPickMode::NoPvp:
    return tile.hasFlag(Domain::TileFlag::NoPvp)
               ? chooseFirst(
                     selectBrushById(
                         tile.getZoneBrushId(Domain::TileFlag::NoPvp),
                         BrushPickMode::NoPvp),
                     selectFlagBrush(Domain::TileFlag::NoPvp))
               : std::nullopt;
  case BrushPickMode::NoLogout:
    return tile.hasFlag(Domain::TileFlag::NoLogout)
               ? chooseFirst(
                     selectBrushById(
                         tile.getZoneBrushId(Domain::TileFlag::NoLogout),
                         BrushPickMode::NoLogout),
                     selectFlagBrush(Domain::TileFlag::NoLogout))
               : std::nullopt;
  case BrushPickMode::PvpZone:
    return tile.hasFlag(Domain::TileFlag::PvpZone)
               ? chooseFirst(
                     selectBrushById(
                         tile.getZoneBrushId(Domain::TileFlag::PvpZone),
                         BrushPickMode::PvpZone),
                     selectFlagBrush(Domain::TileFlag::PvpZone))
               : std::nullopt;
  }

  return std::nullopt;
}

std::optional<ResolvedBrushSelection>
BrushController::resolveBrushFromTile(const Domain::Tile &tile,
                                      BrushPickMode mode,
                                      const Domain::Item *preferredItem) const {
  if (!map_) {
    return std::nullopt;
  }

  if (mode != BrushPickMode::Smart) {
    return selectSpecificMode(tile, mode, preferredItem);
  }

  if (auto selection =
          selectBrushById(tile.getCreatureBrushId(), BrushPickMode::Creature)) {
    return selection;
  }

  if (tile.getSpawnBrushId() != InvalidBrushId) {
    if (auto selection =
            selectBrushById(tile.getSpawnBrushId(), BrushPickMode::Spawn)) {
      return selection;
    }
  }

  if (registry_ && tile.hasCreature()) {
    if (auto *brush = registry_->getBrushForCreature(tile.getCreature()->name)) {
      return makeSelection(brush, BrushPickMode::Creature);
    }
  }

  if (tile.hasSpawn()) {
    return makeSelection(&spawnBrush_, BrushPickMode::Spawn);
  }

  if (auto selection = selectWaypointBrush(tile)) {
    return selection;
  }

  if (auto selection = selectHouseExitBrush(tile)) {
    return selection;
  }

  if (tile.isHouseTile()) {
    if (auto selection =
            selectBrushById(tile.getHouseBrushId(), BrushPickMode::House)) {
      selection->houseId = tile.getHouseId();
      return selection;
    }
    auto selection = makeSelection(&houseBrush_, BrushPickMode::House);
    if (selection) {
      selection->houseId = tile.getHouseId();
    }
    return selection;
  }

  if (preferredItem) {
    for (const auto candidate : {BrushType::Wall, BrushType::Doodad,
                                 BrushType::Table, BrushType::Carpet}) {
      if (auto selection =
              selectPreferredBrushByType(candidate, BrushPickMode::Smart, preferredItem)) {
        return selection;
      }
    }
  }

  for (const auto candidate : {BrushType::Wall, BrushType::Doodad,
                               BrushType::Table, BrushType::Carpet}) {
    if (auto *brush = findOwnedItemBrushByType(tile, registry_, candidate)) {
      return makeSelection(brush, BrushPickMode::Smart);
    }
    if (auto *brush = findItemBrushByType(tile, registry_, candidate)) {
      return makeSelection(brush, BrushPickMode::Smart);
    }
  }

  if (auto selection = selectDoorBrush(tile, preferredItem)) {
    return selection;
  }

  if (registry_) {
    if (auto *brush = registry_->resolveBrushForTile(tile)) {
      if (brush->getType() == BrushType::Raw) {
        auto *rawBrush = static_cast<const RawBrush *>(brush);
        return ResolvedBrushSelection{
            .mode = BrushPickMode::Raw,
            .displayName = "RAW Item",
            .rawItemId = rawBrush->getItemId(),
        };
      }

      auto selection = makeSelection(brush, BrushPickMode::Smart);
      if (!selection) {
        return std::nullopt;
      }

      switch (brush->getType()) {
      case BrushType::House:
        selection->houseId = tile.getHouseId();
        break;
      case BrushType::HouseExit:
        if (tile.getHouseExitHouseId() != 0) {
          selection->houseExitHouseId = tile.getHouseExitHouseId();
        } else if (tile.getHouseId() != 0) {
          selection->houseExitHouseId = tile.getHouseId();
        }
        break;
      case BrushType::Waypoint:
        if (const auto *waypoint = map_->getWaypointAt(tile.getPosition())) {
          selection->waypointName = waypoint->name;
        }
        break;
      default:
        break;
      }

      return selection;
    }
  }

  if (auto selection = selectOptionalBorderBrush(tile)) {
    return selection;
  }

  for (const auto flag : {Domain::TileFlag::ProtectionZone,
                          Domain::TileFlag::NoPvp,
                          Domain::TileFlag::NoLogout,
                          Domain::TileFlag::PvpZone}) {
    if (tile.hasFlag(flag)) {
      if (auto selection = selectFlagBrush(flag)) {
        return selection;
      }
    }
  }

  // Fallback to RAW Brush selection for any remaining items
  if (auto selection = selectRawBrush(tile, preferredItem)) {
    return selection;
  }

  return std::nullopt;
}

} // namespace MapEditor::Brushes
