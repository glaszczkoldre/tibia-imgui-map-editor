#include "Brushes/BrushController.h"
#include "Brushes/BrushRegistry.h"
#include "Brushes/Core/IBrush.h"
#include "Brushes/Types/CarpetBrush.h"
#include "Brushes/Types/DoodadBrush.h"
#include "Brushes/Types/DoodadPlacementPlanner.h"
#include "Brushes/Types/DoodadRedoBorderPlanner.h"
#include "Brushes/Types/TableBrush.h"
#include "Brushes/Types/WallBrush.h"
#include "Domain/ChunkedMap.h"
#include "Domain/History/HistoryManager.h"
#include "Domain/Item.h"
#include "Domain/ItemType.h"
#include "Services/BrushSettingsService.h"
#include "Services/ConfigService.h"
#include "Services/Preview/BrushPreviewFactory.h"
#include "Services/Preview/IPreviewProvider.h"
#include "Services/TilesetService.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

namespace {

using BrushType = MapEditor::Brushes::BrushType;
using BrushPickMode = MapEditor::Brushes::BrushPickMode;

void require(bool condition, std::string_view message) {
  if (!condition) {
    throw std::runtime_error(std::string(message));
  }
}

MapEditor::Brushes::IBrush *
findBrush(MapEditor::Brushes::BrushRegistry &registry, std::string_view name,
          BrushType expectedType) {
  auto *brush = registry.getBrush(std::string(name));
  require(brush != nullptr, std::string(name).append(" brush missing"));
  require(brush->getType() == expectedType,
          std::string(name).append(" brush has unexpected type"));
  return brush;
}

bool hasPreviewTiles(MapEditor::Services::Preview::BrushPreviewFactory &factory,
                     const MapEditor::Brushes::IBrush *brush,
                     MapEditor::Services::BrushSettingsService &settings,
                     const MapEditor::Domain::ChunkedMap *map,
                     const MapEditor::Domain::Position &position) {
  auto provider = factory.createProvider(brush, &settings, map);
  require(provider != nullptr && provider->isActive(),
          "doodad preview provider was not created");
  provider->updateCursorPosition(position);
  return !provider->getTiles().empty();
}

MapEditor::Brushes::DoodadBrush::DoodadLayout
buildProviderPreviewTiles(
    MapEditor::Services::Preview::BrushPreviewFactory &factory,
    const MapEditor::Brushes::IBrush *brush,
    MapEditor::Services::BrushSettingsService &settings,
    const MapEditor::Domain::ChunkedMap *map,
    const MapEditor::Domain::Position &position) {
  auto provider = factory.createProvider(brush, &settings, map);
  require(provider != nullptr && provider->isActive(),
          "doodad preview provider was not created");
  provider->updateCursorPosition(position);
  return provider->getTiles();
}

bool layoutsEqual(const MapEditor::Brushes::DoodadBrush::DoodadLayout &lhs,
                  const MapEditor::Brushes::DoodadBrush::DoodadLayout &rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (size_t tileIndex = 0; tileIndex < lhs.size(); ++tileIndex) {
    if (lhs[tileIndex].relativePosition != rhs[tileIndex].relativePosition ||
        lhs[tileIndex].items.size() != rhs[tileIndex].items.size()) {
      return false;
    }

    for (size_t itemIndex = 0; itemIndex < lhs[tileIndex].items.size();
         ++itemIndex) {
      const auto &left = lhs[tileIndex].items[itemIndex];
      const auto &right = rhs[tileIndex].items[itemIndex];
      if (left.itemId != right.itemId || left.subtype != right.subtype) {
        return false;
      }
    }
  }

  return true;
}

size_t countLayoutTiles(
    const MapEditor::Brushes::DoodadBrush::DoodadLayout &layout) {
  size_t count = 0;
  for (const auto &tile : layout) {
    if (!tile.items.empty()) {
      ++count;
    }
  }
  return count;
}

bool containsPosition(const std::vector<MapEditor::Domain::Position> &positions,
                      const MapEditor::Domain::Position &position) {
  return std::find(positions.begin(), positions.end(), position) !=
         positions.end();
}

bool hasPlacementSkip(
    const MapEditor::Brushes::DoodadBrush::PlacementPlan &plan,
    const MapEditor::Domain::Position &position,
    MapEditor::Brushes::DoodadBrush::PlacementSkipReason reason) {
  return std::ranges::any_of(plan.skipped, [&](const auto &skip) {
    return skip.position == position && skip.reason == reason;
  });
}

bool tileHasItemId(const MapEditor::Domain::Tile &tile, uint16_t itemId) {
  for (const auto &item : tile.getItems()) {
    if (item && item->getServerId() == itemId) {
      return true;
    }
  }
  return false;
}

void addOwnedItem(MapEditor::Domain::Tile &tile,
                  MapEditor::Brushes::BrushRegistry &registry,
                  const MapEditor::Brushes::IBrush &brush, uint16_t itemId,
                  bool complex = false) {
  auto item = std::make_unique<MapEditor::Domain::Item>(itemId);
  item->setOwnerBrushId(registry.getBrushId(&brush));
  if (complex) {
    item->setUniqueId(1234);
  }
  tile.addItem(std::move(item));
}

std::unique_ptr<MapEditor::Domain::Item>
makeOwnedItem(MapEditor::Brushes::BrushRegistry &registry,
              const MapEditor::Brushes::IBrush &brush, uint16_t itemId) {
  auto item = std::make_unique<MapEditor::Domain::Item>(itemId);
  item->setOwnerBrushId(registry.getBrushId(&brush));
  return item;
}

} // namespace

int main() {
  try {
    const fs::path sourceRoot = DOODAD_SMOKE_SOURCE_DIR;
    const fs::path dataPath = sourceRoot.parent_path() / "data" / "1098";
    require(fs::exists(dataPath / "materials.xml"),
            "data/1098/materials.xml is missing");

    MapEditor::Brushes::BrushRegistry registry;
    MapEditor::Services::TilesetService tilesetService(registry);
    require(tilesetService.loadMaterials(dataPath), "materials load failed");

    const MapEditor::Domain::Position redoCenter{10, 10, 7};
    const std::array<MapEditor::Brushes::DoodadRedoBorderTouch, 1>
        genericTouch{{{.position = redoCenter}}};
    const auto genericRedo =
        MapEditor::Brushes::buildDoodadRedoBorderPositions(genericTouch);
    require(genericRedo.size() == 1 && genericRedo.front() == redoCenter,
            "generic doodad redo-border should only queue the touched tile");

    const std::array<MapEditor::Brushes::DoodadRedoBorderTouch, 1> wallTouch{
        {{.position = redoCenter, .placedWall = true}}};
    const auto wallRedo =
        MapEditor::Brushes::buildDoodadRedoBorderPositions(wallTouch);
    require(wallRedo.size() == 5, "wall doodad redo-border should queue self plus cardinal neighbors");
    require(containsPosition(wallRedo, {10, 9, 7}) &&
                containsPosition(wallRedo, {9, 10, 7}) &&
                containsPosition(wallRedo, {11, 10, 7}) &&
                containsPosition(wallRedo, {10, 11, 7}),
            "wall doodad redo-border missed a cardinal neighbor");

    const std::array<MapEditor::Brushes::DoodadRedoBorderTouch, 1> groundTouch{
        {{.position = redoCenter, .placedGround = true, .placedWall = true}}};
    const auto groundRedo =
        MapEditor::Brushes::buildDoodadRedoBorderPositions(groundTouch);
    require(groundRedo.size() == 9,
            "ground doodad redo-border should queue a 3x3 area");
    require(containsPosition(groundRedo, {9, 9, 7}) &&
                containsPosition(groundRedo, {11, 11, 7}),
            "ground doodad redo-border missed a diagonal neighbor");

    auto *waterfall = findBrush(registry, "waterfall", BrushType::Doodad);
    auto *doodadBrush =
        dynamic_cast<MapEditor::Brushes::DoodadBrush *>(waterfall);
    require(doodadBrush != nullptr, "waterfall brush cast failed");
    auto *grassTufts = findBrush(registry, "grass tufts", BrushType::Doodad);
    auto *grassTuftsBrush =
        dynamic_cast<MapEditor::Brushes::DoodadBrush *>(grassTufts);
    require(grassTuftsBrush != nullptr, "grass tufts brush cast failed");
    auto *wallArch = findBrush(registry, "brick wall arch", BrushType::Doodad);
    auto *wallArchBrush =
        dynamic_cast<MapEditor::Brushes::DoodadBrush *>(wallArch);
    require(wallArchBrush != nullptr, "brick wall arch brush cast failed");
    auto *stoneWall = findBrush(registry, "stone wall", BrushType::Wall);
    auto *stoneWallBrush =
        dynamic_cast<MapEditor::Brushes::WallBrush *>(stoneWall);
    require(stoneWallBrush != nullptr, "stone wall brush cast failed");
    auto *bookcase = findBrush(registry, "bookcase", BrushType::Table);
    auto *bookcaseBrush =
        dynamic_cast<MapEditor::Brushes::TableBrush *>(bookcase);
    require(bookcaseBrush != nullptr, "bookcase brush cast failed");
    auto *redCarpet = findBrush(registry, "red carpet", BrushType::Carpet);
    auto *redCarpetBrush =
        dynamic_cast<MapEditor::Brushes::CarpetBrush *>(redCarpet);
    require(redCarpetBrush != nullptr, "red carpet brush cast failed");

    const auto waterfallItemId = doodadBrush->getPreviewItemId();
    const auto grassTuftsItemId = grassTuftsBrush->getPreviewItemId();
    require(waterfallItemId != 0 && grassTuftsItemId != 0,
            "doodad preview ids missing for erase policy test");

    MapEditor::Domain::ChunkedMap eraseMap;
    eraseMap.createNew(32, 32, 1098);
    auto *matchingTile = eraseMap.getOrCreateTile({4, 4, 7});
    require(matchingTile != nullptr, "matching erase test tile missing");
    addOwnedItem(*matchingTile, registry, *doodadBrush, waterfallItemId);
    addOwnedItem(*matchingTile, registry, *grassTuftsBrush, grassTuftsItemId);
    doodadBrush->undraw(eraseMap, matchingTile,
                        {.matchingBrushOnly = true,
                         .preserveComplexItems = false});
    require(!tileHasItemId(*matchingTile, waterfallItemId),
            "matching doodad erase kept current brush item");
    require(tileHasItemId(*matchingTile, grassTuftsItemId),
            "matching doodad erase removed another doodad brush item");

    auto *anyTile = eraseMap.getOrCreateTile({5, 4, 7});
    require(anyTile != nullptr, "any-doodad erase test tile missing");
    addOwnedItem(*anyTile, registry, *doodadBrush, waterfallItemId);
    addOwnedItem(*anyTile, registry, *grassTuftsBrush, grassTuftsItemId);
    doodadBrush->undraw(eraseMap, anyTile,
                        {.matchingBrushOnly = false,
                         .preserveComplexItems = false});
    require(!tileHasItemId(*anyTile, waterfallItemId) &&
                !tileHasItemId(*anyTile, grassTuftsItemId),
            "any-doodad erase did not remove both doodad-owned items");

    auto *complexTile = eraseMap.getOrCreateTile({6, 4, 7});
    require(complexTile != nullptr, "complex erase test tile missing");
    addOwnedItem(*complexTile, registry, *grassTuftsBrush, grassTuftsItemId,
                 true);
    doodadBrush->undraw(eraseMap, complexTile,
                        {.matchingBrushOnly = false,
                         .preserveComplexItems = true});
    require(tileHasItemId(*complexTile, grassTuftsItemId),
            "doodad erase removed complex item despite preserve setting");

    MapEditor::Domain::ChunkedMap map;
    map.createNew(128, 128, 1098);

    MapEditor::Domain::History::HistoryManager history;
    MapEditor::Services::BrushSettingsService settings;
    settings.setDoodadEraseMatchingOnly(true);
    settings.setEraserLeaveUniqueItems(false);
    settings.setLockDoors(true);
    settings.setPreviewBorder(false);
    settings.setAutoCreateSpawn(true);
    settings.setDefaultSpawnRadius(7);
    settings.setDefaultSpawnTime(120);
    const auto settingsPath =
        fs::temp_directory_path() / "tme_doodad_brush_settings_smoke.json";
    {
      MapEditor::Services::ConfigService config;
      config.setConfigPath(settingsPath);
      settings.saveToConfig(config);
      require(config.save(), "brush settings config save failed");
    }
    {
      MapEditor::Services::ConfigService config;
      config.setConfigPath(settingsPath);
      require(config.load(), "brush settings config reload failed");
      MapEditor::Services::BrushSettingsService reloadedSettings;
      reloadedSettings.loadFromConfig(config);
      require(reloadedSettings.getDoodadEraseMatchingOnly(),
              "doodad erase matching setting did not persist");
      require(!reloadedSettings.getEraserLeaveUniqueItems(),
              "eraser leave unique setting did not persist");
      require(reloadedSettings.getLockDoors(),
              "lock doors setting did not persist");
      require(!reloadedSettings.getPreviewBorder(),
              "preview border setting did not persist");
      require(reloadedSettings.getAutoCreateSpawn(),
              "auto spawn setting did not persist");
      require(reloadedSettings.getDefaultSpawnRadius() == 7,
              "spawn radius setting did not persist");
      require(reloadedSettings.getDefaultSpawnTime() == 120,
              "spawn time setting did not persist");
    }
    std::error_code removeError;
    fs::remove(settingsPath, removeError);

    settings.setDoodadEraseMatchingOnly(false);
    settings.setEraserLeaveUniqueItems(true);
    settings.setLockDoors(false);
    settings.setPreviewBorder(true);
    settings.setAutoCreateSpawn(false);
    settings.setDefaultSpawnRadius(3);
    settings.setDefaultSpawnTime(60);
    settings.setStandardSize(1);

    MapEditor::Brushes::BrushController controller;
    controller.initialize(&map, &history, nullptr);
    controller.setBrushRegistry(&registry);
    controller.setBrushSettingsService(&settings);
    std::vector<MapEditor::Domain::Position> notifiedMutations;
    controller.setTilesMutatedCallback(
        [&notifiedMutations](const auto &positions) {
          notifiedMutations = positions;
        });

    const auto wallItemId =
        stoneWallBrush->getWallItemForAlign(MapEditor::Brushes::WallAlign::Horizontal);
    require(wallItemId != 0, "stone wall horizontal item missing");
    const auto tableItemId = bookcaseBrush->getPreviewItemId();
    const auto carpetItemId = redCarpetBrush->getPreviewItemId();
    require(tableItemId != 0, "bookcase preview item missing");
    require(carpetItemId != 0, "red carpet preview item missing");

    auto *coveredWallTile = map.getOrCreateTile({20, 20, 7});
    require(coveredWallTile != nullptr, "covered wall context tile missing");
    coveredWallTile->addItemDirect(
        makeOwnedItem(registry, *stoneWallBrush, wallItemId));
    coveredWallTile->addItemDirect(
        std::make_unique<MapEditor::Domain::Item>(65000));
    const auto coveredWallSelection =
        controller.resolveBrushFromTile(*coveredWallTile, BrushPickMode::Wall);
    require(coveredWallSelection.has_value() &&
                coveredWallSelection->brush == stoneWallBrush,
            "wall context selection ignored wall below top raw item");

    auto *preferredDoodadTile = map.getOrCreateTile({21, 20, 7});
    require(preferredDoodadTile != nullptr,
            "preferred doodad context tile missing");
    auto preferredDoodad =
        makeOwnedItem(registry, *grassTuftsBrush, grassTuftsItemId);
    auto *preferredDoodadPtr = preferredDoodad.get();
    preferredDoodadTile->addItemDirect(std::move(preferredDoodad));
    preferredDoodadTile->addItemDirect(
        std::make_unique<MapEditor::Domain::Item>(65001));
    const auto preferredDoodadSelection = controller.resolveBrushFromTile(
        *preferredDoodadTile, BrushPickMode::Doodad, preferredDoodadPtr);
    require(preferredDoodadSelection.has_value() &&
                preferredDoodadSelection->brush == grassTuftsBrush,
            "preferred doodad context selection ignored selected doodad item");

    stoneWallBrush->setCollection();
    stoneWallBrush->flagAsVisible();
    auto *collectionTile = map.getOrCreateTile({22, 20, 7});
    require(collectionTile != nullptr, "collection context tile missing");
    collectionTile->addItemDirect(
        makeOwnedItem(registry, *stoneWallBrush, wallItemId));
    const auto collectionSelection =
        controller.resolveBrushFromTile(*collectionTile, BrushPickMode::Collection);
    require(collectionSelection.has_value() &&
                collectionSelection->brush == stoneWallBrush,
            "collection context selection did not prefer collection wall brush");

    bookcaseBrush->setCollection();
    bookcaseBrush->flagAsVisible();
    auto *tableTile = map.getOrCreateTile({24, 20, 7});
    require(tableTile != nullptr, "table context tile missing");
    tableTile->addItemDirect(
        makeOwnedItem(registry, *bookcaseBrush, tableItemId));
    const auto tableSelection =
        controller.resolveBrushFromTile(*tableTile, BrushPickMode::Table);
    require(tableSelection.has_value() && tableSelection->brush == bookcaseBrush,
            "table context selection did not resolve owned table brush");
    const auto tableCollectionSelection = controller.resolveBrushFromTile(
        *tableTile, BrushPickMode::Collection);
    require(tableCollectionSelection.has_value() &&
                tableCollectionSelection->brush == bookcaseBrush,
            "collection context selection did not resolve table brush");

    redCarpetBrush->setCollection();
    redCarpetBrush->flagAsVisible();
    auto *carpetTile = map.getOrCreateTile({25, 20, 7});
    require(carpetTile != nullptr, "carpet context tile missing");
    carpetTile->addItemDirect(
        makeOwnedItem(registry, *redCarpetBrush, carpetItemId));
    const auto carpetSelection =
        controller.resolveBrushFromTile(*carpetTile, BrushPickMode::Carpet);
    require(carpetSelection.has_value() &&
                carpetSelection->brush == redCarpetBrush,
            "carpet context selection did not resolve owned carpet brush");
    const auto carpetCollectionSelection = controller.resolveBrushFromTile(
        *carpetTile, BrushPickMode::Collection);
    require(carpetCollectionSelection.has_value() &&
                carpetCollectionSelection->brush == redCarpetBrush,
            "collection context selection did not resolve carpet brush");

    grassTuftsBrush->setCollection();
    grassTuftsBrush->flagAsVisible();
    auto *doodadCollectionTile = map.getOrCreateTile({26, 20, 7});
    require(doodadCollectionTile != nullptr,
            "doodad collection context tile missing");
    doodadCollectionTile->addItemDirect(
        makeOwnedItem(registry, *grassTuftsBrush, grassTuftsItemId));
    const auto doodadCollectionSelection = controller.resolveBrushFromTile(
        *doodadCollectionTile, BrushPickMode::Collection);
    require(doodadCollectionSelection.has_value() &&
                doodadCollectionSelection->brush == grassTuftsBrush,
            "collection context selection did not resolve doodad brush");

    auto *rawFallbackTile = map.getOrCreateTile({23, 20, 7});
    require(rawFallbackTile != nullptr, "raw fallback context tile missing");
    rawFallbackTile->addItemDirect(
        std::make_unique<MapEditor::Domain::Item>(65002));
    const auto rawSelection =
        controller.resolveBrushFromTile(*rawFallbackTile, BrushPickMode::Raw);
    require(rawSelection.has_value() && rawSelection->rawItemId == 65002,
            "raw context selection failed for unbound top item");

    MapEditor::Services::Preview::BrushPreviewFactory previewFactory;
    const MapEditor::Domain::Position position{40, 40, 7};

    require(hasPreviewTiles(previewFactory, waterfall, settings, &map, position),
            "empty doodad preview on empty map");
    require(hasPreviewTiles(previewFactory, waterfall, settings, nullptr, position),
            "mapless doodad preview should remain visible");

    const auto firstSeededLayout =
        doodadBrush->buildPlacementLayout(position, &settings,
                                          doodadBrush->getVariation(), &map,
                                          false, 12345u);
    const auto secondSeededLayout =
        doodadBrush->buildPlacementLayout(position, &settings,
                                          doodadBrush->getVariation(), &map,
                                          false, 12345u);
    require(layoutsEqual(firstSeededLayout, secondSeededLayout),
            "seeded doodad placement layout is not deterministic");
    const auto nonRedoPlan =
        grassTuftsBrush->buildPlacementPlan(position, &settings,
                                            grassTuftsBrush->getVariation(),
                                            &map, false, 12345u);
    require(nonRedoPlan.redoTouches.empty(),
            "non-redo doodad planner produced redo-border intent");
    require(!nonRedoPlan.affectedPositions.empty(),
            "non-redo doodad planner produced no affected positions");

    const MapEditor::Domain::Position redoPlanPosition{64, 60, 7};
    const auto redoPlanSeed =
        MapEditor::Brushes::DoodadPlacementPlanner::buildSeed(
            *wallArchBrush, redoPlanPosition, &settings,
            wallArchBrush->getVariation(), false);
    const auto redoPlan =
        wallArchBrush->buildPlacementPlan(redoPlanPosition, &settings,
                                          wallArchBrush->getVariation(), &map,
                                          false, redoPlanSeed);
    require(!redoPlan.layout.empty(),
            "redo doodad planner produced empty layout");
    require(!redoPlan.redoTouches.empty(),
            "redo doodad planner did not emit redo-border intent");
    const auto redoAffectedPositions =
        MapEditor::Brushes::buildDoodadRedoBorderPositions(
            redoPlan.redoTouches);
    for (const auto &touch : redoPlan.redoTouches) {
      require(containsPosition(
                  wallArchBrush->getPlacementPositions(
                      redoPlanPosition, &settings, wallArchBrush->getVariation(),
                      &map, false, redoPlanSeed),
                  touch.position),
              "redo doodad planner emitted a touch outside placement positions");
    }
    for (const auto &affectedPosition : redoAffectedPositions) {
      require(containsPosition(redoPlan.affectedPositions, affectedPosition),
              "redo doodad planner omitted a redo affected position");
    }

    settings.setStandardSize(4);
    const MapEditor::Domain::Position seededPreviewPosition{62, 62, 7};
    const auto providerPreview =
        buildProviderPreviewTiles(previewFactory, grassTufts, settings, &map,
                                  seededPreviewPosition);
    auto stableProvider =
        previewFactory.createProvider(grassTufts, &settings, &map);
    require(stableProvider != nullptr && stableProvider->isActive(),
            "stable doodad preview provider was not created");
    stableProvider->updateCursorPosition(seededPreviewPosition);
    const auto stableFirstPreview = stableProvider->getTiles();
    stableProvider->updateCursorPosition(seededPreviewPosition);
    require(!stableProvider->needsRegeneration(),
            "doodad preview marked same cursor tile dirty");
    const auto stableSecondPreview = stableProvider->getTiles();
    require(layoutsEqual(stableFirstPreview, stableSecondPreview),
            "doodad preview changed while cursor stayed on same tile");
    const auto providerSeed =
        MapEditor::Brushes::DoodadPlacementPlanner::buildSeed(
            *grassTuftsBrush, seededPreviewPosition, &settings,
            grassTuftsBrush->getVariation(), false);
    const auto seededCommitLayout =
        grassTuftsBrush->buildPlacementLayout(
            seededPreviewPosition, &settings, grassTuftsBrush->getVariation(),
            &map, false, providerSeed);
    require(layoutsEqual(providerPreview, seededCommitLayout),
            "doodad preview provider and commit layout used different seeds");

    for (uint32_t seed = 1; seed <= 50; ++seed) {
      const auto densityLayout =
          grassTuftsBrush->buildPlacementLayout({60, 60, 7}, &settings,
                                                grassTuftsBrush->getVariation(),
                                                &map, false, seed);
      const auto tileCount = countLayoutTiles(densityLayout);
      require(tileCount > 0, "RME-style doodad density produced no objects");
      require(tileCount <= 8,
              "RME-style doodad density exceeded expected 25/100 range");
    }
    settings.setStandardSize(1);

    controller.setBrush(waterfall);
    notifiedMutations.clear();
    require(controller.applyBrush(position), "waterfall paint failed");
    require(containsPosition(notifiedMutations, position),
            "doodad paint did not notify affected tile mutation");

    require(!hasPreviewTiles(previewFactory, waterfall, settings, &map, position),
            "map-aware doodad preview ignored duplicate placement rules");
    require(hasPreviewTiles(previewFactory, waterfall, settings, nullptr, position),
            "mapless doodad preview should not apply duplicate rules");
    const auto duplicatePlanSeed =
        MapEditor::Brushes::DoodadPlacementPlanner::buildSeed(
            *doodadBrush, position, &settings, doodadBrush->getVariation(),
            false);
    const auto duplicatePlan =
        doodadBrush->buildPlacementPlan(position, &settings,
                                        doodadBrush->getVariation(), &map,
                                        false, duplicatePlanSeed);
    require(duplicatePlan.layout.empty(),
            "duplicate doodad plan should reject occupied own item");
    require(duplicatePlan.affectedPositions.empty(),
            "duplicate doodad plan should not report affected positions");
    require(hasPlacementSkip(
                duplicatePlan, position,
                MapEditor::Brushes::DoodadBrush::PlacementSkipReason::
                    DuplicateOwnItem),
            "duplicate doodad plan did not report duplicate skip reason");

    const MapEditor::Domain::Position blockingPosition{70, 70, 7};
    auto *blockingTile = map.getOrCreateTile(blockingPosition);
    require(blockingTile != nullptr, "blocking skip tile missing");
    MapEditor::Domain::ItemType blockingItemType;
    blockingItemType.is_blocking = true;
    auto blockingItem =
        std::make_unique<MapEditor::Domain::Item>(65003);
    blockingItem->setType(&blockingItemType);
    blockingTile->addItemDirect(std::move(blockingItem));
    const auto blockingSeed =
        MapEditor::Brushes::DoodadPlacementPlanner::buildSeed(
            *grassTuftsBrush, blockingPosition, &settings,
            grassTuftsBrush->getVariation(), false);
    const auto blockingPlan =
        grassTuftsBrush->buildPlacementPlan(blockingPosition, &settings,
                                            grassTuftsBrush->getVariation(),
                                            &map, false, blockingSeed);
    require(blockingPlan.layout.empty(),
            "blocking doodad plan should reject blocked tile");
    require(blockingPlan.affectedPositions.empty(),
            "blocking doodad plan should not report affected positions");
    require(hasPlacementSkip(
                blockingPlan, blockingPosition,
                MapEditor::Brushes::DoodadBrush::PlacementSkipReason::
                    BlockingTile),
            "blocking doodad plan did not report blocking skip reason");

    const MapEditor::Domain::Position eraseRedoPosition{74, 70, 7};
    auto *eraseRedoTile = map.getOrCreateTile(eraseRedoPosition);
    require(eraseRedoTile != nullptr, "redo erase tile missing");
    MapEditor::Domain::ItemType eraseRedoItemType;
    eraseRedoItemType.is_wall = true;
    auto eraseRedoItem =
        makeOwnedItem(registry, *wallArchBrush, wallArchBrush->getPreviewItemId());
    eraseRedoItem->setType(&eraseRedoItemType);
    eraseRedoTile->addItemDirect(std::move(eraseRedoItem));
    const auto eraseRedoSeed =
        MapEditor::Brushes::DoodadPlacementPlanner::buildSeed(
            *wallArchBrush, eraseRedoPosition, &settings,
            wallArchBrush->getVariation(), false);
    const auto eraseRedoPlan = wallArchBrush->buildErasePlan(
        eraseRedoPosition, &settings, wallArchBrush->getVariation(), &map,
        false, eraseRedoSeed,
        {.matchingBrushOnly = true, .preserveComplexItems = true});
    require(containsPosition(eraseRedoPlan.positions, eraseRedoPosition),
            "redo erase plan did not include erased tile");
    require(containsPosition(eraseRedoPlan.affectedPositions,
                             {eraseRedoPosition.x - 1, eraseRedoPosition.y,
                              eraseRedoPosition.z}) &&
                containsPosition(eraseRedoPlan.affectedPositions,
                                 {eraseRedoPosition.x + 1, eraseRedoPosition.y,
                                  eraseRedoPosition.z}) &&
                containsPosition(eraseRedoPlan.affectedPositions,
                                 {eraseRedoPosition.x, eraseRedoPosition.y - 1,
                                  eraseRedoPosition.z}) &&
                containsPosition(eraseRedoPlan.affectedPositions,
                                 {eraseRedoPosition.x, eraseRedoPosition.y + 1,
                                  eraseRedoPosition.z}),
            "redo erase plan did not include cardinal affected positions");

    std::cout << "DoodadPreviewSmoke passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "DoodadPreviewSmoke failed: " << e.what() << "\n";
    return 1;
  }
}
