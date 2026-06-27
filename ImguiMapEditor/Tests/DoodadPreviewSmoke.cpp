#include "Brushes/BrushController.h"
#include "Brushes/BrushRegistry.h"
#include "Brushes/Core/IBrush.h"
#include "Brushes/Types/DoodadBrush.h"
#include "Brushes/Types/DoodadPlacementPlanner.h"
#include "Domain/ChunkedMap.h"
#include "Domain/History/HistoryManager.h"
#include "Domain/Item.h"
#include "Domain/ItemType.h"
#include "Services/BrushSettingsService.h"
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

void require(bool condition, std::string_view message) {
  if (!condition) {
    throw std::runtime_error(std::string(message));
  }
}

MapEditor::Brushes::IBrush *
findBrush(MapEditor::Brushes::BrushRegistry &registry, std::string_view name,
          MapEditor::Brushes::BrushType expectedType) {
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

    auto *waterfall = findBrush(registry, "waterfall", MapEditor::Brushes::BrushType::Doodad);
    auto *doodadBrush =
        dynamic_cast<MapEditor::Brushes::DoodadBrush *>(waterfall);
    require(doodadBrush != nullptr, "waterfall brush cast failed");
    
    auto *grassTufts = findBrush(registry, "grass tufts", MapEditor::Brushes::BrushType::Doodad);
    auto *grassTuftsBrush =
        dynamic_cast<MapEditor::Brushes::DoodadBrush *>(grassTufts);
    require(grassTuftsBrush != nullptr, "grass tufts brush cast failed");

    MapEditor::Domain::ChunkedMap map;
    map.createNew(128, 128, 1098);

    MapEditor::Domain::History::HistoryManager history;
    MapEditor::Services::BrushSettingsService settings;
    settings.setExactBrushSize(true);
    settings.setBrushSizeX(4);
    settings.setBrushSizeY(4);

    MapEditor::Brushes::BrushController controller;
    controller.initialize(&map, &history, nullptr);
    controller.setBrushRegistry(&registry);
    controller.setBrushSettingsService(&settings);
    
    std::vector<MapEditor::Domain::Position> notifiedMutations;
    controller.setTilesMutatedCallback(
        [&notifiedMutations](const auto &positions) {
          notifiedMutations = positions;
        });

    MapEditor::Services::Preview::BrushPreviewFactory previewFactory;
    const MapEditor::Domain::Position position{40, 40, 7};

    // 1. Verify preview is active and visible
    require(hasPreviewTiles(previewFactory, waterfall, settings, &map, position),
            "empty doodad preview on empty map");

    // 2. Verify preview stability while moving cursor (seed/nonce stays stable)
    const MapEditor::Domain::Position seededPreviewPosition{62, 62, 7};
    const MapEditor::Domain::Position movedPreviewPosition{63, 62, 7};
    
    auto stableProvider = previewFactory.createProvider(grassTufts, &settings, &map);
    require(stableProvider != nullptr && stableProvider->isActive(),
            "stable doodad preview provider was not created");
            
    stableProvider->updateCursorPosition(seededPreviewPosition);
    const auto seedBeforeMove = stableProvider->getCurrentSeed();
    const auto firstTiles = stableProvider->getTiles();
    
    // Move the cursor to a different position
    stableProvider->updateCursorPosition(movedPreviewPosition);
    const auto seedAfterMove = stableProvider->getCurrentSeed();
    const auto secondTiles = stableProvider->getTiles();
    
    require(seedBeforeMove.has_value() && seedAfterMove.has_value(), "seeds missing");
    require(*seedBeforeMove == *seedAfterMove, "seed changed on cursor movement!");
    
    // Verify that the relative layouts are equal (meaning the chosen items did not change)
    require(layoutsEqual(firstTiles, secondTiles), "relative preview layout changed on cursor movement!");

    // 3. Verify density calculation
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
    settings.setStandardSize(0);

    // 4. Verify controller painting and notification
    controller.setBrush(waterfall);
    notifiedMutations.clear();
    require(controller.applyBrush(position), "waterfall paint failed");
    require(containsPosition(notifiedMutations, position),
            "doodad paint did not notify affected tile mutation");

    std::cout << "DoodadPreviewSmoke passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "DoodadPreviewSmoke failed: " << e.what() << "\n";
    return 1;
  }
}
