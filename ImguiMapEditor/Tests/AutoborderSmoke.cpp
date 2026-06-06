#include "Brushes/BrushController.h"
#include "Brushes/BrushRegistry.h"
#include "Brushes/Core/IBrush.h"
#include "Brushes/Types/WallBrush.h"
#include "Domain/ChunkedMap.h"
#include "Domain/History/HistoryManager.h"
#include "Domain/History/TileSnapshot.h"
#include "Services/Autoborder/AutoborderEngine.h"
#include "Services/Autoborder/TileDiff.h"
#include "Services/BrushSettingsService.h"
#include "Services/Preview/AutoborderBrushPreviewProvider.h"
#include "Services/TilesetService.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

using BrushType = MapEditor::Brushes::BrushType;

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

std::vector<uint8_t>
snapshotTileData(const MapEditor::Domain::ChunkedMap &map,
                 const MapEditor::Domain::Position &pos) {
  return MapEditor::Domain::History::TileSnapshot::capture(map.getTile(pos), pos)
      .data();
}

void requireMapsMatchInRegion(const MapEditor::Domain::ChunkedMap &expected,
                              const MapEditor::Domain::ChunkedMap &actual,
                              const MapEditor::Domain::Position &center,
                              int radius, std::string_view label) {
  for (int dy = -radius; dy <= radius; ++dy) {
    for (int dx = -radius; dx <= radius; ++dx) {
      const MapEditor::Domain::Position pos{center.x + dx, center.y + dy,
                                            center.z};
      if (snapshotTileData(expected, pos) == snapshotTileData(actual, pos)) {
        continue;
      }

      throw std::runtime_error(std::string(label)
                                   .append(" map mismatch at ")
                                   .append(std::to_string(pos.x))
                                   .append(",")
                                   .append(std::to_string(pos.y))
                                   .append(",")
                                   .append(std::to_string(pos.z)));
    }
  }
}

std::map<MapEditor::Domain::Position, std::vector<uint8_t>>
snapshotRegion(const MapEditor::Domain::ChunkedMap &map,
               const MapEditor::Domain::Position &center, int radius) {
  std::map<MapEditor::Domain::Position, std::vector<uint8_t>> snapshot;
  for (int dy = -radius; dy <= radius; ++dy) {
    for (int dx = -radius; dx <= radius; ++dx) {
      const MapEditor::Domain::Position pos{center.x + dx, center.y + dy,
                                            center.z};
      snapshot.emplace(pos, snapshotTileData(map, pos));
    }
  }
  return snapshot;
}

void requireRegionMatchesSnapshot(
    const MapEditor::Domain::ChunkedMap &map,
    const std::map<MapEditor::Domain::Position, std::vector<uint8_t>>
        &snapshot,
    std::string_view label) {
  for (const auto &[pos, expected] : snapshot) {
    if (snapshotTileData(map, pos) == expected) {
      continue;
    }

    throw std::runtime_error(std::string(label)
                                 .append(" history mismatch at ")
                                 .append(std::to_string(pos.x))
                                 .append(",")
                                 .append(std::to_string(pos.y))
                                 .append(",")
                                 .append(std::to_string(pos.z)));
  }
}

bool snapshotsEqual(
    const std::map<MapEditor::Domain::Position, std::vector<uint8_t>> &lhs,
    const std::map<MapEditor::Domain::Position, std::vector<uint8_t>> &rhs) {
  return lhs == rhs;
}

MapEditor::Services::Autoborder::PlacementIntent makeIntent(
    const MapEditor::Brushes::IBrush *brush,
    MapEditor::Brushes::BrushRegistry &registry,
    MapEditor::Services::BrushSettingsService &settings,
    const MapEditor::Domain::Position &pos,
    MapEditor::Services::Autoborder::PlacementMode mode) {
  MapEditor::Services::Autoborder::PlacementIntent intent;
  intent.brush = brush;
  intent.mode = mode;
  intent.positions = {pos};
  intent.context.brushSettings = &settings;
  intent.context.brushRegistry = &registry;
  intent.context.ownerBrushId = registry.getBrushId(brush);
  return intent;
}

void requirePlannedDrawMatchesController(
    MapEditor::Domain::ChunkedMap &map,
    MapEditor::Brushes::BrushController &controller,
    MapEditor::Brushes::BrushRegistry &registry,
    MapEditor::Services::BrushSettingsService &settings,
    MapEditor::Brushes::IBrush *brush,
    const MapEditor::Domain::Position &pos, std::string_view label) {
  auto before = map.clone();
  auto intent = makeIntent(
      brush, registry, settings, pos,
      MapEditor::Services::Autoborder::PlacementMode::Draw);

  MapEditor::Services::Autoborder::AutoborderEngine engine;
  require(engine.canPlan(intent),
          std::string(label).append(" planner unavailable"));
  auto diffs = engine.plan(*before, intent);
  require(!diffs.empty(), std::string(label).append(" produced no diffs"));

  auto expected = before->clone();
  MapEditor::Services::Autoborder::applyTileDiffs(*expected, diffs);

  controller.setBrush(brush);
  require(controller.applyBrush(pos),
          std::string(label).append(" controller draw failed"));
  requireMapsMatchInRegion(*expected, map, pos, 2, label);
}

void requirePlannedEraseMatchesController(
    MapEditor::Domain::ChunkedMap &map,
    MapEditor::Brushes::BrushController &controller,
    MapEditor::Brushes::BrushRegistry &registry,
    MapEditor::Services::BrushSettingsService &settings,
    MapEditor::Brushes::IBrush *brush,
    const MapEditor::Domain::Position &pos, std::string_view label) {
  auto before = map.clone();
  auto intent = makeIntent(
      brush, registry, settings, pos,
      MapEditor::Services::Autoborder::PlacementMode::Erase);

  MapEditor::Services::Autoborder::AutoborderEngine engine;
  require(engine.canPlan(intent),
          std::string(label).append(" planner unavailable"));
  auto diffs = engine.plan(*before, intent);
  require(!diffs.empty(), std::string(label).append(" produced no erase diffs"));

  auto expected = before->clone();
  MapEditor::Services::Autoborder::applyTileDiffs(*expected, diffs);

  controller.setBrush(brush);
  require(controller.eraseBrush(pos),
          std::string(label).append(" controller erase failed"));
  requireMapsMatchInRegion(*expected, map, pos, 2, label);
}

std::vector<uint32_t> tileItemIds(const MapEditor::Domain::Tile *tile) {
  std::vector<uint32_t> ids;
  if (!tile) {
    return ids;
  }

  if (const auto *ground = tile->getGround()) {
    ids.push_back(ground->getServerId());
  }
  for (const auto &item : tile->getItems()) {
    if (item) {
      ids.push_back(item->getServerId());
    }
  }
  return ids;
}

size_t countOwnedItems(const MapEditor::Domain::Tile &tile,
                       const MapEditor::Brushes::IBrush &brush) {
  size_t count = 0;
  for (const auto &item : tile.getItems()) {
    if (item && brush.ownsItem(item.get())) {
      ++count;
    }
  }
  return count;
}

bool tileHasDoorForBrush(const MapEditor::Domain::Tile &tile,
                         const MapEditor::Brushes::WallBrush &wallBrush) {
  for (const auto &item : tile.getItems()) {
    if (item && wallBrush.findDoorForItem(item->getServerId()).has_value()) {
      return true;
    }
  }
  return false;
}

void requirePreviewMatchesControllerDraw(
    MapEditor::Domain::ChunkedMap &map,
    MapEditor::Brushes::BrushController &controller,
    MapEditor::Services::BrushSettingsService &settings,
    MapEditor::Brushes::IBrush *brush,
    const MapEditor::Domain::Position &pos, std::string_view label) {
  MapEditor::Services::Preview::AutoborderBrushPreviewProvider provider(
      brush, &settings, &map);
  provider.updateCursorPosition(pos);

  std::map<MapEditor::Domain::Position, std::vector<uint32_t>> previewItems;
  for (const auto &tile : provider.getTiles()) {
    const MapEditor::Domain::Position absolute{
        pos.x + tile.relativePosition.x, pos.y + tile.relativePosition.y,
        static_cast<int16_t>(pos.z + tile.relativePosition.z)};
    auto &ids = previewItems[absolute];
    ids.reserve(tile.items.size());
    for (const auto &item : tile.items) {
      ids.push_back(item.itemId);
    }
  }
  require(!previewItems.empty(), std::string(label).append(" preview empty"));

  controller.setBrush(brush);
  require(controller.applyBrush(pos),
          std::string(label).append(" controller draw failed"));

  for (const auto &[previewPos, expectedIds] : previewItems) {
    require(tileItemIds(map.getTile(previewPos)) == expectedIds,
            std::string(label).append(" preview/commit item mismatch"));
  }
}

void requireUndoRedoRoundTrip(
    MapEditor::Domain::ChunkedMap &map,
    MapEditor::Domain::History::HistoryManager &history,
    MapEditor::Brushes::BrushController &controller,
    MapEditor::Brushes::IBrush *brush,
    const MapEditor::Domain::Position &pos, std::string_view label) {
  history.clear();
  const auto before = snapshotRegion(map, pos, 2);

  controller.setBrush(brush);
  require(controller.applyBrush(pos),
          std::string(label).append(" draw failed"));

  const auto after = snapshotRegion(map, pos, 2);
  require(history.canUndo(), std::string(label).append(" did not record undo"));
  require(!history.undo(&map).empty(),
          std::string(label).append(" undo failed"));
  requireRegionMatchesSnapshot(map, before,
                               std::string(label).append(" undo"));
  require(history.canRedo(), std::string(label).append(" did not record redo"));
  require(!history.redo(&map).empty(),
          std::string(label).append(" redo failed"));
  requireRegionMatchesSnapshot(map, after,
                               std::string(label).append(" redo"));
}

void requireWallStrokeDefersRebuild(
    MapEditor::Domain::ChunkedMap &map,
    MapEditor::Domain::History::HistoryManager &history,
    MapEditor::Brushes::BrushController &controller,
    MapEditor::Brushes::IBrush *groundBrush,
    MapEditor::Brushes::IBrush *wallBrush,
    std::string_view label) {
  const std::array strokePositions{
      MapEditor::Domain::Position{124, 24, 7},
      MapEditor::Domain::Position{124, 25, 7},
      MapEditor::Domain::Position{125, 25, 7},
  };
  const MapEditor::Domain::Position snapshotCenter{124, 25, 7};

  for (const auto &pos : strokePositions) {
    controller.setBrush(groundBrush);
    require(controller.applyBrush(pos),
            std::string(label).append(" support ground paint failed"));
  }

  history.clear();
  const auto beforeStroke = snapshotRegion(map, snapshotCenter, 3);

  controller.setBrush(wallBrush);
  controller.beginStroke();
  for (const auto &pos : strokePositions) {
    controller.continueStroke(pos);
  }

  const auto beforeFinalize = snapshotRegion(map, snapshotCenter, 3);
  require(!snapshotsEqual(beforeStroke, beforeFinalize),
          std::string(label).append(" did not paint during active stroke"));

  controller.endStroke();

  const auto afterFinalize = snapshotRegion(map, snapshotCenter, 3);
  require(!snapshotsEqual(beforeFinalize, afterFinalize),
          std::string(label).append(" did not defer wall rebuild to endStroke"));
  require(history.canUndo(),
          std::string(label).append(" did not record undo"));
  require(!history.undo(&map).empty(),
          std::string(label).append(" undo failed"));
  requireRegionMatchesSnapshot(map, beforeStroke,
                               std::string(label).append(" undo"));
  require(history.canRedo(),
          std::string(label).append(" did not record redo"));
  require(!history.redo(&map).empty(),
          std::string(label).append(" redo failed"));
  requireRegionMatchesSnapshot(map, afterFinalize,
                               std::string(label).append(" redo"));
}

void requireWallDragPlanSkipsNeighborResolve(
    MapEditor::Domain::ChunkedMap &map,
    MapEditor::Brushes::BrushController &controller,
    MapEditor::Brushes::BrushRegistry &registry,
    MapEditor::Services::BrushSettingsService &settings,
    MapEditor::Brushes::IBrush *groundBrush,
    MapEditor::Brushes::IBrush *wallBrush,
    std::string_view label) {
  const std::array strokePositions{
      MapEditor::Domain::Position{118, 28, 7},
      MapEditor::Domain::Position{118, 29, 7},
      MapEditor::Domain::Position{119, 29, 7},
  };

  for (const auto &pos : strokePositions) {
    controller.setBrush(groundBrush);
    require(controller.applyBrush(pos),
            std::string(label).append(" support ground paint failed"));
  }

  MapEditor::Services::Autoborder::AutoborderEngine engine;
  auto dragIntent = makeIntent(
      wallBrush, registry, settings, strokePositions.front(),
      MapEditor::Services::Autoborder::PlacementMode::Draw);
  dragIntent.positions.assign(strokePositions.begin(), strokePositions.end());
  dragIntent.context.isDragging = true;

  auto dragDiffs = engine.plan(map, dragIntent);
  require(!dragDiffs.empty(),
          std::string(label).append(" active drag produced no diffs"));

  for (const auto &diff : dragDiffs) {
    const auto isStrokeTile =
        std::find(strokePositions.begin(), strokePositions.end(),
                  diff.position) != strokePositions.end();
    require(isStrokeTile,
            std::string(label).append(" active drag emitted neighbor diff"));
  }

  auto afterDrag = map.clone();
  MapEditor::Services::Autoborder::applyTileDiffs(*afterDrag, dragDiffs);

  auto resolveIntent = makeIntent(
      wallBrush, registry, settings, strokePositions.front(),
      MapEditor::Services::Autoborder::PlacementMode::ResolveOnly);
  resolveIntent.positions.assign(strokePositions.begin(), strokePositions.end());
  resolveIntent.context.isDragging = true;

  const auto resolveDiffs = engine.plan(*afterDrag, resolveIntent);
  require(!resolveDiffs.empty(),
          std::string(label).append(" final resolve produced no diffs"));
}

} // namespace

int main() {
  try {
    const fs::path sourceRoot = AUTOBORDER_SMOKE_SOURCE_DIR;
    const fs::path dataPath = sourceRoot.parent_path() / "data" / "1098";
    require(fs::exists(dataPath / "materials.xml"),
            "data/1098/materials.xml is missing");

    MapEditor::Brushes::BrushRegistry registry;
    MapEditor::Services::TilesetService tilesetService(registry);
    require(tilesetService.loadMaterials(dataPath), "materials load failed");

    auto *caveBrush = findBrush(registry, "cave", BrushType::Ground);
    auto *seaBrush = findBrush(registry, "sea", BrushType::Ground);
    auto *sandBrush = findBrush(registry, "sand", BrushType::Ground);
    auto *stoneWallBrush = findBrush(registry, "stone wall", BrushType::Wall);
    const auto *stoneWall =
        dynamic_cast<const MapEditor::Brushes::WallBrush *>(stoneWallBrush);
    require(stoneWall != nullptr, "stone wall brush cast failed");
    auto *mossyWallBrush =
        findBrush(registry, "mossy wall", BrushType::WallDecoration);
    auto *redCarpetBrush = findBrush(registry, "red carpet", BrushType::Carpet);
    auto *logBrush = findBrush(registry, "log", BrushType::Table);

    MapEditor::Domain::ChunkedMap map;
    map.createNew(128, 128, 1098);

    MapEditor::Domain::History::HistoryManager history;
    MapEditor::Services::BrushSettingsService settings;
    settings.setStandardSize(1);

    MapEditor::Brushes::BrushController controller;
    controller.initialize(&map, &history, nullptr);
    controller.setBrushRegistry(&registry);
    controller.setBrushSettingsService(&settings);

    requirePlannedDrawMatchesController(map, controller, registry, settings,
                                        caveBrush, {80, 20, 7},
                                        "ground draw");
    requirePreviewMatchesControllerDraw(map, controller, settings, caveBrush,
                                        {81, 20, 7}, "ground preview/commit");

    controller.setBrush(seaBrush);
    require(controller.applyBrush({82, 20, 7}), "sea support paint failed");
    requirePlannedDrawMatchesController(map, controller, registry, settings,
                                        sandBrush, {83, 20, 7},
                                        "ground border draw");
    requirePlannedEraseMatchesController(map, controller, registry, settings,
                                         sandBrush, {83, 20, 7},
                                         "ground border erase");

    for (const auto &pos : std::array{MapEditor::Domain::Position{86, 20, 7},
                                      MapEditor::Domain::Position{87, 20, 7}}) {
      controller.setBrush(caveBrush);
      require(controller.applyBrush(pos), "wall support ground paint failed");
    }
    requirePlannedDrawMatchesController(map, controller, registry, settings,
                                        stoneWallBrush, {86, 20, 7},
                                        "wall draw");
    requirePlannedDrawMatchesController(map, controller, registry, settings,
                                        stoneWallBrush, {87, 20, 7},
                                        "wall neighbor draw");
    requirePlannedEraseMatchesController(map, controller, registry, settings,
                                         stoneWallBrush, {86, 20, 7},
                                         "wall erase");
    requirePreviewMatchesControllerDraw(map, controller, settings,
                                        stoneWallBrush, {88, 20, 7},
                                        "wall preview/commit");

    const MapEditor::Domain::Position doorLeftPos{100, 20, 7};
    const MapEditor::Domain::Position doorCenterPos{101, 20, 7};
    const MapEditor::Domain::Position doorRightPos{102, 20, 7};
    for (const auto &pos :
         std::array{doorLeftPos, doorCenterPos, doorRightPos}) {
      controller.setBrush(caveBrush);
      require(controller.applyBrush(pos), "door support ground paint failed");
      controller.setBrush(stoneWallBrush);
      require(controller.applyBrush(pos), "door support wall paint failed");
    }
    controller.activateMagicDoorBrush();
    require(controller.applyBrush(doorCenterPos), "door paint failed");
    require(tileHasDoorForBrush(*map.getTile(doorCenterPos), *stoneWall),
            "door paint did not leave a wall door item");
    controller.setBrush(stoneWallBrush);
    require(controller.eraseBrush(doorLeftPos), "door neighbor erase failed");
    require(tileHasDoorForBrush(*map.getTile(doorCenterPos), *stoneWall),
            "wall rebuild removed the door item");

    const MapEditor::Domain::Position decoLeftPos{104, 20, 7};
    const MapEditor::Domain::Position decoCenterPos{105, 20, 7};
    const MapEditor::Domain::Position decoRightPos{106, 20, 7};
    for (const auto &pos :
         std::array{decoLeftPos, decoCenterPos, decoRightPos}) {
      controller.setBrush(caveBrush);
      require(controller.applyBrush(pos),
              "decoration support ground paint failed");
      controller.setBrush(stoneWallBrush);
      require(controller.applyBrush(pos), "decoration base wall paint failed");
    }
    controller.setBrush(mossyWallBrush);
    require(controller.applyBrush(decoCenterPos), "decoration paint failed");
    const auto decorationCountBefore =
        countOwnedItems(*map.getTile(decoCenterPos), *mossyWallBrush);
    require(decorationCountBefore > 0,
            "wall decoration brush did not place an owned item");
    controller.setBrush(stoneWallBrush);
    require(controller.eraseBrush(decoLeftPos),
            "decoration neighbor erase failed");
    const auto decorationCountAfter =
        countOwnedItems(*map.getTile(decoCenterPos), *mossyWallBrush);
    require(decorationCountAfter == decorationCountBefore,
            "wall rebuild removed the stacked wall decoration");

    for (const auto &pos : std::array{MapEditor::Domain::Position{90, 20, 7},
                                      MapEditor::Domain::Position{91, 20, 7},
                                      MapEditor::Domain::Position{94, 20, 7},
                                      MapEditor::Domain::Position{95, 20, 7}}) {
      controller.setBrush(caveBrush);
      require(controller.applyBrush(pos), "shape support ground paint failed");
    }

    controller.setBrush(redCarpetBrush);
    require(controller.applyBrush({90, 20, 7}), "carpet support paint failed");
    requirePlannedDrawMatchesController(map, controller, registry, settings,
                                        redCarpetBrush, {91, 20, 7},
                                        "carpet neighbor draw");
    controller.setBrush(caveBrush);
    require(controller.applyBrush({92, 20, 7}),
            "carpet preview support ground paint failed");
    requirePreviewMatchesControllerDraw(map, controller, settings,
                                        redCarpetBrush, {92, 20, 7},
                                        "carpet preview/commit");

    controller.setBrush(logBrush);
    require(controller.applyBrush({94, 20, 7}), "table support paint failed");
    requirePlannedDrawMatchesController(map, controller, registry, settings,
                                        logBrush, {95, 20, 7},
                                        "table neighbor draw");
    controller.setBrush(caveBrush);
    require(controller.applyBrush({96, 20, 7}),
            "table preview support ground paint failed");
    requirePreviewMatchesControllerDraw(map, controller, settings, logBrush,
                                        {96, 20, 7}, "table preview/commit");

    controller.setBrush(seaBrush);
    require(controller.applyBrush({112, 20, 7}),
            "undo ground border support paint failed");
    requireUndoRedoRoundTrip(map, history, controller, sandBrush, {113, 20, 7},
                             "ground autoborder undo/redo");
    requireUndoRedoRoundTrip(map, history, controller, stoneWallBrush,
                             {116, 20, 7}, "wall autoborder undo/redo");
    requireUndoRedoRoundTrip(map, history, controller, redCarpetBrush,
                             {119, 20, 7}, "carpet autoborder undo/redo");
    requireUndoRedoRoundTrip(map, history, controller, logBrush, {122, 20, 7},
                             "table autoborder undo/redo");
    requireWallStrokeDefersRebuild(map, history, controller, caveBrush,
                                   stoneWallBrush,
                                   "wall drag deferred autoborder");
    requireWallDragPlanSkipsNeighborResolve(
        map, controller, registry, settings, caveBrush, stoneWallBrush,
        "wall drag planner");

    std::cout << "Autoborder smoke passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "Autoborder smoke failed: " << ex.what() << "\n";
    return 1;
  }
}
