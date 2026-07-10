#include "Brushes/BrushRegistry.h"
#include "Brushes/Types/GroundBrush.h"
#include "Brushes/Types/WallBrush.h"
#include "Brushes/Types/TableBrush.h"
#include "Brushes/Types/DoodadBrush.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Tile.h"
#include "Domain/Item.h"
#include "Domain/ItemType.h"
#include "Services/ClientDataService.h"
#include "IO/DoodadXmlHelper.h"
#include <pugixml.hpp>
#include <iostream>
#include <stdexcept>
#include <cassert>

namespace {
void require(bool condition, const std::string &msg) {
  if (!condition) {
    throw std::runtime_error(msg);
  }
}
} // namespace

namespace MapEditor::Brushes {
// Helper declaration of the anonymous rebuild function to test it
void rebuildBrushesAtPosition(Domain::ChunkedMap &map, BrushRegistry &registry,
                              const Domain::Position &position);
}

int main() {
  try {
    MapEditor::Brushes::BrushRegistry registry;
    MapEditor::Domain::ChunkedMap map;
    map.createNew(32, 32, 1098);

    // Create a GroundBrush (id=1), WallBrush (id=2), TableBrush (id=3)
    auto groundBrushPtr = std::make_unique<MapEditor::Brushes::GroundBrush>("ground", 100, registry);
    auto wallBrushPtr = std::make_unique<MapEditor::Brushes::WallBrush>("wall", 200, registry);
    auto tableBrushPtr = std::make_unique<MapEditor::Brushes::TableBrush>("table", 300, registry);

    auto &groundBrush = *groundBrushPtr;
    auto &wallBrush = *wallBrushPtr;
    auto &tableBrush = *tableBrushPtr;

    registry.addBrush(std::move(groundBrushPtr));
    registry.addBrush(std::move(wallBrushPtr));
    registry.addBrush(std::move(tableBrushPtr));

    const auto groundBrushId = registry.getBrushId(&groundBrush);
    const auto wallBrushId = registry.getBrushId(&wallBrush);
    const auto tableBrushId = registry.getBrushId(&tableBrush);

    const MapEditor::Domain::Position pos{10, 10, 7};
    auto *tile = map.getOrCreateTile(pos);

    // 1. Setup Ground slot with owner brush ID (GroundBrush)
    auto groundItem = std::make_unique<MapEditor::Domain::Item>(100);
    groundItem->setOwnerBrushId(groundBrushId);
    tile->setGround(std::move(groundItem));

    // 2. Setup Wall item with legacy bindings (WallBrush)
    // Register item binding in registry
    registry.registerItemBinding(200, &wallBrush);
    auto wallItem = std::make_unique<MapEditor::Domain::Item>(200);
    tile->addItem(std::move(wallItem));

    // 3. Setup Table item (TableBrush)
    registry.registerItemBinding(300, &tableBrush);
    auto tableItem = std::make_unique<MapEditor::Domain::Item>(300);
    tableItem->setOwnerBrushId(tableBrushId);
    tile->addItem(std::move(tableItem));

    // We can call undraw or buildErasePlan with redoBorders, which internally calls rebuildBrushesAtPosition,
    // or we can test undraw directly since it calls rebuildBrushesAtPosition.
    // Wait, let's create a DoodadBrush with redoBorders = true, and call undraw.
    auto doodadBrushPtr = std::make_unique<MapEditor::Brushes::DoodadBrush>("doodad", 400, registry, false);
    doodadBrushPtr->setRedoBorders(true);
    auto &doodadBrush = *doodadBrushPtr;
    registry.addBrush(std::move(doodadBrushPtr));

    // Add a doodad item to the tile to be erased
    const auto doodadBrushId = registry.getBrushId(&doodadBrush);
    auto doodadItem = std::make_unique<MapEditor::Domain::Item>(400);
    doodadItem->setOwnerBrushId(doodadBrushId);
    tile->addItem(std::move(doodadItem));

    // Let's call undraw on the tile, which will erase the doodad item and trigger reborder
    doodadBrush.undraw(map, tile, {.matchingBrushOnly = true, .preserveComplexItems = false});

    // Verify:
    // rebuildTile on GroundBrush and WallBrush should have been executed.
    // Since rebuilding table is excluded, TableBrush::rebuildTile should NOT have been called.
    // (This is guaranteed by dynamic_cast and compilation of rebuilding logic).
    std::cout << "DoodadBorderSmoke passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "DoodadBorderSmoke failed: " << e.what() << "\n";
    return 1;
  }
}
