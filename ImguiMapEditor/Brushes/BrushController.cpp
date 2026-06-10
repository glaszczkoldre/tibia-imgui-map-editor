#include "BrushController.h"
#include "BrushRegistry.h"
#include "Domain/Item.h"
#include "Services/BrushSettingsService.h"
#include "Services/Autoborder/AutoborderEngine.h"
#include "Services/Autoborder/PlannedMutation.h"
#include "Services/ClientDataService.h"
#include "Services/Preview/BrushPreviewFactory.h"
#include "Services/Preview/PreviewService.h"
#include "Types/GroundBrush.h"
#include "Types/DoodadBrush.h"
#include "Types/DoodadPlacementPlanner.h"
#include "Types/WallBrush.h"
#include "Types/DoorBrush.h"
#include "Types/RawBrush.h"
#include <cmath>
#include <spdlog/spdlog.h>

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

} // namespace

BrushController::~BrushController() noexcept = default;

void BrushController::initialize(
    Domain::ChunkedMap *map, Domain::History::HistoryManager *historyManager,
    Services::ClientDataService *clientData) {
  map_ = map;
  historyManager_ = historyManager;
  clientData_ = clientData;
  spdlog::debug("[BrushController] Initialized with map, history manager, and "
                "client data");
}

void BrushController::setBrushRegistry(BrushRegistry *registry) {
  registry_ = registry;
  if (!registry_) {
    normalDoorBrush_.reset();
    normalAltDoorBrush_.reset();
    lockedDoorBrush_.reset();
    questDoorBrush_.reset();
    magicDoorBrush_.reset();
    archwayBrush_.reset();
    windowBrush_.reset();
    hatchWindowBrush_.reset();
    return;
  }

  registry_->registerExternalBrush(&spawnBrush_);
  registry_->registerExternalBrush(&pzBrush_);
  registry_->registerExternalBrush(&noPvpBrush_);
  registry_->registerExternalBrush(&noLogoutBrush_);
  registry_->registerExternalBrush(&pvpZoneBrush_);
  registry_->registerExternalBrush(&eraserBrush_);
  registry_->registerExternalBrush(&houseBrush_);
  registry_->registerExternalBrush(&houseExitBrush_);
  registry_->registerExternalBrush(&waypointBrush_);
  registry_->registerExternalBrush(&optionalBorderBrush_);

  normalDoorBrush_ = std::make_unique<DoorBrush>("Normal Door", 0,
                                                  DoorType::Normal, *registry_);
  normalAltDoorBrush_ = std::make_unique<DoorBrush>(
      "Normal Alt Door", 0, DoorType::NormalAlt, *registry_);
  lockedDoorBrush_ = std::make_unique<DoorBrush>("Locked Door", 0,
                                                  DoorType::Locked, *registry_);
  questDoorBrush_ = std::make_unique<DoorBrush>("Quest Door", 0,
                                                 DoorType::Quest, *registry_);
  magicDoorBrush_ = std::make_unique<DoorBrush>("Magic Door", 0,
                                                 DoorType::Magic, *registry_);
  archwayBrush_ = std::make_unique<DoorBrush>("Archway", 0,
                                               DoorType::Archway, *registry_);
  windowBrush_ = std::make_unique<DoorBrush>("Window", 0,
                                              DoorType::Window, *registry_);
  hatchWindowBrush_ = std::make_unique<DoorBrush>("Hatch Window", 0,
                                                   DoorType::HatchWindow,
                                                   *registry_);
  registry_->registerExternalBrush(normalDoorBrush_.get());
  registry_->registerExternalBrush(normalAltDoorBrush_.get());
  registry_->registerExternalBrush(lockedDoorBrush_.get());
  registry_->registerExternalBrush(questDoorBrush_.get());
  registry_->registerExternalBrush(magicDoorBrush_.get());
  registry_->registerExternalBrush(archwayBrush_.get());
  registry_->registerExternalBrush(windowBrush_.get());
  registry_->registerExternalBrush(hatchWindowBrush_.get());
}

void BrushController::setBrush(IBrush *brush) {
  if (!brush) {
    clearBrush();
    return;
  }

  currentBrush_ = brush;
  currentBrushName_ = brush->getName();
  currentBrush_->setVariation(static_cast<size_t>(variation_));
  lastBrushSelection_ = captureCurrentSelection();

  refreshPreviewProvider();

  if (onBrushActivated_) {
    onBrushActivated_();
  }

  spdlog::info("[BrushController] Set brush: {}", brush->getName());
}

void BrushController::refreshPreviewProvider() {
  if (!previewService_) {
    return;
  }

  if (!currentBrush_) {
    previewService_->clearPreview();
    return;
  }

  if (!previewFactory_) {
    previewService_->clearPreview();
    spdlog::warn("[BrushController] No preview factory available");
    return;
  }

  auto provider =
      previewFactory_->createProvider(currentBrush_, brushSettingsService_, map_);
  if (provider) {
    previewService_->setProvider(std::move(provider));
  } else {
    previewService_->clearPreview();
  }
}

void BrushController::clearBrush() {
  if (currentBrush_) {
    lastBrushSelection_ = captureCurrentSelection();
  }

  currentBrush_ = nullptr;
  currentBrushName_.clear();

  if (previewService_) {
    previewService_->clearPreview();
  }

  spdlog::debug("[BrushController] Brush cleared");
}

bool BrushController::restoreLastBrush() {
  return lastBrushSelection_.has_value() &&
         applyResolvedSelection(*lastBrushSelection_);
}

bool BrushController::toggleSelectionTool() {
  if (hasBrush()) {
    clearBrush();
    return true;
  }

  return restoreLastBrush();
}

void BrushController::activateSpawnBrush() {
  setBrush(&spawnBrush_);
  spdlog::info("[BrushController] Spawn brush activated");
}

std::optional<uint32_t> BrushController::getCurrentItemId() const {
  if (auto *rawBrush = dynamic_cast<RawBrush *>(currentBrush_)) {
    return rawBrush->getItemId();
  }
  return std::nullopt;
}

bool BrushController::selectBrushFromTile(const Domain::Tile &tile,
                                          BrushPickMode mode,
                                          const Domain::Item *preferredItem) {
  const auto selection = resolveBrushFromTile(tile, mode, preferredItem);
  return selection.has_value() && applyResolvedSelection(*selection);
}

std::optional<ResolvedBrushSelection>
BrushController::resolveBrushFromTile(const Domain::Tile &tile,
                                      BrushPickMode mode,
                                      const Domain::Item *preferredItem) {
  if (!map_) {
    return std::nullopt;
  }

  auto makeSelection = [](IBrush *brush, BrushPickMode selectionMode,
                          std::string displayName = {}) {
    if (!brush) {
      return std::optional<ResolvedBrushSelection>{};
    }

    if (displayName.empty()) {
      displayName = brush->getName();
    }

    return std::optional<ResolvedBrushSelection>{
        ResolvedBrushSelection{.brush = brush,
                               .mode = selectionMode,
                               .displayName = std::move(displayName)}};
  };

  auto selectFlagBrush = [this, &makeSelection](Domain::TileFlag flag)
      -> std::optional<ResolvedBrushSelection> {
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
  };

  auto selectBrushById =
      [this, &makeSelection](BrushId brushId,
                                BrushPickMode selectionMode)
      -> std::optional<ResolvedBrushSelection> {
    if (!registry_ || brushId == InvalidBrushId) {
      return std::nullopt;
    }
    return makeSelection(registry_->getBrushById(brushId), selectionMode);
  };

  auto selectPreferredBrushByType =
      [this, preferredItem, &selectBrushById, &makeSelection](
          BrushType type, BrushPickMode selectionMode)
      -> std::optional<ResolvedBrushSelection> {
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
  };

  auto selectDoorBrush = [this, &tile, preferredItem, &makeSelection,
                          &selectBrushById]()
      -> std::optional<ResolvedBrushSelection> {
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
  };

  auto selectGroundBrush = [this, &tile, &selectBrushById, &makeSelection]()
      -> std::optional<ResolvedBrushSelection> {
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
  };

  auto selectRawBrush = [this, &tile, preferredItem]()
      -> std::optional<ResolvedBrushSelection> {
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
  };

  auto selectHouseExitBrush = [this, &tile, &makeSelection,
                               &selectBrushById]()
      -> std::optional<ResolvedBrushSelection> {
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
  };

  auto selectWaypointBrush = [this, &tile, &makeSelection,
                              &selectBrushById]()
      -> std::optional<ResolvedBrushSelection> {
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
  };

  auto selectOptionalBorderBrush = [this, &tile, &makeSelection,
                                    &selectBrushById]()
      -> std::optional<ResolvedBrushSelection> {
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
  };

  auto selectCollectionBrush = [this, &tile, &makeSelection,
                                &selectGroundBrush, &selectPreferredBrushByType]()
      -> std::optional<ResolvedBrushSelection> {
    if (auto selection = selectPreferredBrushByType(BrushType::Wall,
                                                    BrushPickMode::Collection);
        selection && selection->brush && selection->brush->visibleInPalette() &&
        selection->brush->hasCollection()) {
      return selection;
    }
    if (auto selection = selectPreferredBrushByType(BrushType::Table,
                                                    BrushPickMode::Collection);
        selection && selection->brush && selection->brush->visibleInPalette() &&
        selection->brush->hasCollection()) {
      return selection;
    }
    if (auto selection = selectPreferredBrushByType(BrushType::Carpet,
                                                    BrushPickMode::Collection);
        selection && selection->brush && selection->brush->visibleInPalette() &&
        selection->brush->hasCollection()) {
      return selection;
    }
    if (auto selection = selectPreferredBrushByType(BrushType::Doodad,
                                                    BrushPickMode::Collection);
        selection && selection->brush && selection->brush->visibleInPalette() &&
        selection->brush->hasCollection()) {
      return selection;
    }
    if (auto selection = selectPreferredBrushByType(BrushType::Raw,
                                                    BrushPickMode::Collection);
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

    if (auto selection = selectGroundBrush()) {
      if (selection->brush && selection->brush->visibleInPalette() &&
          selection->brush->hasCollection()) {
        selection->mode = BrushPickMode::Collection;
        return selection;
      }
    }

    return std::nullopt;
  };

  auto chooseFirst = [](std::optional<ResolvedBrushSelection> first,
                        std::optional<ResolvedBrushSelection> second)
      -> std::optional<ResolvedBrushSelection> {
    return first ? std::move(first) : std::move(second);
  };

  auto selectSpecificMode = [this, &tile, &selectGroundBrush,
                             &selectDoorBrush, &selectRawBrush,
                             &selectWaypointBrush, &selectHouseExitBrush,
                             &selectOptionalBorderBrush, &selectFlagBrush,
                             &selectCollectionBrush, &selectBrushById,
                             &selectPreferredBrushByType, &makeSelection,
                             &chooseFirst](BrushPickMode pickMode)
      -> std::optional<ResolvedBrushSelection> {
    switch (pickMode) {
    case BrushPickMode::Smart:
      return std::nullopt;
    case BrushPickMode::Raw:
      return selectRawBrush();
    case BrushPickMode::Ground:
      return selectGroundBrush();
    case BrushPickMode::Doodad:
      return chooseFirst(
          selectPreferredBrushByType(BrushType::Doodad,
                                     BrushPickMode::Doodad),
          chooseFirst(
          makeSelection(
              findOwnedItemBrushByType(tile, registry_, BrushType::Doodad),
              BrushPickMode::Doodad),
          makeSelection(
              findItemBrushByType(tile, registry_, BrushType::Doodad),
              BrushPickMode::Doodad)));
    case BrushPickMode::Collection:
      return selectCollectionBrush();
    case BrushPickMode::Door:
      return selectDoorBrush();
    case BrushPickMode::Wall:
      return chooseFirst(
          selectPreferredBrushByType(BrushType::Wall, BrushPickMode::Wall),
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
                                     BrushPickMode::Carpet),
          chooseFirst(
          makeSelection(
              findOwnedItemBrushByType(tile, registry_, BrushType::Carpet),
              BrushPickMode::Carpet),
          makeSelection(
              findItemBrushByType(tile, registry_, BrushType::Carpet),
              BrushPickMode::Carpet)));
    case BrushPickMode::Table:
      return chooseFirst(
          selectPreferredBrushByType(BrushType::Table, BrushPickMode::Table),
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
      return selectHouseExitBrush();
    case BrushPickMode::Waypoint:
      return selectWaypointBrush();
    case BrushPickMode::OptionalBorder:
      return selectOptionalBorderBrush();
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
  };

  if (mode != BrushPickMode::Smart) {
    return selectSpecificMode(mode);
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

  if (auto selection = selectWaypointBrush()) {
    return selection;
  }

  if (auto selection = selectHouseExitBrush()) {
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

  if (auto selection = selectDoorBrush()) {
    return selection;
  }

  if (preferredItem) {
    for (const auto candidate : {BrushType::Doodad, BrushType::Wall,
                                 BrushType::Table, BrushType::Carpet}) {
      if (auto selection =
              selectPreferredBrushByType(candidate, BrushPickMode::Smart)) {
        return selection;
      }
    }
  }

  for (const auto candidate : {BrushType::Doodad, BrushType::Wall,
                               BrushType::Table, BrushType::Carpet}) {
    if (auto *brush = findOwnedItemBrushByType(tile, registry_, candidate)) {
      return makeSelection(brush, BrushPickMode::Smart);
    }
    if (auto *brush = findItemBrushByType(tile, registry_, candidate)) {
      return makeSelection(brush, BrushPickMode::Smart);
    }
  }

  if (registry_) {
    if (auto *brush = registry_->resolveBrushForTile(tile)) {
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

  if (auto selection = selectOptionalBorderBrush()) {
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

  return std::nullopt;
}

bool BrushController::applyResolvedSelection(
    const ResolvedBrushSelection &selection) {
  if (selection.rawItemId.has_value()) {
    if (!registry_) {
      return false;
    }

    if (auto *rawBrush = registry_->getOrCreateRAWBrush(*selection.rawItemId)) {
      variation_ = 0;
      rawBrush->setVariation(0);
      setBrush(rawBrush);
      return true;
    }
  }

  if (!selection.brush) {
    return false;
  }

  if (selection.brush == &houseBrush_ && selection.houseId.has_value()) {
    houseBrush_.setHouseId(*selection.houseId);
  }

  if (selection.brush == &houseExitBrush_ &&
      selection.houseExitHouseId.has_value()) {
    houseExitBrush_.setHouseId(*selection.houseExitHouseId);
  }

  if (selection.brush == &waypointBrush_ && selection.waypointName.has_value()) {
    waypointBrush_.setWaypointName(*selection.waypointName);
  }

  variation_ = std::max(0, selection.variation);
  if (selection.brush) {
    selection.brush->setVariation(static_cast<size_t>(variation_));
  }

  setBrush(selection.brush);
  return true;
}

ResolvedBrushSelection BrushController::captureCurrentSelection() const {
  ResolvedBrushSelection selection;
  selection.brush = currentBrush_;
  selection.displayName = currentBrush_ ? currentBrush_->getName() : std::string{};
  selection.variation = variation_;

  if (!currentBrush_) {
    return selection;
  }

  if (const auto *rawBrush = dynamic_cast<const RawBrush *>(currentBrush_)) {
    selection.rawItemId = rawBrush->getItemId();
  }

  if (currentBrush_ == &houseBrush_) {
    selection.houseId = houseBrush_.getHouseId();
  }

  if (currentBrush_ == &houseExitBrush_) {
    selection.houseExitHouseId = houseExitBrush_.getHouseId();
  }

  if (currentBrush_ == &waypointBrush_) {
    selection.waypointName = waypointBrush_.getWaypointName();
  }

  return selection;
}

} // namespace MapEditor::Brushes
