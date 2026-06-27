#include "Brushes/BrushRegistry.h"
#include "Brushes/Types/DoodadBrush.h"
#include "Brushes/Types/DoodadPlacementPlanner.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Tile.h"
#include "Domain/Item.h"
#include "Domain/ItemType.h"
#include "Services/BrushSettingsService.h"
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <algorithm>

namespace {
void require(bool condition, const std::string &msg) {
  if (!condition) {
    throw std::runtime_error(msg);
  }
}

bool containsPosition(const std::vector<MapEditor::Domain::Position> &positions,
                      const MapEditor::Domain::Position &position) {
  return std::find(positions.begin(), positions.end(), position) !=
         positions.end();
}
} // namespace

int main() {
  try {
    MapEditor::Brushes::BrushRegistry registry;
    MapEditor::Domain::ChunkedMap map;
    map.createNew(32, 32, 1098);

    MapEditor::Services::BrushSettingsService settings;
    settings.setStandardSize(0);

    // Create a custom DoodadBrush manually
    auto brushPtr = std::make_unique<MapEditor::Brushes::DoodadBrush>(
        "plan_test_brush", 100, registry, false);
    
    // Add a single alternative with one composite of 2 tiles and a zero-chance single item
    MapEditor::Brushes::DoodadAlternative alt;
    
    // Composite: tile (0,0,0) and tile (1,0,0)
    MapEditor::Brushes::CompositeItem composite;
    composite.chance = 10;
    
    MapEditor::Brushes::CompositeItem::TileOffset offset1{.dx = 0, .dy = 0, .dz = 0};
    offset1.items.push_back({.itemId = 100, .chance = 10});
    composite.tiles.push_back(offset1);

    MapEditor::Brushes::CompositeItem::TileOffset offset2{.dx = 1, .dy = 0, .dz = 0};
    offset2.items.push_back({.itemId = 101, .chance = 10});
    composite.tiles.push_back(offset2);
    
    alt.addComposite(composite);

    // Single item with 0 chance
    alt.addSingleItem({.itemId = 200, .chance = 0});

    brushPtr->addAlternative(std::move(alt));
    auto &brush = *brushPtr;
    registry.addBrush(std::move(brushPtr));

    const MapEditor::Domain::Position center{10, 10, 7};
    const uint32_t seed = 9999;

    // 1. Verify stable raw footprint & zero-chance is not placed
    {
      auto rawStamp = MapEditor::Brushes::DoodadPlacementPlanner::generateRawStamp(brush, &settings, 0, seed);
      require(!rawStamp.empty(), "generateRawStamp returned empty layout");
      
      // Should have composite items (itemIds 100 and 101), should NEVER have itemId 200 (since chance is 0)
      for (const auto &tile : rawStamp) {
        for (const auto &item : tile.items) {
          require(item.itemId != 200, "zero chance item was placed!");
        }
      }
    }

    // 2. Verify Phase 2 map projection filters composite tiles independently
    {
      // Placed at center on empty map: both tiles should succeed
      auto planEmpty = brush.buildPlacementPlan(center, &settings, 0, &map, false, seed);
      require(planEmpty.layout.size() == 2, "empty map layout should have 2 tiles");
      require(planEmpty.skipped.empty(), "empty map plan should have no skips");

      // Place a duplicate own item at (11, 10, 7) (which is center + relative x=1)
      auto *dupTile = map.getOrCreateTile({11, 10, 7});
      auto dupItem = std::make_unique<MapEditor::Domain::Item>(101);
      dupItem->setOwnerBrushId(registry.getBrushId(&brush));
      dupTile->addItem(std::move(dupItem));

      // Build plan again: center (10, 10, 7) should still place tile (0, 0)
      auto planDup = brush.buildPlacementPlan(center, &settings, 0, &map, false, seed);
      
      // Since center + relative (1,0) is duplicate, it should be filtered out independently
      // leaving only 1 tile placed!
      require(planDup.layout.size() == 1, "duplicate map layout should filter duplicate tile independently, remaining size: " + std::to_string(planDup.layout.size()));
      require(planDup.layout[0].relativePosition.x == 0 && planDup.layout[0].relativePosition.y == 0, "incorrect tile remained");
      require(planDup.skipped.size() == 1, "should report exactly 1 skip");
      require(planDup.skipped[0].reason == MapEditor::Brushes::DoodadBrush::PlacementSkipReason::DuplicateOwnItem, "incorrect skip reason");
      require(planDup.skipped[0].position == MapEditor::Domain::Position({11, 10, 7}), "incorrect skip position");
    }

    // 3. Verify erasing uses the unfiltered raw stamp footprint
    {
      // Erase plan on center (10, 10, 7)
      auto erasePlan = brush.buildErasePlan(center, &settings, 0, &map, false, seed, {});
      
      // The erase plan positions must include BOTH (10, 10, 7) and (11, 10, 7)
      // despite (11, 10, 7) being occupied/duplicate or blocked!
      require(erasePlan.positions.size() == 2, "erase plan should include both stamp positions");
      require(containsPosition(erasePlan.positions, {10, 10, 7}), "missing center from erase plan");
      require(containsPosition(erasePlan.positions, {11, 10, 7}), "missing offset position from erase plan");
    }

    std::cout << "DoodadPlanningSmoke passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "DoodadPlanningSmoke failed: " << e.what() << "\n";
    return 1;
  }
}
