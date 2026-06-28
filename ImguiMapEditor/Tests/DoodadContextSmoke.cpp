#include "Brushes/BrushController.h"
#include "Brushes/BrushRegistry.h"
#include "Brushes/Types/DoodadBrush.h"
#include "Brushes/Types/WallBrush.h"
#include "Brushes/Types/TableBrush.h"
#include "Brushes/Types/CarpetBrush.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Item.h"
#include "Domain/Tile.h"
#include "Services/BrushSettingsService.h"
#include "Domain/History/HistoryManager.h"
#include <iostream>
#include <stdexcept>
#include <cassert>

namespace {
void require(bool condition, const std::string &msg) {
  if (!condition) {
    throw std::runtime_error(msg);
  }
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
    MapEditor::Brushes::BrushRegistry registry;
    MapEditor::Domain::ChunkedMap map;
    map.createNew(32, 32, 1098);

    // 1. Verify variation mapping 1-to-1 (no variants flattening)
    {
      MapEditor::Brushes::DoodadBrush brush("var_test", 100, registry, false);
      require(brush.getMaxVariation() == 0, "initial variation max should be 0");
      
      MapEditor::Brushes::DoodadAlternative alt1;
      alt1.addSingleItem({.itemId = 100, .chance = 10});
      alt1.addSingleItem({.itemId = 101, .chance = 10});
      brush.addAlternative(std::move(alt1));
      
      // Max variation should be 1 (exactly the number of alternatives)
      require(brush.getMaxVariation() == 1, "max variation after 1 alt should be 1");
      
      MapEditor::Brushes::DoodadAlternative alt2;
      alt2.addSingleItem({.itemId = 200, .chance = 10});
      brush.addAlternative(std::move(alt2));
      
      require(brush.getMaxVariation() == 2, "max variation after 2 alts should be 2");
    }

    // 2. Setup Context-picking test
    auto grassTuftsPtr = std::make_unique<MapEditor::Brushes::DoodadBrush>("grass tufts", 400, registry, false);
    MapEditor::Brushes::DoodadAlternative alt;
    alt.addSingleItem({.itemId = 400, .chance = 10});
    grassTuftsPtr->addAlternative(std::move(alt));
    auto bookcasePtr = std::make_unique<MapEditor::Brushes::TableBrush>("bookcase", 500, registry);
    auto redCarpetPtr = std::make_unique<MapEditor::Brushes::CarpetBrush>("red carpet", 600, registry);

    auto &grassTufts = *grassTuftsPtr;
    auto &bookcase = *bookcasePtr;
    auto &redCarpet = *redCarpetPtr;

    registry.addBrush(std::move(grassTuftsPtr));
    registry.addBrush(std::move(bookcasePtr));
    registry.addBrush(std::move(redCarpetPtr));

    MapEditor::Services::BrushSettingsService settings;
    MapEditor::Domain::History::HistoryManager history;
    MapEditor::Brushes::BrushController controller;
    controller.initialize(&map, &history, nullptr);
    controller.setBrushRegistry(&registry);
    controller.setBrushSettingsService(&settings);

    // Table context selection
    bookcase.setCollection();
    bookcase.flagAsVisible();
    auto *tableTile = map.getOrCreateTile({24, 20, 7});
    tableTile->addItemDirect(makeOwnedItem(registry, bookcase, 500));
    const auto tableSelection = controller.resolveBrushFromTile(*tableTile, MapEditor::Brushes::BrushPickMode::Table);
    require(tableSelection.has_value() && tableSelection->brush == &bookcase,
            "table context selection did not resolve owned table brush");

    // Carpet context selection
    redCarpet.setCollection();
    redCarpet.flagAsVisible();
    auto *carpetTile = map.getOrCreateTile({25, 20, 7});
    carpetTile->addItemDirect(makeOwnedItem(registry, redCarpet, 600));
    const auto carpetSelection = controller.resolveBrushFromTile(*carpetTile, MapEditor::Brushes::BrushPickMode::Carpet);
    require(carpetSelection.has_value() && carpetSelection->brush == &redCarpet,
            "carpet context selection did not resolve owned carpet brush");

    // Doodad collection context selection
    grassTufts.setCollection();
    grassTufts.flagAsVisible();
    auto *doodadCollectionTile = map.getOrCreateTile({26, 20, 7});
    doodadCollectionTile->addItemDirect(makeOwnedItem(registry, grassTufts, 400));
    const auto doodadCollectionSelection = controller.resolveBrushFromTile(*doodadCollectionTile, MapEditor::Brushes::BrushPickMode::Collection);
    require(doodadCollectionSelection.has_value() && doodadCollectionSelection->brush == &grassTufts,
            "collection context selection did not resolve doodad brush");

    // Duplicate protection verification
    const MapEditor::Domain::Position dupPos{1, 1, 7};
    auto *gTile = map.getOrCreateTile(dupPos);
    gTile->setGround(std::make_unique<MapEditor::Domain::Item>(100));
    controller.setBrush(&grassTufts);
    require(controller.applyBrush(dupPos), "first doodad placement failed");
    const auto *dupTile1 = map.getTile(dupPos);
    require(dupTile1 != nullptr && dupTile1->getItemCount() == 1, "first doodad placement item count mismatch");

    // Second apply on same tile should be ignored (duplicate own item protection)
    require(!controller.applyBrush(dupPos), "second doodad placement should have been skipped");
    const auto *dupTile2 = map.getTile(dupPos);
    require(dupTile2 != nullptr && dupTile2->getItemCount() == 1, "doodad was duplicated or modified on existing tile");

    std::cout << "DoodadContextSmoke passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "DoodadContextSmoke failed: " << e.what() << "\n";
    return 1;
  }
}
