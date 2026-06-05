#include "Brushes/BrushController.h"
#include "Brushes/BrushRegistry.h"
#include "Brushes/Core/IBrush.h"
#include "Domain/ChunkedMap.h"
#include "Domain/History/HistoryManager.h"
#include "Domain/History/TileSnapshot.h"
#include "Services/Autoborder/AutoborderEngine.h"
#include "Services/Autoborder/TileDiff.h"
#include "Services/BrushSettingsService.h"
#include "Services/TilesetService.h"
#include <array>
#include <filesystem>
#include <iostream>
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

    controller.setBrush(seaBrush);
    require(controller.applyBrush({82, 20, 7}), "sea support paint failed");
    requirePlannedDrawMatchesController(map, controller, registry, settings,
                                        sandBrush, {83, 20, 7},
                                        "ground border draw");

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

    controller.setBrush(logBrush);
    require(controller.applyBrush({94, 20, 7}), "table support paint failed");
    requirePlannedDrawMatchesController(map, controller, registry, settings,
                                        logBrush, {95, 20, 7},
                                        "table neighbor draw");

    std::cout << "Autoborder smoke passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "Autoborder smoke failed: " << ex.what() << "\n";
    return 1;
  }
}
