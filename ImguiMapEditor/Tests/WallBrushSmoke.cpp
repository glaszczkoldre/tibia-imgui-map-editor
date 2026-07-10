/**
 * @file WallBrushSmoke.cpp
 * @brief Comprehensive WallBrush connection and parity tests.
 */

#include "Brushes/BrushController.h"
#include "Brushes/BrushRegistry.h"
#include "Brushes/Core/IBrush.h"
#include "Brushes/Types/WallBrush.h"
#include "Domain/ChunkedMap.h"
#include "Domain/History/HistoryManager.h"
#include "Domain/Item.h"
#include "Domain/Position.h"
#include "Domain/Tile.h"
#include "Services/Autoborder/AutoborderEngine.h"
#include "Services/Autoborder/PlacementIntent.h"
#include "Services/Autoborder/PlannedMutation.h"
#include "Services/BrushSettingsService.h"
#include "Services/Brushes/WallLookupService.h"
#include "Services/TilesetService.h"
#include <chrono>
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;
using namespace MapEditor::Brushes;
using namespace MapEditor::Domain;
using namespace MapEditor::Domain::History;
using namespace MapEditor::Services::Autoborder;

namespace {

void require(bool condition, std::string_view message) {
  if (!condition) {
    throw std::runtime_error(std::string(message));
  }
}

IBrush *findBrush(BrushRegistry &registry, std::string_view name, BrushType expected) {
  auto *brush = registry.getBrush(std::string(name));
  require(brush != nullptr, std::string(name).append(" brush missing"));
  require(brush->getType() == expected,
          std::string(name).append(" brush has unexpected type"));
  return brush;
}

std::unique_ptr<Item> makeOwnedItem(BrushRegistry &registry, const IBrush &brush, uint16_t itemId) {
  auto item = std::make_unique<Item>(itemId);
  item->setOwnerBrushId(registry.getBrushId(&brush));
  return item;
}

std::vector<uint16_t> tileItemIds(const Tile *tile) {
  std::vector<uint16_t> ids;
  if (!tile) {
    return ids;
  }
  for (const auto &item : tile->getItems()) {
    if (item) {
      ids.push_back(item->getServerId());
    }
  }
  return ids;
}

bool tileContainsItemId(const Tile &tile, uint16_t itemId) {
  for (const auto &item : tile.getItems()) {
    if (item && item->getServerId() == itemId) {
      return true;
    }
  }
  return false;
}

void requireNeighborMasks(ChunkedMap &map, BrushRegistry &registry,
                          WallBrush &brush, IBrush *groundBrush) {
  // Clear map region
  for (int y = 0; y < 10; ++y) {
    for (int x = 0; x < 10; ++x) {
      Position pos{x, y, 7};
      map.setTile(pos, nullptr);
      auto *tile = map.getOrCreateTile(pos);
      tile->addItemDirect(makeOwnedItem(registry, *groundBrush, 351)); // Cave ground
    }
  }

  // Verify all 16 neighbor mask combinations (North=1, West=2, East=4, South=8)
  // We place a wall at center (5, 5) and set up neighbors to trigger each mask.
  const Position center{5, 5, 7};
  MapEditor::Services::Brushes::WallLookupService lookup;

  for (uint8_t mask = 0; mask < 16; ++mask) {
    // Clean up wall items on center and card neighbors
    for (const auto &pos : std::array{
             center,
             Position{5, 4, 7}, // N
             Position{4, 5, 7}, // W
             Position{6, 5, 7}, // E
             Position{5, 6, 7}  // S
         }) {
      if (auto *tile = map.getTile(pos)) {
        tile->removeItemsIf([&brush](const Item *item) { return brush.ownsItem(item); });
      }
    }

    // Place neighbor walls based on mask bits
    if (mask & 1) { // North
      map.getTile(5, 4, 7)->addItemDirect(makeOwnedItem(registry, brush, brush.getWallItemForAlign(WallAlign::Vertical)));
    }
    if (mask & 2) { // West
      map.getTile(4, 5, 7)->addItemDirect(makeOwnedItem(registry, brush, brush.getWallItemForAlign(WallAlign::Horizontal)));
    }
    if (mask & 4) { // East
      map.getTile(6, 5, 7)->addItemDirect(makeOwnedItem(registry, brush, brush.getWallItemForAlign(WallAlign::Horizontal)));
    }
    if (mask & 8) { // South
      map.getTile(5, 6, 7)->addItemDirect(makeOwnedItem(registry, brush, brush.getWallItemForAlign(WallAlign::Vertical)));
    }

    // Place center wall
    map.getTile(center)->addItemDirect(makeOwnedItem(registry, brush, brush.getWallItemForAlign(WallAlign::Horizontal)));

    // Rebuild center wall connection
    brush.rebuildTile(map, center);

    auto expectedAlign = lookup.getFullType(static_cast<WallNeighbor>(mask));
    auto expectedItem = brush.getWallItemForAlign(expectedAlign);
    if (expectedItem == 0) {
      expectedAlign = lookup.getHalfType(static_cast<WallNeighbor>(mask));
      expectedItem = brush.getWallItemForAlign(expectedAlign);
    }
    if (expectedItem == 0) {
      expectedAlign = WallAlign::Horizontal;
      expectedItem = brush.getWallItemForAlign(expectedAlign);
    }

    const auto *tile = map.getTile(center);
    require(tile != nullptr, "Center tile missing");
    if (!tileContainsItemId(*tile, expectedItem)) {
      std::cerr << "Wall alignment failed at mask=" << static_cast<int>(mask)
                << ", expected align=" << static_cast<int>(expectedAlign)
                << ", expected item=" << expectedItem << "\n";
      std::cerr << "Actual items on center tile:\n";
      for (const auto &item : tile->getItems()) {
        std::cerr << "  itemId=" << item->getServerId()
                  << ", owner=" << item->getOwnerBrushId() << "\n";
      }
      require(false, "Wall alignment did not resolve to the expected mask item");
    }
  }
}

void requireUntouchables(ChunkedMap &map, BrushRegistry &registry,
                        WallBrush &brush, IBrush *groundBrush) {
  // Clear tile (10, 10)
  const Position pos{10, 10, 7};
  auto *tile = map.getOrCreateTile(pos);
  tile->removeItemsIf([](const Item*) { return true; });
  tile->addItemDirect(makeOwnedItem(registry, *groundBrush, 351));

  // Place an untouchable wall item directly (e.g. Pole or Untouchable variant)
  const auto untouchableItem = brush.getWallItemForAlign(WallAlign::Untouchable);
  if (untouchableItem == 0) {
    return; // Skip if untouchable item is not registered in this version
  }

  tile->addItemDirect(makeOwnedItem(registry, brush, untouchableItem));

  // Rebuild neighbor around it
  const Position neighborPos{10, 11, 7};
  auto *neighborTile = map.getOrCreateTile(neighborPos);
  neighborTile->addItemDirect(makeOwnedItem(registry, *groundBrush, 351));
  neighborTile->addItemDirect(makeOwnedItem(registry, brush, brush.getWallItemForAlign(WallAlign::Vertical)));

  brush.rebuildAround(map, neighborPos);

  // Untouchable should not be modified
  require(tileContainsItemId(*tile, untouchableItem), "Untouchable wall item was modified during neighbor rebuild");
}

void requireRedirectsAndHateFlags(ChunkedMap &map, BrushRegistry &registry,
                                  WallBrush &brush, WallBrush &mossyWallDeco,
                                  IBrush *groundBrush) {
  const Position pos1{20, 20, 7};
  const Position pos2{20, 19, 7};

  for (const auto &p : {pos1, pos2}) {
    auto *tile = map.getOrCreateTile(p);
    tile->removeItemsIf([](const Item*) { return true; });
    tile->addItemDirect(makeOwnedItem(registry, *groundBrush, 351));
  }

  // Redirects: WallDecorationBrush (mossy wall) redirects connections to WallBrush (stone wall)
  map.getTile(pos1)->addItemDirect(makeOwnedItem(registry, brush, brush.getWallItemForAlign(WallAlign::Vertical)));
  map.getTile(pos2)->addItemDirect(makeOwnedItem(registry, mossyWallDeco, mossyWallDeco.getWallItemForAlign(WallAlign::Vertical)));

  // Rebuild pos1. Since mossy wall is a redirect friend, pos1 should detect pos2 as a connected wall.
  brush.rebuildTile(map, pos1);
  // Rebuilt item on pos1 should align vertically (North-South connection) instead of horizontal/alone fallback
  require(tileContainsItemId(*map.getTile(pos1), brush.getWallItemForAlign(WallAlign::Vertical)),
          "Wall brush did not connect to redirect friend decoration brush");

  // Hate flags: Add a hate item to pos2 and verify it breaks connection
  const auto hateItems = brush.getWallHateMeItems();
  if (!hateItems.empty()) {
    const uint16_t hateItem = *hateItems.begin();
    map.getTile(pos2)->addItemDirect(std::make_unique<Item>(hateItem));
    brush.rebuildTile(map, pos1);
    // Connection should be broken, so pos1 falls back to horizontal/alone
    require(!tileContainsItemId(*map.getTile(pos1), brush.getWallItemForAlign(WallAlign::Vertical)),
            "Wall brush connection was not broken by wall-hate-me item");
  }
}

void requireDoors(ChunkedMap &map, BrushRegistry &registry,
                  WallBrush &brush, IBrush *groundBrush) {
  const Position pos{30, 30, 7};
  auto *tile = map.getOrCreateTile(pos);
  tile->removeItemsIf([](const Item*) { return true; });
  tile->addItemDirect(makeOwnedItem(registry, *groundBrush, 351));

  // Add a West neighbor to make the wall at pos align as Horizontal
  const Position westPos{29, 30, 7};
  auto *westTile = map.getOrCreateTile(westPos);
  westTile->removeItemsIf([](const Item*) { return true; });
  westTile->addItemDirect(makeOwnedItem(registry, *groundBrush, 351));
  westTile->addItemDirect(makeOwnedItem(registry, brush, brush.getWallItemForAlign(WallAlign::Horizontal)));

  // Initialize controller
  HistoryManager history;
  BrushController controller;
  controller.initialize(&map, &history, nullptr);
  controller.setBrushRegistry(&registry);

  // Paint base wall
  controller.setBrush(&brush);
  require(controller.applyBrush(pos), "Base wall paint failed");

  // Paint door on wall
  controller.activateMagicDoorBrush();
  require(controller.applyBrush(pos), "Magic door paint failed");

  tile = map.getTile(pos);
  require(tile != nullptr, "Tile missing after magic door paint");

  // Check magic door item was placed
  if (!tileContainsItemId(*tile, 6265)) {
    std::cerr << "Door item 6265 check failed. Actual items on tile:\n";
    for (const auto &item : tile->getItems()) {
      std::cerr << "  itemId=" << item->getServerId() << "\n";
    }
    require(false, "Door item 6265 not placed on wall");
  }

  // Switch door state
  require(controller.canSwitchDoorAt(pos), "Cannot switch door at position");
  require(controller.switchDoorAt(pos), "Switch door failed");

  tile = map.getTile(pos);
  require(tile != nullptr, "Tile missing after switch door");

  // Should have toggled open/closed state
  require(!tileContainsItemId(*tile, 6265), "Magic door variant did not switch");
}

void requireContextPicking(ChunkedMap &map, BrushRegistry &registry,
                           WallBrush &brush, IBrush *groundBrush) {
  const Position pos{40, 40, 7};
  auto *tile = map.getOrCreateTile(pos);
  tile->removeItemsIf([](const Item*) { return true; });
  tile->addItemDirect(makeOwnedItem(registry, *groundBrush, 351));

  // Place door item
  tile->addItemDirect(makeOwnedItem(registry, brush, 6265)); // magic door

  HistoryManager history;
  BrushController controller;
  controller.initialize(&map, &history, nullptr);
  controller.setBrushRegistry(&registry);

  // Smart context pick
  const auto selection = controller.resolveBrushFromTile(*tile, BrushPickMode::Smart);
  require(selection.has_value(), "Smart context picking failed on door tile");
  require(selection->brush == &brush, "Smart picking resolved to door brush instead of owning wall brush");
}

void requireSegmentedVsSingleDragParity(ChunkedMap &map, BrushRegistry &registry,
                                       WallBrush &brush, IBrush *groundBrush) {
  // Clear map area
  for (int x = 50; x < 60; ++x) {
    for (int y = 50; y < 60; ++y) {
      Position p{x, y, 7};
      map.setTile(p, nullptr);
      map.getOrCreateTile(p)->addItemDirect(makeOwnedItem(registry, *groundBrush, 351));
    }
  }

  const std::array path{
      Position{51, 51, 7},
      Position{51, 52, 7},
      Position{51, 53, 7},
      Position{52, 53, 7},
      Position{53, 53, 7}
  };

  // Run segmented drag
  HistoryManager historySegmented;
  BrushController controllerSegmented;
  controllerSegmented.initialize(&map, &historySegmented, nullptr);
  controllerSegmented.setBrushRegistry(&registry);
  controllerSegmented.setBrush(&brush);

  controllerSegmented.beginStroke();
  for (const auto &p : path) {
    controllerSegmented.continueStroke(p);
  }
  controllerSegmented.endStroke();

  auto segmentedMapCopy = map.clone();

  // Reset map area
  for (int x = 50; x < 60; ++x) {
    for (int y = 50; y < 60; ++y) {
      Position p{x, y, 7};
      map.setTile(p, nullptr);
      map.getOrCreateTile(p)->addItemDirect(makeOwnedItem(registry, *groundBrush, 351));
    }
  }

  // Run single batch drag plan
  AutoborderEngine engine;
  PlacementIntent intent;
  intent.brush = &brush;
  intent.mode = PlacementMode::Draw;
  intent.context.brushRegistry = &registry;
  intent.context.ownerBrushId = registry.getBrushId(&brush);
  intent.context.isDragging = true;
  intent.positions.assign(path.begin(), path.end());

  auto diffs = engine.plan(map, intent);
  for (const auto &diff : diffs) {
    if (diff.after) {
      map.setTile(diff.position, diff.after->clone());
    }
  }

  // Compare results
  for (int x = 50; x < 60; ++x) {
    for (int y = 50; y < 60; ++y) {
      Position p{x, y, 7};
      const auto *tile1 = map.getTile(p);
      const auto *tile2 = segmentedMapCopy->getTile(p);
      if (tile1 || tile2) {
        require(tile1 != nullptr && tile2 != nullptr, "Drag parity failed (missing tile mismatch)");
        require(tileItemIds(tile1) == tileItemIds(tile2), "Drag parity failed (items mismatch)");
      }
    }
  }
}

void requireWallPerformance(ChunkedMap &map, BrushRegistry &registry,
                            WallBrush &brush, IBrush *groundBrush) {
  // Setup 1,000 tiles
  std::vector<Position> performancePath;
  performancePath.reserve(1000);
  for (int i = 0; i < 1000; ++i) {
    Position p{static_cast<int32_t>(100 + (i % 10)), static_cast<int32_t>(100 + (i / 10)), 7};
    map.getOrCreateTile(p)->addItemDirect(makeOwnedItem(registry, *groundBrush, 351));
    performancePath.push_back(p);
  }

  HistoryManager history;
  BrushController controller;
  controller.initialize(&map, &history, nullptr);
  controller.setBrushRegistry(&registry);
  controller.setBrush(&brush);

  const auto startTime = std::chrono::high_resolution_clock::now();

  controller.beginStroke();
  for (const auto &p : performancePath) {
    controller.continueStroke(p);
  }
  controller.endStroke();

  const auto endTime = std::chrono::high_resolution_clock::now();
  const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

  std::cout << "[WallBrushSmoke] Paint 1,000 tiles elapsed: " << elapsedMs << "ms\n";
  require(elapsedMs < 1000, "Wall brush 1,000-tile stroke took longer than 1 second (regression)");
}

} // namespace

int main() {
  try {
    const fs::path sourceRoot = WALL_SMOKE_SOURCE_DIR;
    const fs::path dataPath = sourceRoot.parent_path() / "data" / "1098";
    require(fs::exists(dataPath / "materials.xml"),
            "data/1098/materials.xml is missing");

    BrushRegistry registry;
    MapEditor::Services::TilesetService tilesetService(registry);
    require(tilesetService.loadMaterials(dataPath), "materials load failed");

    auto *groundBrush = findBrush(registry, "cave", BrushType::Ground);
    auto *stoneWallBrush = findBrush(registry, "stone wall", BrushType::Wall);
    auto *stoneWall = dynamic_cast<WallBrush *>(stoneWallBrush);
    require(stoneWall != nullptr, "stone wall brush cast failed");

    auto *mossyWallDecoBrush = findBrush(registry, "mossy wall", BrushType::WallDecoration);
    auto *mossyWallDeco = dynamic_cast<WallBrush *>(mossyWallDecoBrush);
    require(mossyWallDeco != nullptr, "mossy wall deco cast failed");

    std::cout << "Stone Wall registered alignments:\n";
    for (int a = 0; a < 17; ++a) {
      std::cout << "  Align " << a << ": " << stoneWall->getWallItemForAlign(static_cast<WallAlign>(a)) << "\n";
    }

    ChunkedMap map;
    map.createNew(512, 512, 1098);

    // 1. Neighbor masks (16 configs)
    requireNeighborMasks(map, registry, *stoneWall, groundBrush);

    // 2. Untouchables
    requireUntouchables(map, registry, *stoneWall, groundBrush);

    // 3. Redirects & Hate flags
    requireRedirectsAndHateFlags(map, registry, *stoneWall, *mossyWallDeco, groundBrush);

    // 4. Doors/Windows
    requireDoors(map, registry, *stoneWall, groundBrush);

    // 5. Context Picking
    requireContextPicking(map, registry, *stoneWall, groundBrush);

    // 6. Segmented vs Single-Update Drag Parity
    requireSegmentedVsSingleDragParity(map, registry, *stoneWall, groundBrush);

    // 7. Performance (1,000-tile drag)
    requireWallPerformance(map, registry, *stoneWall, groundBrush);

    std::cout << "WallBrushSmoke passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "WallBrushSmoke failed: " << ex.what() << "\n";
    return 1;
  }
}
