#include "Brushes/BrushController.h"
#include "Brushes/BrushRegistry.h"
#include "Brushes/Behaviors/WeightedSelection.h"
#include "Brushes/Core/IBrush.h"
#include "Brushes/Data/BorderBlock.h"
#include "Brushes/Types/DoodadBrush.h"
#include "Brushes/Types/GroundBrush.h"
#include "Brushes/Types/RawBrush.h"
#include "Brushes/Types/WallBrush.h"
#include "Application/EditorSession.h"
#include "Controllers/MapInputController.h"
#include "Domain/ChunkedMap.h"
#include "Domain/History/HistoryManager.h"
#include "Domain/Item.h"
#include "Domain/ItemType.h"
#include "Domain/MapInstance.h"
#include "Domain/SelectionSettings.h"
#include "Domain/Tileset/TilesetEntry.h"
#include "IO/HouseXmlReader.h"
#include "IO/BrushXmlReader.h"
#include "IO/HouseXmlWriter.h"
#include "IO/Otbm/OtbmReader.h"
#include "IO/Otbm/OtbmWriter.h"
#include "IO/SpawnXmlReader.h"
#include "IO/SpawnXmlWriter.h"
#include "IO/TilesetXmlReader.h"
#include "Services/Brushes/BorderLookupService.h"
#include "Services/Brushes/CarpetLookupService.h"
#include "Services/Brushes/TableLookupService.h"
#include "Services/Brushes/WallLookupService.h"
#include "Services/Autoborder/AutoborderEngine.h"
#include "Services/Autoborder/TileDiff.h"
#include "Services/BrushSettingsService.h"
#include "Services/HotkeyRegistry.h"
#include "Services/Preview/BrushPreviewFactory.h"
#include "Services/Preview/IPreviewProvider.h"
#include "Services/Preview/PreviewService.h"
#include "Services/Selection/SelectionService.h"
#include "Services/TilesetService.h"
#include "UI/Utils/BrushPreviewResolver.h"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace fs = std::filesystem;

namespace {

using MapBrushType = MapEditor::Brushes::BrushType;
using PickMode = MapEditor::Brushes::BrushPickMode;
using EdgeType = MapEditor::Brushes::EdgeType;
using DoorType = MapEditor::Brushes::DoorType;
using TableAlign = MapEditor::Brushes::TableAlign;
using TileNeighbor = MapEditor::Brushes::TileNeighbor;
using WallAlign = MapEditor::Brushes::WallAlign;
using WallNeighbor = MapEditor::Brushes::WallNeighbor;

static_assert(static_cast<uint8_t>(EdgeType::N) == 1);
static_assert(static_cast<uint8_t>(EdgeType::E) == 2);
static_assert(static_cast<uint8_t>(EdgeType::S) == 3);
static_assert(static_cast<uint8_t>(EdgeType::W) == 4);
static_assert(static_cast<uint8_t>(EdgeType::CNW) == 5);
static_assert(static_cast<uint8_t>(EdgeType::CNE) == 6);
static_assert(static_cast<uint8_t>(EdgeType::CSW) == 7);
static_assert(static_cast<uint8_t>(EdgeType::CSE) == 8);
static_assert(static_cast<uint8_t>(EdgeType::DNW) == 9);
static_assert(static_cast<uint8_t>(EdgeType::DNE) == 10);
static_assert(static_cast<uint8_t>(EdgeType::DSE) == 11);
static_assert(static_cast<uint8_t>(EdgeType::DSW) == 12);
static_assert(static_cast<uint8_t>(TableAlign::North) == 0);
static_assert(static_cast<uint8_t>(TableAlign::South) == 1);
static_assert(static_cast<uint8_t>(TableAlign::East) == 2);
static_assert(static_cast<uint8_t>(TableAlign::West) == 3);
static_assert(static_cast<uint8_t>(TableAlign::Horizontal) == 4);
static_assert(static_cast<uint8_t>(TableAlign::Vertical) == 5);
static_assert(static_cast<uint8_t>(TableAlign::Alone) == 6);
static_assert(static_cast<uint8_t>(WallAlign::Pole) == 0);
static_assert(static_cast<uint8_t>(WallAlign::SouthEnd) == 1);
static_assert(static_cast<uint8_t>(WallAlign::EastEnd) == 2);
static_assert(static_cast<uint8_t>(WallAlign::NorthwestDiagonal) == 3);
static_assert(static_cast<uint8_t>(WallAlign::WestEnd) == 4);
static_assert(static_cast<uint8_t>(WallAlign::NortheastDiagonal) == 5);
static_assert(static_cast<uint8_t>(WallAlign::Horizontal) == 6);
static_assert(static_cast<uint8_t>(WallAlign::SouthT) == 7);
static_assert(static_cast<uint8_t>(WallAlign::NorthEnd) == 8);
static_assert(static_cast<uint8_t>(WallAlign::Vertical) == 9);
static_assert(static_cast<uint8_t>(WallAlign::SouthwestDiagonal) == 10);
static_assert(static_cast<uint8_t>(WallAlign::EastT) == 11);
static_assert(static_cast<uint8_t>(WallAlign::SoutheastDiagonal) == 12);
static_assert(static_cast<uint8_t>(WallAlign::WestT) == 13);
static_assert(static_cast<uint8_t>(WallAlign::NorthT) == 14);
static_assert(static_cast<uint8_t>(WallAlign::Intersection) == 15);

void require(bool condition, std::string_view message) {
  if (!condition) {
    throw std::runtime_error(std::string(message));
  }
}

MapEditor::Brushes::IBrush *
findBrushByType(MapEditor::Brushes::BrushRegistry &registry,
                MapBrushType type) {
  for (auto *brush : registry.getAllBrushes()) {
    if (brush && brush->getType() == type) {
      return brush;
    }
  }
  return nullptr;
}

MapEditor::Brushes::IBrush *
findBrush(MapEditor::Brushes::BrushRegistry &registry, std::string_view preferred,
          MapBrushType fallbackType) {
  if (auto *brush = registry.getBrush(std::string(preferred))) {
    return brush;
  }
  return findBrushByType(registry, fallbackType);
}

bool tileContainsItemId(const MapEditor::Domain::Tile &tile, uint16_t itemId) {
  if (tile.hasGround() && tile.getGround()->getServerId() == itemId) {
    return true;
  }

  for (const auto &item : tile.getItems()) {
    if (item && item->getServerId() == itemId) {
      return true;
    }
  }

  return false;
}

size_t countItemId(const MapEditor::Domain::Tile &tile, uint16_t itemId) {
  size_t count = tile.hasGround() && tile.getGround()->getServerId() == itemId
                     ? 1
                     : 0;
  for (const auto &item : tile.getItems()) {
    if (item && item->getServerId() == itemId) {
      ++count;
    }
  }
  return count;
}

MapEditor::Brushes::GroundBrush *
findGroundBrush(MapEditor::Brushes::BrushRegistry &registry,
                std::string_view name) {
  auto *brush = registry.getBrush(std::string(name));
  return dynamic_cast<MapEditor::Brushes::GroundBrush *>(brush);
}

void paintGround(MapEditor::Domain::ChunkedMap &map,
                 MapEditor::Brushes::BrushRegistry &registry,
                 MapEditor::Brushes::GroundBrush &brush,
                 const MapEditor::Domain::Position &pos) {
  MapEditor::Brushes::DrawContext ctx;
  ctx.brushRegistry = &registry;
  ctx.ownerBrushId = registry.getBrushId(&brush);
  brush.draw(map, map.getOrCreateTile(pos), ctx);
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

std::optional<size_t>
findFirstOwnedItemIndex(const MapEditor::Domain::Tile &tile,
                        const MapEditor::Brushes::IBrush &brush) {
  for (size_t index = 0; index < tile.getItemCount(); ++index) {
    if (brush.ownsItem(tile.getItem(index))) {
      return index;
    }
  }
  return std::nullopt;
}

size_t countTiles(const MapEditor::Domain::ChunkedMap &map) {
  size_t count = 0;
  map.forEachTile([&count](const MapEditor::Domain::Tile *) { ++count; });
  return count;
}

std::optional<MapEditor::Domain::Position>
findTileWithItemsNear(const MapEditor::Domain::ChunkedMap &map,
                      const MapEditor::Domain::Position &center, int radius) {
  for (int dy = -radius; dy <= radius; ++dy) {
    for (int dx = -radius; dx <= radius; ++dx) {
      const MapEditor::Domain::Position pos{center.x + dx, center.y + dy,
                                            center.z};
      const auto *tile = map.getTile(pos);
      if (tile && (tile->hasGround() || tile->getItemCount() > 0)) {
        return pos;
      }
    }
  }
  return std::nullopt;
}

void clearTileBrushOwnership(MapEditor::Domain::Tile &tile) {
  if (auto *ground = tile.getGround()) {
    ground->setOwnerBrushId(MapEditor::Brushes::InvalidBrushId);
  }
  for (const auto &item : tile.getItems()) {
    if (item) {
      item->setOwnerBrushId(MapEditor::Brushes::InvalidBrushId);
    }
  }

  tile.setGroundBrushId(MapEditor::Brushes::InvalidBrushId);
  tile.setOptionalBorderBrushId(MapEditor::Brushes::InvalidBrushId);
  tile.setSpawnBrushId(MapEditor::Brushes::InvalidBrushId);
  tile.setCreatureBrushId(MapEditor::Brushes::InvalidBrushId);
  tile.setHouseBrushId(MapEditor::Brushes::InvalidBrushId);
  tile.setHouseExitBrushId(MapEditor::Brushes::InvalidBrushId);
  tile.setWaypointBrushId(MapEditor::Brushes::InvalidBrushId);
  tile.setZoneBrushId(MapEditor::Domain::TileFlag::ProtectionZone,
                      MapEditor::Brushes::InvalidBrushId);
  tile.setZoneBrushId(MapEditor::Domain::TileFlag::NoPvp,
                      MapEditor::Brushes::InvalidBrushId);
  tile.setZoneBrushId(MapEditor::Domain::TileFlag::NoLogout,
                      MapEditor::Brushes::InvalidBrushId);
  tile.setZoneBrushId(MapEditor::Domain::TileFlag::PvpZone,
                      MapEditor::Brushes::InvalidBrushId);
  tile.setZoneBrushId(MapEditor::Domain::TileFlag::Refresh,
                      MapEditor::Brushes::InvalidBrushId);
}

std::optional<std::pair<uint16_t, uint16_t>>
findWallVariantPair(const MapEditor::Brushes::WallBrush &wallBrush) {
  constexpr std::array<WallAlign, 17> alignments{
      WallAlign::Pole,
      WallAlign::SouthEnd,
      WallAlign::EastEnd,
      WallAlign::NorthwestDiagonal,
      WallAlign::WestEnd,
      WallAlign::NortheastDiagonal,
      WallAlign::Horizontal,
      WallAlign::SouthT,
      WallAlign::NorthEnd,
      WallAlign::Vertical,
      WallAlign::SouthwestDiagonal,
      WallAlign::EastT,
      WallAlign::SoutheastDiagonal,
      WallAlign::WestT,
      WallAlign::NorthT,
      WallAlign::Intersection,
      WallAlign::Untouchable,
  };

  for (const auto alignment : alignments) {
    const auto itemId = wallBrush.getWallItemForAlign(alignment);
    if (itemId == 0) {
      continue;
    }

    if (const auto nextItemId = wallBrush.findNextWallVariant(itemId)) {
      return std::pair<uint16_t, uint16_t>{itemId, *nextItemId};
    }
  }

  return std::nullopt;
}

void writeTextFile(const fs::path &path, std::string_view content) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) {
    throw std::runtime_error("failed to create " + path.string());
  }
  output.write(content.data(), static_cast<std::streamsize>(content.size()));
}

void requireBorderSelectionUsesScopedSeed() {
  MapEditor::Brushes::BorderBlock block;
  block.addItem(EdgeType::N, 61000, 1);
  block.addItem(EdgeType::N, 61001, 1);

  for (uint32_t seed = 1; seed <= 64; ++seed) {
    std::mt19937 rng1(seed);
    uint32_t first = block.getRandomItem(EdgeType::N, rng1);

    std::mt19937 rng2(seed);
    uint32_t second = block.getRandomItem(EdgeType::N, rng2);

    require(first == second,
            "border item selection ignored scoped deterministic seed");
  }
}

void requireGroundBorderMissingAlignDefaultsOuter(const fs::path &tempDir) {
  const fs::path brushPath = tempDir / "ground_missing_align.xml";
  writeTextFile(
      brushPath,
      R"xml(<brushes>
  <brush name="missing-align-low" type="ground" lookid="61010" z-order="1">
    <item id="61010" />
  </brush>
  <brush name="missing-align-high" type="ground" lookid="61011" z-order="2">
    <item id="61011" />
    <border to="missing-align-low">
      <borderitem edge="n" id="61012" />
    </border>
  </brush>
</brushes>)xml");

  MapEditor::Brushes::BrushRegistry localRegistry;
  MapEditor::IO::BrushXmlReader reader({&localRegistry});
  require(reader.loadFile(brushPath), "missing-align ground brush XML failed");

  auto *lowBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("missing-align-low"));
  auto *highBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("missing-align-high"));
  require(lowBrush != nullptr && highBrush != nullptr,
          "missing-align ground brushes did not load");

  MapEditor::Domain::ChunkedMap map;
  map.createNew(16, 16, 1098);
  const MapEditor::Domain::Position north{8, 7, 7};
  const MapEditor::Domain::Position center{8, 8, 7};

  MapEditor::Brushes::DrawContext ctx;
  ctx.brushRegistry = &localRegistry;
  ctx.ownerBrushId = localRegistry.getBrushId(highBrush);
  highBrush->draw(map, map.getOrCreateTile(north), ctx);

  ctx.ownerBrushId = localRegistry.getBrushId(lowBrush);
  lowBrush->draw(map, map.getOrCreateTile(center), ctx);

  const auto *centerTile = map.getTile(center);
  require(centerTile != nullptr && tileContainsItemId(*centerTile, 61012),
          "missing align on ground border did not default to outer");
}

void requireGroundBorderZOrderMatchesRme(const fs::path &tempDir) {
  const fs::path brushPath = tempDir / "ground_z_order.xml";
  writeTextFile(
      brushPath,
      R"xml(<brushes>
  <brush name="z-low" type="ground" lookid="62010" z-order="1">
    <item id="62010" />
    <border align="inner" to="z-high">
      <borderitem edge="n" id="62012" />
    </border>
    <border align="outer" to="z-high">
      <borderitem edge="n" id="62013" />
    </border>
  </brush>
  <brush name="z-high" type="ground" lookid="62011" z-order="2">
    <item id="62011" />
    <border align="inner" to="z-low">
      <borderitem edge="n" id="62014" />
    </border>
    <border align="outer" to="z-low">
      <borderitem edge="n" id="62015" />
    </border>
  </brush>
</brushes>)xml");

  MapEditor::Brushes::BrushRegistry localRegistry;
  MapEditor::IO::BrushXmlReader reader({&localRegistry});
  require(reader.loadFile(brushPath), "z-order ground brush XML failed");

  auto *lowBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("z-low"));
  auto *highBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("z-high"));
  require(lowBrush != nullptr && highBrush != nullptr,
          "z-order ground brushes did not load");

  auto paint = [&](MapEditor::Domain::ChunkedMap &map,
                   MapEditor::Brushes::GroundBrush &brush,
                   const MapEditor::Domain::Position &pos) {
    MapEditor::Brushes::DrawContext ctx;
    ctx.brushRegistry = &localRegistry;
    ctx.ownerBrushId = localRegistry.getBrushId(&brush);
    brush.draw(map, map.getOrCreateTile(pos), ctx);
  };

  {
    MapEditor::Domain::ChunkedMap map;
    map.createNew(16, 16, 1098);
    const MapEditor::Domain::Position north{8, 7, 7};
    const MapEditor::Domain::Position center{8, 8, 7};
    paint(map, *highBrush, north);
    paint(map, *lowBrush, center);

    const auto *centerTile = map.getTile(center);
    require(centerTile != nullptr && tileContainsItemId(*centerTile, 62012),
            "low-center/high-neighbor should prefer low inner border");
    require(!tileContainsItemId(*centerTile, 62015),
            "low-center/high-neighbor used high outer before low inner");
  }

  {
    MapEditor::Domain::ChunkedMap map;
    map.createNew(16, 16, 1098);
    const MapEditor::Domain::Position north{8, 7, 7};
    const MapEditor::Domain::Position center{8, 8, 7};
    paint(map, *lowBrush, north);
    paint(map, *highBrush, center);

    const auto *centerTile = map.getTile(center);
    require(centerTile != nullptr && tileContainsItemId(*centerTile, 62014),
            "high-center/low-neighbor should use high inner border");
    require(!tileContainsItemId(*centerTile, 62013),
            "high-center/low-neighbor used low outer before high inner");
  }
}

void requireGroundFriendEnemySemanticsMatchRme(const fs::path &tempDir) {
  const fs::path brushPath = tempDir / "ground_friend_enemy.xml";
  writeTextFile(
      brushPath,
      R"xml(<brushes>
  <brush name="friend-owner" type="ground" lookid="63010" z-order="1">
    <item id="63010" />
    <friend name="friend-target" />
  </brush>
  <brush name="friend-target" type="ground" lookid="63011" z-order="2">
    <item id="63011" />
    <border align="inner" to="friend-owner">
      <borderitem edge="n" id="63012" />
    </border>
  </brush>
  <brush name="enemy-owner" type="ground" lookid="63020" z-order="1">
    <item id="63020" />
    <enemy name="blocked-target" />
  </brush>
  <brush name="friendly-target" type="ground" lookid="63021" z-order="2">
    <item id="63021" />
    <border align="inner" to="enemy-owner">
      <borderitem edge="n" id="63022" />
    </border>
  </brush>
  <brush name="blocked-target" type="ground" lookid="63023" z-order="2">
    <item id="63023" />
    <border align="inner" to="enemy-owner">
      <borderitem edge="n" id="63024" />
    </border>
  </brush>
  <brush name="enemy-all-owner" type="ground" lookid="63030" z-order="1">
    <item id="63030" />
    <enemy name="all" />
  </brush>
  <brush name="enemy-all-target" type="ground" lookid="63031" z-order="2">
    <item id="63031" />
    <border align="inner" to="enemy-all-owner">
      <borderitem edge="n" id="63032" />
    </border>
  </brush>
  <brush name="clear-owner" type="ground" lookid="63040" z-order="1">
    <item id="63040" />
    <friend name="all" />
    <clear_friends />
  </brush>
  <brush name="clear-target" type="ground" lookid="63041" z-order="2">
    <item id="63041" />
    <border align="inner" to="clear-owner">
      <borderitem edge="n" id="63042" />
    </border>
  </brush>
</brushes>)xml");

  MapEditor::Brushes::BrushRegistry localRegistry;
  MapEditor::IO::BrushXmlReader reader({&localRegistry});
  require(reader.loadFile(brushPath), "friend/enemy ground brush XML failed");

  auto getGround = [&](std::string_view name) -> MapEditor::Brushes::GroundBrush * {
    auto *brush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
        localRegistry.getBrush(std::string(name)));
    require(brush != nullptr,
            std::string(name).append(" ground brush did not load"));
    return brush;
  };

  auto setOptionalBorder = [](MapEditor::Brushes::GroundBrush &brush,
                              uint16_t itemId) {
    MapEditor::Brushes::BorderBlock optionalBorder;
    optionalBorder.addItem(EdgeType::N, itemId, 1);
    brush.setOptionalBorder(std::move(optionalBorder), false);
  };

  setOptionalBorder(*getGround("friend-owner"), 63110);
  setOptionalBorder(*getGround("enemy-owner"), 63120);
  setOptionalBorder(*getGround("enemy-all-owner"), 63130);
  setOptionalBorder(*getGround("clear-owner"), 63140);

  auto paint = [&](MapEditor::Domain::ChunkedMap &map,
                   MapEditor::Brushes::GroundBrush &brush,
                   const MapEditor::Domain::Position &pos) {
    MapEditor::Brushes::DrawContext ctx;
    ctx.brushRegistry = &localRegistry;
    ctx.ownerBrushId = localRegistry.getBrushId(&brush);
    brush.draw(map, map.getOrCreateTile(pos), ctx);
  };

  auto requireScenario = [&](std::string_view ownerName,
                             std::string_view targetName,
                             uint16_t borderItemId, bool shouldHaveBorder,
                             std::string_view label) {
    MapEditor::Domain::ChunkedMap map;
    map.createNew(16, 16, 1098);
    const MapEditor::Domain::Position north{8, 7, 7};
    const MapEditor::Domain::Position center{8, 8, 7};
    paint(map, *getGround(ownerName), north);
    paint(map, *getGround(targetName), center);

    const auto *centerTile = map.getTile(center);
    const bool hasBorder =
        centerTile && tileContainsItemId(*centerTile, borderItemId);
    require(hasBorder == shouldHaveBorder, label);
  };

  requireScenario("friend-owner", "friend-target", 63012, false,
                  "named friend should suppress normal border");
  requireScenario("enemy-owner", "friendly-target", 63022, false,
                  "named enemy should make unlisted targets friendly");
  requireScenario("enemy-owner", "blocked-target", 63024, true,
                  "named enemy target should still receive normal border");
  requireScenario("enemy-all-owner", "enemy-all-target", 63032, true,
                  "enemy all should make every target non-friendly");
  requireScenario("clear-owner", "clear-target", 63042, true,
                  "clear_friends should remove previous friend all");
}

void requireGroundInlineOptionalBorderXml(const fs::path &tempDir) {
  const fs::path brushPath = tempDir / "ground_inline_optional.xml";
  writeTextFile(
      brushPath,
      R"xml(<brushes>
  <brush name="inline-optional-owner" type="ground" lookid="64010" z-order="1">
    <item id="64010" />
    <friend name="inline-optional-target" />
    <optional ground_equivalent="64010">
      <borderitem edge="n" id="64012" />
    </optional>
  </brush>
  <brush name="inline-optional-target" type="ground" lookid="64011" z-order="2">
    <item id="64011" />
    <border align="inner" to="inline-optional-owner">
      <borderitem edge="n" id="64013" />
    </border>
  </brush>
</brushes>)xml");

  MapEditor::Brushes::BrushRegistry localRegistry;
  MapEditor::IO::BrushXmlReader reader({&localRegistry});
  require(reader.loadFile(brushPath), "inline optional ground brush XML failed");

  auto *ownerBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("inline-optional-owner"));
  auto *targetBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("inline-optional-target"));
  require(ownerBrush != nullptr && targetBrush != nullptr,
          "inline optional ground brushes did not load");

  MapEditor::Domain::ChunkedMap map;
  map.createNew(16, 16, 1098);
  const MapEditor::Domain::Position north{8, 7, 7};
  const MapEditor::Domain::Position center{8, 8, 7};

  auto paint = [&](MapEditor::Brushes::GroundBrush &brush,
                   const MapEditor::Domain::Position &pos) {
    MapEditor::Brushes::DrawContext ctx;
    ctx.brushRegistry = &localRegistry;
    ctx.ownerBrushId = localRegistry.getBrushId(&brush);
    brush.draw(map, map.getOrCreateTile(pos), ctx);
  };

  paint(*ownerBrush, north);
  paint(*targetBrush, center);

  auto *centerTile = map.getTile(center);
  require(centerTile != nullptr, "inline optional target tile missing");
  centerTile->setOptionalBorder(true);
  targetBrush->rebuildTile(map, center);

  require(tileContainsItemId(*centerTile, 64012),
          "inline optional ground_equivalent border item was not placed");
  require(!tileContainsItemId(*centerTile, 64013),
          "friendly inline optional border should suppress normal border");
}

void requireGroundOuterZilchMaterializesEmptyTile(const fs::path &tempDir) {
  const fs::path brushPath = tempDir / "ground_outer_zilch.xml";
  writeTextFile(
      brushPath,
      R"xml(<brushes>
  <brush name="outer-zilch-owner" type="ground" lookid="64500" z-order="10">
    <item id="64500" />
    <border align="outer" to="none" id="6451" />
  </brush>
</brushes>)xml");

  MapEditor::Brushes::BrushRegistry localRegistry;
  MapEditor::Brushes::BorderBlock borderBlock;
  borderBlock.addItem(EdgeType::N, 64510, 1);
  localRegistry.registerBorderTemplate(6451, std::move(borderBlock));

  MapEditor::IO::BrushXmlReader reader({&localRegistry});
  require(reader.loadFile(brushPath),
          "outer-zilch ground brush XML failed");

  auto *ownerBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("outer-zilch-owner"));
  require(ownerBrush != nullptr, "outer-zilch ground brush did not load");

  MapEditor::Domain::ChunkedMap map;
  map.createNew(16, 16, 1098);
  const MapEditor::Domain::Position north{8, 7, 7};
  const MapEditor::Domain::Position center{8, 8, 7};

  MapEditor::Brushes::DrawContext ctx;
  ctx.brushRegistry = &localRegistry;
  ctx.ownerBrushId = localRegistry.getBrushId(ownerBrush);
  ownerBrush->draw(map, map.getOrCreateTile(north), ctx);

  const auto *centerTile = map.getTile(center);
  require(centerTile != nullptr && !centerTile->hasGround(),
          "outer-zilch border did not materialize an empty target tile");
  require(tileContainsItemId(*centerTile, 64510),
          "outer-zilch border item was not placed on the empty target tile");
}

void requireGroundClearBordersMatchesRme(const fs::path &tempDir) {
  auto registerTemplate = [](MapEditor::Brushes::BrushRegistry &registry,
                             uint32_t templateId, uint16_t itemId,
                             EdgeType edge) {
    MapEditor::Brushes::BorderBlock block;
    block.addItem(edge, itemId, 1);
    registry.registerBorderTemplate(templateId, std::move(block));
  };

  const fs::path brushPath = tempDir / "ground_clear_borders.xml";
  writeTextFile(
      brushPath,
      R"xml(<brushes>
  <brush name="clear-border-owner" type="ground" lookid="64600" z-order="10">
    <item id="64600" />
    <border align="inner" to="clear-border-north" id="6461" />
    <clear_borders />
    <border align="inner" to="clear-border-east" id="6462" />
  </brush>
  <brush name="clear-border-north" type="ground" lookid="64601" z-order="1">
    <item id="64601" />
  </brush>
  <brush name="clear-border-east" type="ground" lookid="64602" z-order="1">
    <item id="64602" />
  </brush>
</brushes>)xml");

  MapEditor::Brushes::BrushRegistry localRegistry;
  registerTemplate(localRegistry, 6461, 64610, EdgeType::N);
  registerTemplate(localRegistry, 6462, 64620, EdgeType::E);
  MapEditor::IO::BrushXmlReader reader({&localRegistry});
  require(reader.loadFile(brushPath), "clear_borders ground brush XML failed");

  auto *ownerBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("clear-border-owner"));
  auto *northBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("clear-border-north"));
  auto *eastBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("clear-border-east"));
  require(ownerBrush != nullptr && northBrush != nullptr && eastBrush != nullptr,
          "clear_borders ground brushes did not load");
  require(localRegistry.getBrushForItem(64610) != ownerBrush,
          "clear_borders left cleared border item bound to owner brush");

  auto paint = [](MapEditor::Domain::ChunkedMap &map,
                  MapEditor::Brushes::BrushRegistry &registry,
                  MapEditor::Brushes::GroundBrush &brush,
                  const MapEditor::Domain::Position &pos) {
    MapEditor::Brushes::DrawContext ctx;
    ctx.brushRegistry = &registry;
    ctx.ownerBrushId = registry.getBrushId(&brush);
    brush.draw(map, map.getOrCreateTile(pos), ctx);
  };

  MapEditor::Domain::ChunkedMap map;
  map.createNew(16, 16, 1098);
  const MapEditor::Domain::Position north{8, 7, 7};
  const MapEditor::Domain::Position center{8, 8, 7};
  const MapEditor::Domain::Position east{9, 8, 7};

  paint(map, localRegistry, *northBrush, north);
  paint(map, localRegistry, *eastBrush, east);
  paint(map, localRegistry, *ownerBrush, center);

  const auto *centerTile = map.getTile(center);
  require(centerTile != nullptr && !tileContainsItemId(*centerTile, 64610),
          "clear_borders did not remove the earlier border rule");
  require(tileContainsItemId(*centerTile, 64620),
          "clear_borders prevented later border rules from applying");
}

void requireGroundTerrainPlacementUsesGroundEquivalent(const fs::path &tempDir) {
  const fs::path brushPath = tempDir / "ground_terrain_equivalent.xml";
  writeTextFile(
      brushPath,
      R"xml(<brushes>
  <brush name="terrain-equivalent-owner" type="ground" lookid="64700" z-order="10">
    <item id="64710" />
    <border align="inner" ground_equivalent="64700">
      <borderitem edge="n" id="64710" />
    </border>
  </brush>
  <brush name="terrain-template-equivalent-owner" type="ground" lookid="64700" z-order="10">
    <item id="64720" />
    <border align="inner" id="6471" ground_equivalent="64700" />
  </brush>
</brushes>)xml");

  MapEditor::Brushes::BrushRegistry localRegistry;
  MapEditor::Brushes::BorderBlock templateBlock;
  templateBlock.addItem(EdgeType::N, 64720, 1);
  localRegistry.registerBorderTemplate(6471, std::move(templateBlock));

  MapEditor::IO::BrushXmlReader reader({&localRegistry});
  require(reader.loadFile(brushPath),
          "terrain-equivalent ground brush XML failed");

  auto *ownerBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("terrain-equivalent-owner"));
  auto *templateOwnerBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("terrain-template-equivalent-owner"));
  require(ownerBrush != nullptr,
          "terrain-equivalent ground brush did not load");
  require(templateOwnerBrush != nullptr,
          "terrain template-equivalent ground brush did not load");

  MapEditor::Domain::ChunkedMap map;
  map.createNew(16, 16, 1098);
  const MapEditor::Domain::Position center{8, 8, 7};
  const MapEditor::Domain::Position east{9, 8, 7};

  MapEditor::Brushes::DrawContext ctx;
  ctx.brushRegistry = &localRegistry;
  ctx.ownerBrushId = localRegistry.getBrushId(ownerBrush);
  ownerBrush->draw(map, map.getOrCreateTile(center), ctx);

  const auto *centerTile = map.getTile(center);
  require(centerTile != nullptr && centerTile->hasGround(),
          "terrain-equivalent draw did not place ground");
  require(centerTile->getGround()->getServerId() == 64700,
          "terrain-equivalent draw did not convert border item to base ground");
  require(!tileContainsItemId(*centerTile, 64710),
          "terrain-equivalent draw left the border item on the tile");

  ctx.ownerBrushId = localRegistry.getBrushId(templateOwnerBrush);
  templateOwnerBrush->draw(map, map.getOrCreateTile(east), ctx);

  const auto *eastTile = map.getTile(east);
  require(eastTile != nullptr && eastTile->hasGround(),
          "terrain template-equivalent draw did not place ground");
  require(eastTile->getGround()->getServerId() == 64700,
          "terrain template-equivalent draw did not convert border item to base ground");
  require(!tileContainsItemId(*eastTile, 64720),
          "terrain template-equivalent draw left the border item on the tile");
}

void requireGroundSpecificCaseBorderActions(const fs::path &tempDir) {
  auto registerTemplate = [](MapEditor::Brushes::BrushRegistry &registry,
                             uint32_t templateId, uint16_t itemId,
                             EdgeType edge, uint16_t group = 0) {
    MapEditor::Brushes::BorderBlock block;
    block.setGroup(group);
    block.addItem(edge, itemId, 1);
    registry.registerBorderTemplate(templateId, std::move(block));
  };

  auto paint = [](MapEditor::Domain::ChunkedMap &map,
                  MapEditor::Brushes::BrushRegistry &registry,
                  MapEditor::Brushes::GroundBrush &brush,
                  const MapEditor::Domain::Position &pos) {
    MapEditor::Brushes::DrawContext ctx;
    ctx.brushRegistry = &registry;
    ctx.ownerBrushId = registry.getBrushId(&brush);
    brush.draw(map, map.getOrCreateTile(pos), ctx);
  };

  {
    const fs::path brushPath = tempDir / "ground_specific_replace.xml";
    writeTextFile(
        brushPath,
        R"xml(<brushes>
  <brush name="specific-replace-owner" type="ground" lookid="65000" z-order="10">
    <item id="65000" />
    <border align="inner" to="specific-replace-north" id="6501">
      <specific>
        <conditions>
          <match_border id="6501" edge="n" />
          <match_border id="6502" edge="e" />
        </conditions>
        <actions>
          <replace_border id="6502" edge="e" with="65099" />
        </actions>
      </specific>
    </border>
    <border align="inner" to="specific-replace-east" id="6502" />
  </brush>
  <brush name="specific-replace-north" type="ground" lookid="65001" z-order="1">
    <item id="65001" />
  </brush>
  <brush name="specific-replace-east" type="ground" lookid="65002" z-order="1">
    <item id="65002" />
  </brush>
</brushes>)xml");

    MapEditor::Brushes::BrushRegistry localRegistry;
    registerTemplate(localRegistry, 6501, 65010, EdgeType::N);
    registerTemplate(localRegistry, 6502, 65020, EdgeType::E);
    MapEditor::IO::BrushXmlReader reader({&localRegistry});
    require(reader.loadFile(brushPath),
            "specific replace ground brush XML failed");

    auto *ownerBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
        localRegistry.getBrush("specific-replace-owner"));
    auto *northBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
        localRegistry.getBrush("specific-replace-north"));
    auto *eastBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
        localRegistry.getBrush("specific-replace-east"));
    require(ownerBrush != nullptr && northBrush != nullptr && eastBrush != nullptr,
            "specific replace ground brushes did not load");

    MapEditor::Domain::ChunkedMap map;
    map.createNew(16, 16, 1098);
    const MapEditor::Domain::Position north{8, 7, 7};
    const MapEditor::Domain::Position center{8, 8, 7};
    const MapEditor::Domain::Position east{9, 8, 7};
    paint(map, localRegistry, *northBrush, north);
    paint(map, localRegistry, *eastBrush, east);
    paint(map, localRegistry, *ownerBrush, center);

    const auto *centerTile = map.getTile(center);
    require(centerTile != nullptr && tileContainsItemId(*centerTile, 65099),
            "specific replace_border did not place replacement item");
    require(!tileContainsItemId(*centerTile, 65020),
            "specific replace_border left the replaced border item");
    require(!tileContainsItemId(*centerTile, 65010),
            "specific replace_border did not delete the other matched border");
  }

  {
    const fs::path brushPath = tempDir / "ground_specific_delete.xml";
    writeTextFile(
        brushPath,
        R"xml(<brushes>
  <brush name="specific-delete-owner" type="ground" lookid="65100" z-order="10">
    <item id="65100" />
    <border align="inner" to="specific-delete-north" id="6511">
      <specific>
        <conditions>
          <match_border id="6511" edge="n" />
          <match_border id="6512" edge="e" />
        </conditions>
        <actions>
          <delete_borders />
        </actions>
      </specific>
    </border>
    <border align="inner" to="specific-delete-east" id="6512" />
  </brush>
  <brush name="specific-delete-north" type="ground" lookid="65101" z-order="1">
    <item id="65101" />
  </brush>
  <brush name="specific-delete-east" type="ground" lookid="65102" z-order="1">
    <item id="65102" />
  </brush>
</brushes>)xml");

    MapEditor::Brushes::BrushRegistry localRegistry;
    registerTemplate(localRegistry, 6511, 65110, EdgeType::N);
    registerTemplate(localRegistry, 6512, 65120, EdgeType::E);
    MapEditor::IO::BrushXmlReader reader({&localRegistry});
    require(reader.loadFile(brushPath),
            "specific delete ground brush XML failed");

    auto *ownerBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
        localRegistry.getBrush("specific-delete-owner"));
    auto *northBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
        localRegistry.getBrush("specific-delete-north"));
    auto *eastBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
        localRegistry.getBrush("specific-delete-east"));
    require(ownerBrush != nullptr && northBrush != nullptr && eastBrush != nullptr,
            "specific delete ground brushes did not load");

    MapEditor::Domain::ChunkedMap map;
    map.createNew(16, 16, 1098);
    const MapEditor::Domain::Position north{8, 7, 7};
    const MapEditor::Domain::Position center{8, 8, 7};
    const MapEditor::Domain::Position east{9, 8, 7};
    paint(map, localRegistry, *northBrush, north);
    paint(map, localRegistry, *eastBrush, east);
    paint(map, localRegistry, *ownerBrush, center);

    const auto *centerTile = map.getTile(center);
    require(centerTile != nullptr && !tileContainsItemId(*centerTile, 65110) &&
                !tileContainsItemId(*centerTile, 65120),
            "specific delete_borders did not remove matched borders");
  }

  {
    const fs::path brushPath = tempDir / "ground_specific_group.xml";
    writeTextFile(
        brushPath,
        R"xml(<brushes>
  <brush name="specific-group-owner" type="ground" lookid="65200" z-order="10">
    <item id="65200" />
    <border align="inner" to="specific-group-north" id="6521">
      <specific>
        <conditions>
          <match_group group="1" edge="n" />
          <match_border id="6522" edge="e" />
        </conditions>
        <actions>
          <replace_border id="6522" edge="e" with="65299" />
        </actions>
      </specific>
    </border>
    <border align="inner" to="specific-group-east" id="6522" />
  </brush>
  <brush name="specific-group-north" type="ground" lookid="65201" z-order="1">
    <item id="65201" />
  </brush>
  <brush name="specific-group-east" type="ground" lookid="65202" z-order="1">
    <item id="65202" />
  </brush>
</brushes>)xml");

    MapEditor::Brushes::BrushRegistry localRegistry;
    registerTemplate(localRegistry, 6521, 65210, EdgeType::N, 1);
    registerTemplate(localRegistry, 6522, 65220, EdgeType::E);
    MapEditor::IO::BrushXmlReader reader({&localRegistry});
    require(reader.loadFile(brushPath),
            "specific group ground brush XML failed");

    auto *ownerBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
        localRegistry.getBrush("specific-group-owner"));
    auto *northBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
        localRegistry.getBrush("specific-group-north"));
    auto *eastBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
        localRegistry.getBrush("specific-group-east"));
    require(ownerBrush != nullptr && northBrush != nullptr && eastBrush != nullptr,
            "specific group ground brushes did not load");

    MapEditor::Domain::ChunkedMap map;
    map.createNew(16, 16, 1098);
    const MapEditor::Domain::Position north{8, 7, 7};
    const MapEditor::Domain::Position center{8, 8, 7};
    const MapEditor::Domain::Position east{9, 8, 7};
    paint(map, localRegistry, *northBrush, north);
    paint(map, localRegistry, *eastBrush, east);
    paint(map, localRegistry, *ownerBrush, center);

    const auto *centerTile = map.getTile(center);
    require(centerTile != nullptr && tileContainsItemId(*centerTile, 65299),
            "specific match_group did not enable replacement action");
    require(tileContainsItemId(*centerTile, 65210),
            "specific match_group should keep the group-matched border");
    require(!tileContainsItemId(*centerTile, 65220),
            "specific match_group replacement left the replaced border item");
  }

  {
    const fs::path brushPath = tempDir / "ground_specific_replace_item.xml";
    writeTextFile(
        brushPath,
        R"xml(<brushes>
  <brush name="specific-replace-item-owner" type="ground" lookid="65300" z-order="10">
    <item id="65300" />
    <border align="inner" to="specific-replace-item-east" id="6531">
      <specific>
        <conditions>
          <match_border id="6531" edge="e" />
        </conditions>
        <actions>
          <replace_item id="65310" with="65399" />
        </actions>
      </specific>
    </border>
  </brush>
  <brush name="specific-replace-item-east" type="ground" lookid="65301" z-order="1">
    <item id="65301" />
  </brush>
</brushes>)xml");

    MapEditor::Brushes::BrushRegistry localRegistry;
    registerTemplate(localRegistry, 6531, 65310, EdgeType::E);
    MapEditor::IO::BrushXmlReader reader({&localRegistry});
    require(reader.loadFile(brushPath),
            "specific replace_item ground brush XML failed");

    auto *ownerBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
        localRegistry.getBrush("specific-replace-item-owner"));
    auto *eastBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
        localRegistry.getBrush("specific-replace-item-east"));
    require(ownerBrush != nullptr && eastBrush != nullptr,
            "specific replace_item ground brushes did not load");

    MapEditor::Domain::ChunkedMap map;
    map.createNew(16, 16, 1098);
    const MapEditor::Domain::Position center{8, 8, 7};
    const MapEditor::Domain::Position east{9, 8, 7};
    paint(map, localRegistry, *eastBrush, east);
    paint(map, localRegistry, *ownerBrush, center);

    const auto *centerTile = map.getTile(center);
    require(centerTile != nullptr && tileContainsItemId(*centerTile, 65399),
            "specific replace_item did not place replacement item");
    require(!tileContainsItemId(*centerTile, 65310),
            "specific replace_item left the replaced item");
  }

  {
    const fs::path brushPath = tempDir / "ground_specific_keep_border.xml";
    writeTextFile(
        brushPath,
        R"xml(<brushes>
  <brush name="specific-keep-owner" type="ground" lookid="65400" z-order="10">
    <item id="65400" />
    <border align="inner" to="specific-keep-north" id="6541">
      <specific keep_border="true">
        <conditions>
          <match_border id="6541" edge="n" />
          <match_border id="6542" edge="e" />
        </conditions>
        <actions>
          <replace_border id="6542" edge="e" with="65499" />
        </actions>
      </specific>
    </border>
    <border align="inner" to="specific-keep-east" id="6542" />
  </brush>
  <brush name="specific-keep-north" type="ground" lookid="65401" z-order="1">
    <item id="65401" />
  </brush>
  <brush name="specific-keep-east" type="ground" lookid="65402" z-order="1">
    <item id="65402" />
  </brush>
</brushes>)xml");

    MapEditor::Brushes::BrushRegistry localRegistry;
    registerTemplate(localRegistry, 6541, 65410, EdgeType::N);
    registerTemplate(localRegistry, 6542, 65420, EdgeType::E);
    MapEditor::IO::BrushXmlReader reader({&localRegistry});
    require(reader.loadFile(brushPath),
            "specific keep_border ground brush XML failed");

    auto *ownerBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
        localRegistry.getBrush("specific-keep-owner"));
    auto *northBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
        localRegistry.getBrush("specific-keep-north"));
    auto *eastBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
        localRegistry.getBrush("specific-keep-east"));
    require(ownerBrush != nullptr && northBrush != nullptr && eastBrush != nullptr,
            "specific keep_border ground brushes did not load");

    MapEditor::Domain::ChunkedMap map;
    map.createNew(16, 16, 1098);
    const MapEditor::Domain::Position north{8, 7, 7};
    const MapEditor::Domain::Position center{8, 8, 7};
    const MapEditor::Domain::Position east{9, 8, 7};
    paint(map, localRegistry, *northBrush, north);
    paint(map, localRegistry, *eastBrush, east);
    paint(map, localRegistry, *ownerBrush, center);

    const auto *centerTile = map.getTile(center);
    require(centerTile != nullptr && tileContainsItemId(*centerTile, 65499),
            "specific keep_border did not place replacement item");
    require(tileContainsItemId(*centerTile, 65410),
            "specific keep_border did not keep the other matched border");
    require(!tileContainsItemId(*centerTile, 65420),
            "specific keep_border left the replaced border item");
  }
}

void requireRealDataGroundSpecificCaseStack(
    MapEditor::Brushes::BrushRegistry &registry) {
  auto *grassBrush = findGroundBrush(registry, "grass");
  auto *snowBrush = findGroundBrush(registry, "snow");
  auto *seaBrush = findGroundBrush(registry, "sea");
  require(grassBrush != nullptr && snowBrush != nullptr && seaBrush != nullptr,
          "real-data ground specific-case fixture brushes are missing");

  MapEditor::Domain::ChunkedMap map;
  map.createNew(16, 16, 1098);
  const MapEditor::Domain::Position north{8, 7, 7};
  const MapEditor::Domain::Position center{8, 8, 7};
  const MapEditor::Domain::Position east{9, 8, 7};

  paintGround(map, registry, *snowBrush, north);
  paintGround(map, registry, *seaBrush, east);
  paintGround(map, registry, *grassBrush, center);

  const auto *centerTile = map.getTile(center);
  require(centerTile != nullptr && tileContainsItemId(*centerTile, 6656),
          "real-data snow/sea specific case did not place replacement item");
  require(!tileContainsItemId(*centerTile, 4645),
          "real-data snow/sea specific case left the replaced sea border");
  require(!tileContainsItemId(*centerTile, 4737),
          "real-data snow/sea specific case left the matched snow border");
}

void requireRealDataSandSpecificCaseReplaceItem(
    MapEditor::Brushes::BrushRegistry &registry) {
  auto *sandBrush = findGroundBrush(registry, "sand");
  auto *seaBrush = findGroundBrush(registry, "sea");
  auto *grassBrush = findGroundBrush(registry, "grass");
  require(sandBrush != nullptr && seaBrush != nullptr && grassBrush != nullptr,
          "real-data sand specific-case fixture brushes are missing");

  MapEditor::Domain::ChunkedMap map;
  map.createNew(16, 16, 1098);
  const MapEditor::Domain::Position center{8, 8, 7};
  const MapEditor::Domain::Position east{9, 8, 7};
  const MapEditor::Domain::Position south{8, 9, 7};

  paintGround(map, registry, *grassBrush, east);
  paintGround(map, registry, *seaBrush, south);
  paintGround(map, registry, *sandBrush, center);

  const auto *centerTile = map.getTile(center);
  require(centerTile != nullptr && tileContainsItemId(*centerTile, 4661),
          "real-data sand specific case did not place replacement item");
  require(!tileContainsItemId(*centerTile, 4634),
          "real-data sand specific case left the replaced shoreline item");
  require(!tileContainsItemId(*centerTile, 4543),
          "real-data sand specific case did not delete the matched grass border " 
          "(match_border adds all template items to match set; without keep_border "
          "they are deleted per RME semantics)");
}

void requireRealDataFrozenMudSpecificCaseMatchGroup(
    MapEditor::Brushes::BrushRegistry &registry) {
  auto *frozenMudBrush = findGroundBrush(registry, "frozen mud");
  auto *seaBrush = findGroundBrush(registry, "sea");
  auto *grassBrush = findGroundBrush(registry, "grass");
  require(frozenMudBrush != nullptr && seaBrush != nullptr &&
              grassBrush != nullptr,
          "real-data frozen mud specific-case fixture brushes are missing");

  MapEditor::Domain::ChunkedMap map;
  map.createNew(16, 16, 1098);
  const MapEditor::Domain::Position center{8, 8, 7};
  const MapEditor::Domain::Position east{9, 8, 7};
  const MapEditor::Domain::Position south{8, 9, 7};

  paintGround(map, registry, *seaBrush, east);
  paintGround(map, registry, *grassBrush, south);
  paintGround(map, registry, *frozenMudBrush, center);

  const auto *centerTile = map.getTile(center);
  require(centerTile != nullptr && tileContainsItemId(*centerTile, 6664),
          "real-data frozen mud match_group case did not replace sea border");
  require(tileContainsItemId(*centerTile, 4544),
          "real-data frozen mud match_group case lost the matched group border");
  require(!tileContainsItemId(*centerTile, 6640),
          "real-data frozen mud match_group case left the replaced sea border");
}

void requireRealDataGroundOuterZilch(
    MapEditor::Brushes::BrushRegistry &registry) {
  auto *alternateGrassBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      registry.getBrush("grass (alternate border)"));
  require(alternateGrassBrush != nullptr,
          "real-data outer-zilch fixture brush is missing");

  MapEditor::Domain::ChunkedMap map;
  map.createNew(16, 16, 1098);
  const MapEditor::Domain::Position north{8, 7, 7};
  const MapEditor::Domain::Position center{8, 8, 7};

  MapEditor::Brushes::DrawContext ctx;
  ctx.brushRegistry = &registry;
  ctx.ownerBrushId = registry.getBrushId(alternateGrassBrush);
  alternateGrassBrush->draw(map, map.getOrCreateTile(north), ctx);

  const auto *centerTile = map.getTile(center);
  require(centerTile != nullptr && !centerTile->hasGround(),
          "real-data outer-zilch did not materialize a groundless tile");
  require(tileContainsItemId(*centerTile, 7653),
          "real-data outer-zilch did not place border id 120 north item");
}

void requireGroundMultiTileBatchedPlacement(const fs::path &tempDir) {
  const fs::path brushPath = tempDir / "ground_batch_multitile.xml";
  writeTextFile(
      brushPath,
      R"xml(<brushes>
  <brush name="batch-grass" type="ground" lookid="66050" z-order="10">
    <item id="66050" />
    <border align="inner" to="batch-dirt">
      <borderitem edge="n" id="66051" />
      <borderitem edge="e" id="66052" />
      <borderitem edge="s" id="66053" />
      <borderitem edge="w" id="66054" />
    </border>
    <border align="inner" to="none" id="1" />
  </brush>
  <brush name="batch-dirt" type="ground" lookid="66060" z-order="5">
    <item id="66060" />
  </brush>
</brushes>)xml");

  MapEditor::Brushes::BrushRegistry localRegistry;
  MapEditor::Brushes::BorderBlock zilchBlock;
  zilchBlock.addItem(EdgeType::N, 66071, 1);
  zilchBlock.addItem(EdgeType::E, 66072, 1);
  zilchBlock.addItem(EdgeType::S, 66073, 1);
  zilchBlock.addItem(EdgeType::W, 66074, 1);
  localRegistry.registerBorderTemplate(1, std::move(zilchBlock));

  MapEditor::IO::BrushXmlReader reader({&localRegistry});
  require(reader.loadFile(brushPath), "batch multitile ground brush XML failed");

  auto *grassBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("batch-grass"));
  auto *dirtBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("batch-dirt"));
  require(grassBrush != nullptr && dirtBrush != nullptr,
          "batch multitile ground brushes did not load");

  auto paint = [&](MapEditor::Domain::ChunkedMap &map,
                   MapEditor::Brushes::GroundBrush &brush,
                   const MapEditor::Domain::Position &pos) {
    MapEditor::Brushes::DrawContext ctx;
    ctx.brushRegistry = &localRegistry;
    ctx.ownerBrushId = localRegistry.getBrushId(&brush);
    brush.draw(map, map.getOrCreateTile(pos), ctx);
  };

  MapEditor::Domain::ChunkedMap map;
  map.createNew(16, 16, 1098);
  const MapEditor::Domain::Position grass1{8, 8, 7};
  const MapEditor::Domain::Position grass2{9, 8, 7};
  const MapEditor::Domain::Position grass3{8, 9, 7};
  const MapEditor::Domain::Position dirt1{10, 8, 7};

  paint(map, *grassBrush, grass1);
  paint(map, *grassBrush, grass2);
  paint(map, *grassBrush, grass3);
  paint(map, *dirtBrush, dirt1);

  const auto *grassEastTile = map.getTile(grass2);
  require(grassEastTile != nullptr,
          "batch multitile grass tile missing at 9,8");

  require(tileContainsItemId(*grassEastTile, 66052),
          "batch multitile: grass at 9,8 should have inner border to dirt at east");

  const auto *grassCornerTile = map.getTile(grass1);
  require(grassCornerTile != nullptr,
          "batch multitile grass corner tile missing at 8,8");

  const bool hasZilchBorder =
      tileContainsItemId(*grassCornerTile, 66071) ||
      tileContainsItemId(*grassCornerTile, 66074);
  require(hasZilchBorder,
          "batch multitile: grass corner should have zilch border against empty");
}

void requireGroundSequentialVsBatchedParity(const fs::path &tempDir) {
  const fs::path brushPath = tempDir / "ground_parity.xml";
  writeTextFile(
      brushPath,
      R"xml(<brushes>
  <brush name="parity-grass" type="ground" lookid="65150" z-order="10">
    <item id="65150" />
    <border align="inner">
      <borderitem edge="n" id="65151" />
      <borderitem edge="e" id="65152" />
      <borderitem edge="s" id="65153" />
      <borderitem edge="w" id="65154" />
    </border>
  </brush>
  <brush name="parity-dirt" type="ground" lookid="65160" z-order="5">
    <item id="65160" />
    <border align="inner" to="parity-grass">
      <borderitem edge="n" id="65161" />
    </border>
  </brush>
</brushes>)xml");

  MapEditor::Brushes::BrushRegistry localRegistry;
  MapEditor::IO::BrushXmlReader reader({&localRegistry});
  require(reader.loadFile(brushPath), "parity ground brush XML failed");

  auto *grassBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("parity-grass"));
  auto *dirtBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("parity-dirt"));
  require(grassBrush != nullptr && dirtBrush != nullptr,
          "parity ground brushes did not load");

  auto paintOne = [&](MapEditor::Domain::ChunkedMap &map,
                      MapEditor::Brushes::GroundBrush &brush,
                      const MapEditor::Domain::Position &pos) {
    MapEditor::Brushes::DrawContext ctx;
    ctx.brushRegistry = &localRegistry;
    ctx.ownerBrushId = localRegistry.getBrushId(&brush);
    brush.draw(map, map.getOrCreateTile(pos), ctx);
  };

  const std::array sequentialPositions{
      MapEditor::Domain::Position{8, 7, 7},
      MapEditor::Domain::Position{9, 7, 7},
      MapEditor::Domain::Position{8, 8, 7},
      MapEditor::Domain::Position{9, 8, 7},
  };

  MapEditor::Domain::ChunkedMap sequentialMap;
  sequentialMap.createNew(16, 16, 1098);
  for (const auto &pos : sequentialPositions) {
    paintOne(sequentialMap, *dirtBrush, pos);
  }

  MapEditor::Domain::ChunkedMap batchedMap;
  batchedMap.createNew(16, 16, 1098);

  auto paintBatched = [&](MapEditor::Domain::ChunkedMap &map,
                          MapEditor::Brushes::GroundBrush &brush,
                          std::span<const MapEditor::Domain::Position> positions) {
    MapEditor::Services::Autoborder::AutoborderEngine engine;
    MapEditor::Services::Autoborder::PlacementIntent intent;
    intent.brush = &brush;
    intent.mode = MapEditor::Services::Autoborder::PlacementMode::Draw;
    intent.context.brushRegistry = &localRegistry;
    intent.context.ownerBrushId = localRegistry.getBrushId(&brush);
    intent.context.variation = 0;
    intent.context.modifiers = 0;
    intent.positions.assign(positions.begin(), positions.end());
    auto diffs = engine.plan(map, intent);
    require(!diffs.empty(), "batched parity plan produced no diffs");
    MapEditor::Services::Autoborder::applyTileDiffs(map, std::move(diffs));
  };

  paintBatched(batchedMap, *dirtBrush, sequentialPositions);

  for (int y = 5; y <= 10; ++y) {
    for (int x = 5; x <= 11; ++x) {
      const MapEditor::Domain::Position pos{x, y, 7};
      const auto *seqTile = sequentialMap.getTile(pos);
      const auto *batTile = batchedMap.getTile(pos);
      const bool seqHas = seqTile && seqTile->hasGround();
      const bool batHas = batTile && batTile->hasGround();
      require(seqHas == batHas,
              "sequential vs batched ground presence mismatch at " +
                  std::to_string(x) + "," + std::to_string(y));
      if (seqHas) {
        require(seqTile->getGround()->getServerId() ==
                    batTile->getGround()->getServerId(),
                "sequential vs batched ground item mismatch at " +
                    std::to_string(x) + "," + std::to_string(y));
      }
    }
  }
}

void requireRawBrushPlacementAndParity(MapEditor::Brushes::BrushRegistry &registry) {
  // 1. Prepare settings and controller
  MapEditor::Services::BrushSettingsService brushSettings;
  MapEditor::Brushes::BrushController controller;
  MapEditor::Domain::History::HistoryManager history;

  MapEditor::Domain::ChunkedMap map;
  map.createNew(16, 16, 1098);
  controller.initialize(&map, &history, nullptr);
  controller.setBrushRegistry(&registry);
  controller.setBrushSettingsService(&brushSettings);

  const MapEditor::Domain::Position pos{8, 8, 7};
  auto *tile = map.getOrCreateTile(pos);

  // 2. Define mock ItemTypes
  MapEditor::Domain::ItemType groundType;
  groundType.server_id = 9001;
  groundType.client_id = 9001;
  groundType.group = MapEditor::Domain::ItemGroup::Ground;

  MapEditor::Domain::ItemType normalType;
  normalType.server_id = 9002;
  normalType.client_id = 9002;
  normalType.group = MapEditor::Domain::ItemGroup::None;

  MapEditor::Domain::ItemType bottom1Type;
  bottom1Type.server_id = 9003;
  bottom1Type.client_id = 9003;
  bottom1Type.flags = MapEditor::Domain::ItemFlag::AlwaysOnBottom;
  bottom1Type.always_on_top_order = 1;

  MapEditor::Domain::ItemType bottom3Type;
  bottom3Type.server_id = 9004;
  bottom3Type.client_id = 9004;
  bottom3Type.flags = MapEditor::Domain::ItemFlag::AlwaysOnBottom;
  bottom3Type.always_on_top_order = 3;

  MapEditor::Domain::ItemType bottom2Type;
  bottom2Type.server_id = 9005;
  bottom2Type.client_id = 9005;
  bottom2Type.flags = MapEditor::Domain::ItemFlag::AlwaysOnBottom;
  bottom2Type.always_on_top_order = 2;

  MapEditor::Domain::ItemType hookType;
  hookType.server_id = 9006;
  hookType.client_id = 9006;
  hookType.flags = MapEditor::Domain::ItemFlag::HookSouth | MapEditor::Domain::ItemFlag::HookEast;

  // Let's register raw brushes with the registry
  auto* groundRaw = dynamic_cast<MapEditor::Brushes::RawBrush*>(registry.getOrCreateRAWBrush(9001));
  auto* normalRaw = dynamic_cast<MapEditor::Brushes::RawBrush*>(registry.getOrCreateRAWBrush(9002));
  auto* bottom1Raw = dynamic_cast<MapEditor::Brushes::RawBrush*>(registry.getOrCreateRAWBrush(9003));
  auto* bottom3Raw = dynamic_cast<MapEditor::Brushes::RawBrush*>(registry.getOrCreateRAWBrush(9004));
  auto* bottom2Raw = dynamic_cast<MapEditor::Brushes::RawBrush*>(registry.getOrCreateRAWBrush(9005));
  auto* hookRaw = dynamic_cast<MapEditor::Brushes::RawBrush*>(registry.getOrCreateRAWBrush(9006));

  require(groundRaw && normalRaw && bottom1Raw && bottom3Raw && bottom2Raw && hookRaw,
          "failed to create RAW brushes for testing");

  groundRaw->setCachedType(&groundType);
  normalRaw->setCachedType(&normalType);
  bottom1Raw->setCachedType(&bottom1Type);
  bottom3Raw->setCachedType(&bottom3Type);
  bottom2Raw->setCachedType(&bottom2Type);
  hookRaw->setCachedType(&hookType);

  // Test 1: Ground-like RAW placement overwrites ground slot
  controller.setBrush(groundRaw);
  require(controller.applyBrush(pos), "ground raw apply failed");
  require(tile->getGround() != nullptr, "ground slot should be populated");
  require(tile->getGround()->getServerId() == 9001, "ground server ID mismatch");

  // Test 2: Normal item RAW placement appends to tile items
  controller.setBrush(normalRaw);
  require(controller.applyBrush(pos), "normal raw apply failed");
  require(tile->getItemCount() == 1, "items list should have 1 item");
  require(tile->getItem(0)->getServerId() == 9002, "item 0 server ID mismatch");

  // Test 3: Always-on-bottom items sorted by top_order
  // Draw in order: bottom3 (order 3), then bottom1 (order 1), then bottom2 (order 2)
  controller.setBrush(bottom3Raw);
  require(controller.applyBrush(pos), "bottom3 raw apply failed");
  controller.setBrush(bottom1Raw);
  require(controller.applyBrush(pos), "bottom1 raw apply failed");
  controller.setBrush(bottom2Raw);
  require(controller.applyBrush(pos), "bottom2 raw apply failed");

  // Items should be: bottom1 (9003, order 1), bottom2 (9005, order 2), bottom3 (9004, order 3), normal (9002)
  require(tile->getItemCount() == 4, "tile items count mismatch after bottom items draw");
  require(tile->getItem(0)->getServerId() == 9003, "stack order mismatch: item 0 should be order 1");
  require(tile->getItem(1)->getServerId() == 9005, "stack order mismatch: item 1 should be order 2");
  require(tile->getItem(2)->getServerId() == 9004, "stack order mismatch: item 2 should be order 3");
  require(tile->getItem(3)->getServerId() == 9002, "stack order mismatch: item 3 should be normal item");

  // Test 4: Hook South/East item placement registers hooks on tile
  controller.setBrush(hookRaw);
  require(controller.applyBrush(pos), "hook raw apply failed");
  require(tile->hasHookSouth(), "tile should register hook south");
  require(tile->hasHookEast(), "tile should register hook east");

  // Test 5: SimOne Replace vs Stacking (top_order == 2)
  MapEditor::Domain::ItemType bottom2AltType;
  bottom2AltType.server_id = 9007;
  bottom2AltType.client_id = 9007;
  bottom2AltType.flags = MapEditor::Domain::ItemFlag::AlwaysOnBottom;
  bottom2AltType.always_on_top_order = 2;

  auto* bottom2AltRaw = dynamic_cast<MapEditor::Brushes::RawBrush*>(registry.getOrCreateRAWBrush(9007));
  bottom2AltRaw->setCachedType(&bottom2AltType);

  // rawLikeSimone is true by default, Alt modifier is false. Placing bottom2AltRaw should replace bottom2 (9005).
  brushSettings.setRawLikeSimone(true);
  controller.setBrush(bottom2AltRaw);
  require(controller.applyBrush(pos), "bottom2alt raw apply failed");

  // Verify that item 9005 (order 2) was replaced by 9007, but order 1 and 3 items and normal items remain.
  require(!tileContainsItemId(*tile, 9005), "order 2 item 9005 should have been replaced");
  require(tileContainsItemId(*tile, 9007), "order 2 item 9007 should be placed");
  require(tileContainsItemId(*tile, 9003), "order 1 item should remain");
  require(tileContainsItemId(*tile, 9004), "order 3 item should remain");

  // Now, test with Alt modifier pressed (should stack instead of replace)
  {
    MapEditor::Brushes::DrawContext altCtx;
    altCtx.brushRegistry = &registry;
    altCtx.brushSettings = &brushSettings;
    altCtx.modifiers = MapEditor::Brushes::Modifiers::Alt;
    altCtx.ownerBrushId = registry.getBrushId(bottom2Raw);
    bottom2Raw->draw(map, tile, altCtx);
  }
  // Now both 9007 and 9005 (both order 2) should be on the tile.
  require(tileContainsItemId(*tile, 9007), "order 2 item 9007 should still be there");
  require(tileContainsItemId(*tile, 9005), "order 2 item 9005 should be stacked");

  // Test 6: RAW Erasing
  controller.setBrush(groundRaw);
  groundRaw->undraw(map, tile);
  require(tile->getGround() == nullptr, "ground should be cleared after undraw");

  normalRaw->undraw(map, tile);
  require(!tileContainsItemId(*tile, 9002), "normal item should be cleared after undraw");

  // Test 7: Smart Pick Fallback
  tile->removeGround();
  tile->removeItemsIf([](const MapEditor::Domain::Item*) { return true; });

  controller.setBrush(groundRaw);
  controller.applyBrush(pos);
  controller.setBrush(normalRaw);
  controller.applyBrush(pos);

  auto selection = controller.resolveBrushFromTile(*tile, PickMode::Smart);
  require(selection.has_value(), "smart pick should succeed");
  require(selection->mode == MapEditor::Brushes::BrushPickMode::Raw, "resolved pick mode should be Raw");
  require(selection->rawItemId == 9002, "resolved raw item ID should be top item (9002)");

  normalRaw->undraw(map, tile);
  selection = controller.resolveBrushFromTile(*tile, PickMode::Smart);
  require(selection.has_value(), "smart pick should succeed");
  require(selection->mode == MapEditor::Brushes::BrushPickMode::Raw, "resolved pick mode should be Raw");
  require(selection->rawItemId == 9001, "resolved raw item ID should fall back to ground (9001)");
}

void requireDoodadCtrlEraseOnlyRemovesSelectedDoodad() {
  MapEditor::Brushes::BrushRegistry registry;
  MapEditor::Services::BrushSettingsService brushSettings;

  auto grassPtr = std::make_unique<MapEditor::Brushes::DoodadBrush>(
      "grass turfs", 400, registry, false);
  MapEditor::Brushes::DoodadAlternative grassAlternative;
  grassAlternative.addSingleItem({.itemId = 400, .chance = 1});
  grassPtr->addAlternative(std::move(grassAlternative));
  auto *grassBrush = grassPtr.get();
  registry.addBrush(std::move(grassPtr));

  auto firePtr = std::make_unique<MapEditor::Brushes::DoodadBrush>(
      "fire", 500, registry, false);
  MapEditor::Brushes::DoodadAlternative fireAlternative;
  fireAlternative.addSingleItem({.itemId = 500, .chance = 1});
  firePtr->addAlternative(std::move(fireAlternative));
  auto *fireBrush = firePtr.get();
  registry.addBrush(std::move(firePtr));

  MapEditor::Domain::ChunkedMap map;
  map.createNew(16, 16, 1098);
  MapEditor::Domain::History::HistoryManager history;
  MapEditor::Brushes::BrushController controller;
  controller.initialize(&map, &history, nullptr);
  controller.setBrushRegistry(&registry);
  controller.setBrushSettingsService(&brushSettings);

  const MapEditor::Domain::Position pos{8, 8, 7};
  controller.setBrush(fireBrush);
  require(controller.applyBrush(pos), "fire doodad paint failed");
  controller.setBrush(grassBrush);
  require(controller.applyBrush(pos), "grass doodad paint failed");

  auto *tile = map.getTile(pos);
  require(tile != nullptr, "doodad erase fixture tile missing");
  require(tileContainsItemId(*tile, 400), "grass doodad was not placed");
  require(tileContainsItemId(*tile, 500), "fire doodad was not placed");

  require(controller.eraseBrush(pos, GLFW_MOD_CONTROL),
          "grass doodad ctrl erase did not mutate");
  require(!tileContainsItemId(*tile, 400),
          "ctrl erase left the selected grass doodad");
  require(tileContainsItemId(*tile, 500),
          "ctrl erase removed a different fire doodad");

  tile->removeItemsIf([](const MapEditor::Domain::Item *) { return true; });
  auto legacyGrassItem = std::make_unique<MapEditor::Domain::Item>(400);
  tile->addItemDirect(std::move(legacyGrassItem));

  auto fireOwnedSameId = std::make_unique<MapEditor::Domain::Item>(400);
  fireOwnedSameId->setOwnerBrushId(registry.getBrushId(fireBrush));
  tile->addItemDirect(std::move(fireOwnedSameId));

  require(countItemId(*tile, 400) == 2,
          "legacy doodad erase fixture did not stack two same-id items");
  controller.setBrush(grassBrush);
  require(controller.eraseBrush(pos, GLFW_MOD_CONTROL),
          "legacy grass doodad ctrl erase did not mutate");
  require(countItemId(*tile, 400) == 1,
          "ctrl erase did not remove exactly one legacy matching item");
  require(tile->getItemCount() == 1 &&
              tile->getItem(0)->getOwnerBrushId() == registry.getBrushId(fireBrush),
          "ctrl erase removed a same-id item owned by a different doodad");
}

void requireSandDuneGroundEquivalentDoesNotOuterBorder(const fs::path &tempDir) {
  const fs::path brushPath = tempDir / "sand_dune_ground_equivalent.xml";
  writeTextFile(
      brushPath,
      R"xml(<brushes>
  <brush name="test-grass" type="ground" lookid="100" z-order="10">
    <item id="100" />
    <border align="inner" to="none">
      <borderitem edge="n" id="895" />
      <borderitem edge="e" id="895" />
      <borderitem edge="s" id="895" />
      <borderitem edge="w" id="895" />
    </border>
  </brush>
  <brush name="test-sand" type="ground" lookid="231" z-order="20">
    <item id="231" />
    <item id="8317" chance="0" />
    <border align="outer" to="none">
      <borderitem edge="n" id="894" />
      <borderitem edge="e" id="894" />
      <borderitem edge="s" id="894" />
      <borderitem edge="w" id="894" />
      <specific>
        <conditions>
          <match_item id="894" />
        </conditions>
        <actions>
          <replace_item id="894" with="8317" />
        </actions>
      </specific>
    </border>
  </brush>
</brushes>)xml");

  MapEditor::Brushes::BrushRegistry localRegistry;
  MapEditor::IO::BrushXmlReader reader({&localRegistry});
  require(reader.loadFile(brushPath), "sand dune fixture brush XML failed");

  auto *sandBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("test-sand"));
  auto *grassBrush = dynamic_cast<MapEditor::Brushes::GroundBrush *>(
      localRegistry.getBrush("test-grass"));
  require(sandBrush != nullptr && grassBrush != nullptr,
          "sand dune fixture brushes did not load");

  MapEditor::Domain::ChunkedMap map;
  map.createNew(16, 16, 1098);
  const MapEditor::Domain::Position center{8, 8, 7};
  for (int dy = -2; dy <= 2; ++dy) {
    for (int dx = -2; dx <= 2; ++dx) {
      if (dx == 0 && dy == 0) {
        continue;
      }
      paintGround(map, localRegistry, *grassBrush,
                  {center.x + dx, center.y + dy, center.z});
    }
  }

  auto *duneTile = map.getOrCreateTile(center);
  duneTile->setGround(std::make_unique<MapEditor::Domain::Item>(8317));
  duneTile->setGroundBrushId(MapEditor::Brushes::InvalidBrushId);

  require(MapEditor::Brushes::GroundBrush::resolveGroundBrush(localRegistry,
                                                              *duneTile) ==
              nullptr,
          "sand dune ground-equivalent id resolved as a real sand ground brush");

  sandBrush->rebuildAround(map, center);
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      const MapEditor::Domain::Position pos{center.x + dx, center.y + dy,
                                            center.z};
      const auto *tile = map.getTile(pos);
      require(!tile || !tileContainsItemId(*tile, 894),
              "sand dune ground-equivalent id created outer border rock 894");
      require(!tile || !tileContainsItemId(*tile, 895),
              "sand dune ground-equivalent id created inner grass border");
    }
  }
}

} // namespace

int main() {
  try {
    const fs::path sourceRoot = BRUSH_SMOKE_SOURCE_DIR;
    const fs::path dataPath = sourceRoot.parent_path() / "data" / "1098";
    const fs::path tempDir =
        fs::temp_directory_path() / "imgui-mapeditor-brush-smoke";
    std::error_code cleanupError;
    fs::remove_all(tempDir, cleanupError);
    fs::create_directories(tempDir);

    require(fs::exists(dataPath / "materials.xml"),
            "data/1098/materials.xml is missing");

    MapEditor::Brushes::BrushRegistry registry;
    const auto hotkeys = MapEditor::Services::HotkeyRegistry::createDefaults();
    require(hotkeys.findByAction("BRUSH_REFRESH_CURRENT") != nullptr,
            "brush refresh hotkey binding is missing");
    require(hotkeys.findByAction("BRUSH_TOGGLE_SELECTION_TOOL") != nullptr,
            "brush selection-toggle hotkey binding is missing");
    require(hotkeys.findByAction("BRUSH_RESTORE_LAST") != nullptr,
            "brush previous-brush hotkey binding is missing");
    require(hotkeys.findByAction("BRUSH_VARIATION_PREV") != nullptr,
            "brush variation-prev hotkey binding is missing");
    require(hotkeys.findByAction("BRUSH_VARIATION_NEXT") != nullptr,
            "brush variation-next hotkey binding is missing");
    require(hotkeys.findByAction("BRUSH_SLOT_0") != nullptr,
            "brush slot recall hotkey binding is missing");
    require(hotkeys.findByAction("BRUSH_STORE_SLOT_0") != nullptr,
            "brush slot store hotkey binding is missing");
    require(hotkeys.findByAction("ROTATE_ITEM") != nullptr,
            "rotate-item hotkey binding is missing");
    require(MapEditor::Brushes::parseDoorType("hatch window") ==
                DoorType::HatchWindow,
            "RME hatch window door spelling did not parse");
    requireBorderSelectionUsesScopedSeed();
    requireGroundBorderMissingAlignDefaultsOuter(tempDir);
    requireGroundBorderZOrderMatchesRme(tempDir);
    requireGroundFriendEnemySemanticsMatchRme(tempDir);
    requireGroundInlineOptionalBorderXml(tempDir);
    requireGroundOuterZilchMaterializesEmptyTile(tempDir);
    requireGroundClearBordersMatchesRme(tempDir);
    requireGroundTerrainPlacementUsesGroundEquivalent(tempDir);
    requireGroundSpecificCaseBorderActions(tempDir);
    requireGroundMultiTileBatchedPlacement(tempDir);
    requireGroundSequentialVsBatchedParity(tempDir);
    const auto hotkeyPath = tempDir / "hotkeys.json";
    writeTextFile(hotkeyPath,
                  R"json({"bindings":{"edit":{"SAVE":{"key":"S","mods":["Ctrl"]}}}})json");
    const auto loadedHotkeys = MapEditor::Services::HotkeyRegistry::loadOrCreateDefaults(
        {hotkeyPath.string()});
    require(loadedHotkeys.findByAction("SAVE") != nullptr,
            "custom hotkey binding did not load");
    require(loadedHotkeys.findByAction("BRUSH_TOGGLE_SELECTION_TOOL") != nullptr,
            "custom hotkey load did not merge brush defaults");
    MapEditor::Services::TilesetService tilesetService(registry);
    require(tilesetService.loadMaterials(dataPath),
            "materials.xml failed to load");
    require(!tilesetService.getTilesetRegistry().empty(),
            "no tilesets were registered");
    require(!tilesetService.getPaletteRegistry().empty(),
            "no palettes were registered");
    requireRealDataGroundSpecificCaseStack(registry);
    requireRealDataSandSpecificCaseReplaceItem(registry);
    requireRealDataFrozenMudSpecificCaseMatchGroup(registry);
    requireRealDataGroundOuterZilch(registry);
    requireRawBrushPlacementAndParity(registry);
    requireDoodadCtrlEraseOnlyRemovesSelectedDoodad();
    requireSandDuneGroundEquivalentDoesNotOuterBorder(tempDir);

    auto *creatureOthersTileset =
        tilesetService.getTilesetRegistry().getTilesetBySourceFile(
            dataPath / "tilesets/creatures/Others.xml");
    auto *rawOthersTileset =
        tilesetService.getTilesetRegistry().getTilesetBySourceFile(
            dataPath / "tilesets/raw/Others.xml");
    require(creatureOthersTileset != nullptr,
            "creature Others tileset was not registered by source file");
    require(rawOthersTileset != nullptr,
            "raw Others tileset was not registered by source file");
    require(creatureOthersTileset != rawOthersTileset,
            "same-named creature/raw tilesets were merged together");
    require(creatureOthersTileset->size() < rawOthersTileset->size(),
            "creature Others tileset unexpectedly contains raw-scale entries");

    auto *creaturePalette =
        tilesetService.getPaletteRegistry().getPalette("Creature");
    require(creaturePalette != nullptr, "Creature palette lookup failed");
    require(creaturePalette->getTilesetCount() == 1,
            "Creature palette should resolve to one tileset");
    require(creaturePalette->getTilesetAt(0) == creatureOthersTileset,
            "Creature palette did not bind to the creature Others tileset");
    require(creaturePalette->getTilesetAt(0)->size() < 1000,
            "Creature palette tileset unexpectedly contains raw-scale entries");

    size_t inspectedTilesetBrushes = 0;
    for (const auto &tilesetPtr : tilesetService.getTilesetRegistry().getAllTilesets()) {
      require(tilesetPtr != nullptr, "tileset registry contained a null tileset");
      for (const auto &entry : tilesetPtr->getEntries()) {
        if (!MapEditor::Domain::Tileset::isBrush(entry)) {
          continue;
        }

        const auto *brush = MapEditor::Domain::Tileset::getBrush(entry);
        require(brush != nullptr, "tileset brush entry was null after load");
        [[maybe_unused]] const auto brushType = brush->getType();
        [[maybe_unused]] const auto &brushName = brush->getName();
        [[maybe_unused]] const auto preview = brush->getPreviewDescriptor();
        ++inspectedTilesetBrushes;
      }
    }
    require(inspectedTilesetBrushes > 0,
            "tileset post-load inspection did not visit any brush entries");

    MapEditor::Services::Brushes::BorderLookupService borderLookup;
    MapEditor::Services::Brushes::WallLookupService wallLookup;
    MapEditor::Services::Brushes::TableLookupService tableLookup;
    MapEditor::Services::Brushes::CarpetLookupService carpetLookup;

    const auto northOnly = MapEditor::Services::Brushes::BorderLookupService::unpack(
        borderLookup.getBorderTypes(TileNeighbor::North));
    require(northOnly.size() == 1 && northOnly.front() == EdgeType::N,
            "border lookup north edge contract mismatch");
    const auto eastOnly = MapEditor::Services::Brushes::BorderLookupService::unpack(
        borderLookup.getBorderTypes(TileNeighbor::East));
    require(eastOnly.size() == 1 && eastOnly.front() == EdgeType::E,
            "border lookup east edge contract mismatch");
    const auto northEast = MapEditor::Services::Brushes::BorderLookupService::unpack(
        borderLookup.getBorderTypes(TileNeighbor::North | TileNeighbor::East));
    require(northEast.size() == 1 && northEast.front() == EdgeType::DNE,
            "border lookup north-east diagonal contract mismatch");
    const auto southWest = MapEditor::Services::Brushes::BorderLookupService::unpack(
        borderLookup.getBorderTypes(TileNeighbor::South | TileNeighbor::West));
    require(southWest.size() == 1 && southWest.front() == EdgeType::DSW,
            "border lookup south-west diagonal contract mismatch");
    require(wallLookup.getFullType(WallNeighbor::North) == WallAlign::SouthEnd,
            "wall lookup north contract mismatch");
    require(wallLookup.getFullType(WallNeighbor::West) == WallAlign::EastEnd,
            "wall lookup west contract mismatch");
    require(wallLookup.getFullType(WallNeighbor::North | WallNeighbor::East) ==
                WallAlign::NortheastDiagonal,
            "wall lookup north-east contract mismatch");
    require(tableLookup.getTableType(TileNeighbor::North | TileNeighbor::South) ==
                TableAlign::Vertical,
            "table lookup vertical contract mismatch");
    require(tableLookup.getTableType(TileNeighbor::East | TileNeighbor::West) ==
                TableAlign::Horizontal,
            "table lookup horizontal contract mismatch");
    const auto carpetSouth = MapEditor::Services::Brushes::CarpetLookupService::unpack(
        carpetLookup.getCarpetTypes(TileNeighbor::South));
    require(carpetSouth.size() == 1 && carpetSouth.front() == EdgeType::CSW,
            "carpet lookup south contract mismatch");

    MapEditor::Brushes::BorderBlock groupedBorder;
    groupedBorder.setGroup(77);
    groupedBorder.addItem(EdgeType::N, 60000, 1);
    registry.registerBorderTemplate(9000, std::move(groupedBorder));
    const auto *borderMetadata = registry.getBorderItemMetadata(60000);
    require(borderMetadata != nullptr && borderMetadata->group == 77 &&
                borderMetadata->alignment == EdgeType::N,
            "border item metadata contract mismatch");

    {
      MapEditor::Brushes::BrushRegistry placeholderRegistry;
      MapEditor::Domain::Tileset::TilesetRegistry placeholderTilesets;
      MapEditor::IO::TilesetXmlReader placeholderReader(placeholderRegistry,
                                                        placeholderTilesets);
      const fs::path placeholderTilesetPath = tempDir / "placeholder_tileset.xml";
      const fs::path resolveTriggerPath = tempDir / "resolve_trigger.xml";

      writeTextFile(placeholderTilesetPath,
                    "<tileset name=\"Placeholder Test\"><brush name=\"deferred brush\"/></tileset>");
      require(placeholderReader.loadTilesetFile(placeholderTilesetPath),
              "placeholder tileset load failed");

      auto *placeholderTileset =
          placeholderTilesets.getTileset("Placeholder Test");
      require(placeholderTileset != nullptr && placeholderTileset->size() == 1,
              "placeholder tileset entry missing");
      require(MapEditor::Domain::Tileset::isBrush(
                  placeholderTileset->getEntries().front()),
              "placeholder tileset did not register a brush entry");
      require(MapEditor::Domain::Tileset::getBrush(
                      placeholderTileset->getEntries().front())
                      ->getType() == MapEditor::Brushes::BrushType::Placeholder,
              "placeholder tileset entry was not a placeholder brush");

      auto deferredBrush = std::make_unique<MapEditor::Brushes::GroundBrush>(
          "deferred brush", 0, placeholderRegistry);
      deferredBrush->addGroundItem(101, 1);
      placeholderRegistry.addBrush(std::move(deferredBrush));

      writeTextFile(resolveTriggerPath,
                    "<tileset name=\"Resolve Trigger\"><separator name=\"x\"/></tileset>");
      require(placeholderReader.loadTilesetFile(resolveTriggerPath),
              "placeholder resolve trigger load failed");

      const auto *resolvedBrush = MapEditor::Domain::Tileset::getBrush(
          placeholderTileset->getEntries().front());
      require(resolvedBrush != nullptr &&
                  resolvedBrush->getType() !=
                      MapEditor::Brushes::BrushType::Placeholder &&
                  resolvedBrush ==
                      placeholderRegistry.getBrush("deferred brush"),
              "tileset placeholder was not rebound to the real brush");
    }

    auto *groundBrush = findBrush(registry, "cave", MapBrushType::Ground);
    auto *seaBrush = findBrush(registry, "sea", MapBrushType::Ground);
    auto *sandBrush = findBrush(registry, "sand", MapBrushType::Ground);
    auto *doodadBrush = findBrush(registry, "waterfall", MapBrushType::Doodad);
    auto *wallBrush = findBrush(registry, "grass wall", MapBrushType::Wall);
    auto *doorWallBrush = findBrush(registry, "stone wall", MapBrushType::Wall);
    auto *mossyWallBrush = findBrush(registry, "mossy wall", MapBrushType::Wall);
    auto *carpetBrush = findBrush(registry, "red carpet", MapBrushType::Carpet);
    auto *tableBrush = findBrush(registry, "damaged mast2", MapBrushType::Table);
    auto *earthSoftBrush = registry.getBrush("earth");
    auto *earthHardBrush = registry.getBrush("earth (stone border)");

    require(groundBrush != nullptr, "ground brush lookup failed");
    require(seaBrush != nullptr, "sea ground brush lookup failed");
    require(sandBrush != nullptr, "sand ground brush lookup failed");
    require(doodadBrush != nullptr, "doodad brush lookup failed");
    require(wallBrush != nullptr, "wall brush lookup failed");
    require(doorWallBrush != nullptr, "door wall brush lookup failed");
    require(mossyWallBrush != nullptr, "mossy wall brush lookup failed");
    auto *doorWallTyped =
        dynamic_cast<MapEditor::Brushes::WallBrush *>(doorWallBrush);
    require(doorWallTyped != nullptr, "door wall brush cast failed");
    require(carpetBrush != nullptr, "carpet brush lookup failed");
    require(tableBrush != nullptr, "table brush lookup failed");
    require(earthSoftBrush != nullptr, "earth (soft) brush lookup failed");
    require(earthHardBrush != nullptr, "earth (hard) brush lookup failed");

    const auto cavePreview = groundBrush->getPreviewDescriptor();
    require(cavePreview.kind == MapEditor::Brushes::BrushPreviewKind::ServerItem &&
                cavePreview.numericId == 351,
            "cave preview descriptor should use server item lookid");
    const auto mossyWallPreview = mossyWallBrush->getPreviewDescriptor();
    require(mossyWallPreview.kind ==
                    MapEditor::Brushes::BrushPreviewKind::ServerItem &&
                mossyWallPreview.numericId == 1933,
            "mossy wall preview descriptor should use server item lookid");

    MapEditor::Domain::ChunkedMap map;
    map.createNew(128, 128, 1098);
    map.setHouseFile("brush-smoke-houses.xml");
    map.setSpawnFile("brush-smoke-spawns.xml");

    if (const auto variantPair = findWallVariantPair(*doorWallTyped)) {
      const MapEditor::Domain::Position variantPos{28, 20, 7};
      auto *variantTile = map.getOrCreateTile(variantPos);
      require(variantTile != nullptr, "wall variant test tile missing");
      auto variantItem =
          std::make_unique<MapEditor::Domain::Item>(variantPair->first);
      variantItem->setOwnerBrushId(registry.getBrushId(doorWallBrush));
      variantTile->addItemDirect(std::move(variantItem));

      MapEditor::Brushes::DrawContext variantContext;
      variantContext.specialAction = true;
      variantContext.ownerBrushId = registry.getBrushId(doorWallBrush);
      const auto placement =
          doorWallTyped->placeWallTile(*variantTile, variantContext);
      require(placement.changed, "wall variant special action did not apply");
      require(variantTile->getItemCount() > 0 &&
                  variantTile->getItem(variantTile->getItemCount() - 1)
                          ->getServerId() == variantPair->second,
              "wall variant special action selected the wrong item");
    }

    MapEditor::Domain::History::HistoryManager history;
    MapEditor::Services::BrushSettingsService settings;
    MapEditor::Services::Preview::PreviewService previewService;
    MapEditor::Services::Preview::BrushPreviewFactory previewFactory;
    MapEditor::Brushes::BrushController controller;
    controller.initialize(&map, &history, nullptr);
    controller.setBrushRegistry(&registry);
    controller.setBrushSettingsService(&settings);
    controller.setPreviewFactory(&previewFactory);
    controller.setPreviewService(&previewService);

    for (const auto *brush : {static_cast<const MapEditor::Brushes::IBrush *>(
                                  controller.getSpawnBrush()),
                              static_cast<const MapEditor::Brushes::IBrush *>(
                                  controller.getHouseBrush()),
                              static_cast<const MapEditor::Brushes::IBrush *>(
                                  controller.getHouseExitBrush()),
                              static_cast<const MapEditor::Brushes::IBrush *>(
                                  controller.getWaypointBrush()),
                              static_cast<const MapEditor::Brushes::IBrush *>(
                                  controller.getOptionalBorderBrush()),
                              static_cast<const MapEditor::Brushes::IBrush *>(
                                  controller.getEraserBrush()),
                              static_cast<const MapEditor::Brushes::IBrush *>(
                                  controller.getPZBrush())}) {
      const auto resolvedPreview =
          MapEditor::UI::Utils::ResolveBrushPreview(brush, nullptr, nullptr);
      require(!resolvedPreview.fallbackLabel.empty(),
              "symbolic preview fallback label should not be empty");
    }

    settings.setStandardSize(2);
    controller.setBrush(groundBrush);
    previewService.updateCursor({40, 40, 7});
    require(previewService.hasPreview(), "ground brush preview is inactive");
    require(previewService.getPreviewTiles().size() ==
                settings.getBrushOffsets().size(),
            "ground preview does not expand with brush settings");

    settings.setStandardSize(0);
    previewService.regenerate();

    const MapEditor::Domain::Position cavePos{20, 20, 7};
    require(controller.applyBrush(cavePos), "ground brush paint failed");
    auto *caveTile = map.getTile(cavePos);
    require(caveTile != nullptr && caveTile->hasGround(),
            "ground brush did not create ground");
    require(controller.selectBrushFromTile(*caveTile, PickMode::Ground),
            "ground brush reselection failed");
    require(controller.getCurrentBrush() == groundBrush,
            "ground reselection chose the wrong brush");
    controller.setBrush(wallBrush);
    const auto caveResolution =
        controller.resolveBrushFromTile(*caveTile, PickMode::Ground);
    require(caveResolution.has_value() && caveResolution->brush == groundBrush,
            "ground resolver did not identify the cave brush");
    require(controller.getCurrentBrush() == wallBrush,
            "non-mutating ground resolver changed the active brush");

    auto stackedBorderItem = std::make_unique<MapEditor::Domain::Item>(60000);
    stackedBorderItem->setOwnerBrushId(registry.getBrushId(sandBrush));
    caveTile->addItemDirect(std::move(stackedBorderItem));
    const auto stackedGroundResolution =
        controller.resolveBrushFromTile(*caveTile, PickMode::Ground);
    require(stackedGroundResolution.has_value() &&
                stackedGroundResolution->brush == groundBrush,
            "ground resolver was confused by border stack ownership");
    require(controller.canSelectBrushFromTile(*caveTile, PickMode::Ground),
            "context menu should offer ground brush selection on bordered ground");

    require(controller.toggleSelectionTool(),
            "spacebar-style toggle should switch to selection mode");
    require(!controller.hasBrush(),
            "selection mode toggle did not clear the active brush");
    require(controller.toggleSelectionTool(),
            "spacebar-style toggle should restore the last brush");
    require(controller.getCurrentBrush() == wallBrush,
            "selection mode toggle did not restore the previous brush");
      clearTileBrushOwnership(*caveTile);
      const auto legacyGroundResolution =
          controller.resolveBrushFromTile(*caveTile, PickMode::Ground);
      require(legacyGroundResolution.has_value() &&
                  legacyGroundResolution->brush == groundBrush,
              "legacy ground selection fallback failed");

      auto contextMap = std::make_unique<MapEditor::Domain::ChunkedMap>();
      contextMap->createNew(32, 32, 1098);
      const MapEditor::Domain::Position contextPos{6, 6, 7};
      auto contextTile = std::make_unique<MapEditor::Domain::Tile>(contextPos);
      contextTile->setGround(std::make_unique<MapEditor::Domain::Item>(101));
      auto contextTopItem = std::make_unique<MapEditor::Domain::Item>(202);
      auto *contextTopItemPtr = contextTopItem.get();
      contextTile->addItem(std::move(contextTopItem));
      contextMap->setTile(contextPos, std::move(contextTile));

      auto contextDocument =
          std::make_unique<MapEditor::Domain::MapInstance>(std::move(contextMap));
      MapEditor::AppLogic::EditorSession contextSession(
          std::move(contextDocument), 1);
      MapEditor::Domain::SelectionSettings selectionSettings;
      MapEditor::AppLogic::MapInputController inputController(
          selectionSettings, nullptr);
      MapEditor::Domain::History::HistoryManager contextHistory;
      MapEditor::Brushes::BrushController contextBrushController;
      contextBrushController.initialize(contextSession.getMap(), &contextHistory,
                                       nullptr);
      contextBrushController.setBrushRegistry(&registry);
      contextBrushController.setBrush(groundBrush);
      inputController.setBrushController(&contextBrushController);

      bool itemPropertiesOpenedWithBrush = false;
      inputController.setOpenItemPropertiesCallback(
          [&itemPropertiesOpenedWithBrush](MapEditor::Domain::Item *) {
            itemPropertiesOpenedWithBrush = true;
          });
      inputController.onDoubleClick(contextPos, {0.0f, 0.0f},
                                    &contextSession);
      require(!itemPropertiesOpenedWithBrush,
              "double click should not open item properties while brush mode is active");

      inputController.onRightClick(contextPos, 0, {0.0f, 0.0f}, &contextSession);
      require(!contextBrushController.hasBrush(),
              "right click should toggle out of brush mode before opening context");
      const auto contextEntries =
          contextSession.getSelectionService().getEntriesAt(contextPos);
      require(contextEntries.size() == 1 &&
                  contextEntries.front().getType() ==
                      MapEditor::Domain::Selection::EntityType::Item &&
                  contextEntries.front().entity_ptr == contextTopItemPtr,
              "right click did not select the top item as context");

      contextSession.getSelectionService().selectTile(contextSession.getMap(),
                                                      contextPos);
      const auto preservedCount =
          contextSession.getSelectionService().getEntriesAt(contextPos).size();
      inputController.onRightClick(contextPos, 0, {0.0f, 0.0f}, &contextSession);
      require(contextSession.getSelectionService().getEntriesAt(contextPos).size() ==
                  preservedCount,
              "right click should preserve existing selection on the clicked tile");

      const MapEditor::Domain::Position emptyContextPos{7, 7, 7};
      inputController.onRightClick(emptyContextPos, 0, {0.0f, 0.0f},
                                   &contextSession);
      require(contextSession.getSelectionService().isEmpty(),
              "right click on empty space should clear stale selection context");

      const MapEditor::Domain::Position seaPos{20, 22, 7};
    const MapEditor::Domain::Position sandPos{21, 22, 7};
    controller.setBrush(seaBrush);
    require(controller.applyBrush(seaPos), "sea ground paint failed");
    controller.setBrush(sandBrush);
    require(controller.applyBrush(sandPos), "sand ground paint failed");
    const auto *seaTile = map.getTile(seaPos);
    const auto *sandTile = map.getTile(sandPos);
    require(seaTile != nullptr && sandTile != nullptr, "terrain tiles were not created");
    require(seaTile->getItemCount() > 0 || sandTile->getItemCount() > 0,
            "terrain/autoborder interaction did not create border items");

    const MapEditor::Domain::Position optionalPos{22, 20, 7};
    auto *optionalGroundBrush =
        findBrush(registry, "wooden floor", MapBrushType::Ground);
    require(optionalGroundBrush != nullptr,
            "optional-border ground brush lookup failed");
    controller.setBrush(optionalGroundBrush);
    require(controller.applyBrush(optionalPos),
            "optional-border ground paint failed");
    auto *optionalTile = map.getTile(optionalPos);
    require(optionalTile != nullptr && optionalTile->hasGround(),
            "optional-border test tile has no ground");
    controller.activateOptionalBorderBrush();
    require(controller.applyBrush(optionalPos),
            "optional border brush application failed");
    require(optionalTile->hasOptionalBorder(),
            "optional border state was not stored on tile");
    require(controller.selectBrushFromTile(*optionalTile,
                                           PickMode::OptionalBorder),
            "optional border reselection failed");

    const MapEditor::Domain::Position rawPos{24, 20, 7};
    auto *rawBrush = registry.getOrCreateRAWBrush(2160);
    require(rawBrush != nullptr, "raw brush creation failed");
    controller.setBrush(rawBrush);
    require(controller.applyBrush(rawPos), "raw brush paint failed");
    auto *rawTile = map.getTile(rawPos);
    require(rawTile != nullptr && rawTile->getItemCount() > 0,
            "raw brush did not place an item");
    require(controller.selectBrushFromTile(*rawTile, PickMode::Raw),
            "raw brush reselection failed");
    require(controller.getCurrentItemId().has_value() &&
                controller.getCurrentItemId().value() == 2160,
            "raw brush reselection did not preserve the item id");
    require(controller.resolveBrushFromTile(*rawTile, PickMode::Wall) ==
                std::nullopt,
            "raw tile should not resolve as a wall brush");

    const auto verifyEarthBorder =
        [&](const MapEditor::Domain::Position &center,
            std::initializer_list<MapEditor::Domain::Position> softNeighbors,
            std::string_view label) {
          controller.setBrush(earthHardBrush);
          require(controller.applyBrush(center), "earth (hard) paint failed");
          controller.setBrush(earthSoftBrush);
          for (const auto &neighborPos : softNeighbors) {
            require(controller.applyBrush(neighborPos),
                    "earth (soft) paint failed");
          }

          const auto *tile = map.getTile(center);
          require(tile != nullptr, "earth parity tile missing");

          bool hasEarthBorderId21 = false;
          for (const auto &item : tile->getItems()) {
            if (!item) continue;
            auto id = item->getServerId();
            if (id == 5631 || id == 5632 || id == 5637 || id == 5638 ||
                id == 5633 || id == 5634 || id == 5635 || id == 5636 ||
                id == 5647 || id == 5649 || id == 5650 || id == 5651) {
              hasEarthBorderId21 = true;
              break;
            }
          }

          require(!hasEarthBorderId21,
                  std::string(label)
                      .append(" hard earth should skip border against friend soft earth"));
        };

    verifyEarthBorder({60, 20, 7}, {{60, 19, 7}}, "earth north edge (friend)");
    verifyEarthBorder({64, 20, 7}, {{65, 20, 7}}, "earth east edge (friend)");
    verifyEarthBorder({66, 24, 7}, {{66, 25, 7}}, "earth south edge (friend)");
    verifyEarthBorder({62, 24, 7}, {{61, 24, 7}}, "earth west edge (friend)");
    verifyEarthBorder({68, 20, 7}, {{68, 19, 7}, {69, 20, 7}},
                      "earth north-east diagonal (friend)");
    verifyEarthBorder({72, 20, 7}, {{72, 19, 7}, {71, 20, 7}},
                      "earth north-west diagonal (friend)");
    verifyEarthBorder({68, 24, 7}, {{69, 24, 7}, {68, 25, 7}},
                      "earth south-east diagonal (friend)");
    verifyEarthBorder({72, 24, 7}, {{71, 24, 7}, {72, 25, 7}},
                      "earth south-west diagonal (friend)");

    auto doodadProvider = previewFactory.createProvider(doodadBrush, &settings);
    require(doodadProvider != nullptr && doodadProvider->isActive(),
            "doodad preview provider was not created");
    doodadProvider->updateCursorPosition({50, 20, 7});
    const auto &doodadPreviewTiles = doodadProvider->getTiles();
    require(!doodadPreviewTiles.empty(), "doodad preview is empty");
    require(doodadPreviewTiles.size() > 1 ||
                doodadPreviewTiles.front().relativePosition.x != 0 ||
                doodadPreviewTiles.front().relativePosition.y != 0 ||
                doodadPreviewTiles.front().relativePosition.z != 0,
            "doodad preview did not expose a composite/offset footprint");

    const MapEditor::Domain::Position doodadPos{50, 20, 7};
    const auto doodadTileCountBefore = countTiles(map);
    controller.setBrush(doodadBrush);
    require(controller.applyBrush(doodadPos), "doodad brush paint failed");
    require(countTiles(map) >= doodadTileCountBefore,
            "doodad paint unexpectedly removed tiles");
    const auto doodadPaintedPos = findTileWithItemsNear(map, doodadPos, 2);
    require(doodadPaintedPos.has_value(),
            "doodad placement did not affect the map");
    auto *doodadTile = map.getTile(*doodadPaintedPos);
    require(doodadTile != nullptr, "doodad tile lookup failed");
    require(controller.selectBrushFromTile(*doodadTile, PickMode::Doodad),
            "doodad brush reselection failed");
    clearTileBrushOwnership(*doodadTile);
    require(controller.resolveBrushFromTile(*doodadTile, PickMode::Doodad)
                .has_value(),
            "legacy doodad selection fallback failed");

    const MapEditor::Domain::Position wallLeftPos{30, 20, 7};
    const MapEditor::Domain::Position wallCenterPos{31, 20, 7};
    const MapEditor::Domain::Position wallRightPos{32, 20, 7};
    controller.setBrush(groundBrush);
    require(controller.applyBrush(wallLeftPos),
            "left wall ground paint failed");
    controller.setBrush(groundBrush);
    require(controller.applyBrush(wallCenterPos),
            "center wall ground paint failed");
    controller.setBrush(groundBrush);
    require(controller.applyBrush(wallRightPos),
            "right wall ground paint failed");
    controller.setBrush(doorWallBrush);
    require(controller.applyBrush(wallLeftPos), "left wall brush paint failed");
    require(controller.applyBrush(wallCenterPos),
            "center wall brush paint failed");
    require(controller.applyBrush(wallRightPos),
            "right wall brush paint failed");
    auto *wallTile = map.getTile(wallCenterPos);
    require(wallTile != nullptr && wallTile->getItemCount() > 0,
            "wall brush did not place wall items");
    const auto wallItemBeforeDoor =
        wallTile->getItem(wallTile->getItemCount() - 1)->getServerId();
    const auto specialClickItemIndex =
        findFirstOwnedItemIndex(*wallTile, *doorWallBrush);
    require(specialClickItemIndex.has_value(),
            "single-tile wall special click found no owned wall item");
    const auto specialClickItemBefore =
        wallTile->getItem(*specialClickItemIndex)->getServerId();
    const auto expectedSpecialClickItem =
        doorWallTyped->findNextWallVariant(specialClickItemBefore);
    controller.setBrush(doorWallBrush);
    require(controller.applyBrush(wallCenterPos, GLFW_MOD_ALT),
            "single-tile wall special click failed");
    wallTile = map.getTile(wallCenterPos);
    require(wallTile != nullptr, "wall tile missing after special click");
    require(wallTile->getItemCount() > *specialClickItemIndex,
            "wall special click changed the expected stack shape");
    if (expectedSpecialClickItem) {
      require(wallTile->getItem(*specialClickItemIndex)->getServerId() ==
                  *expectedSpecialClickItem,
              "single-tile wall special click selected the wrong variant");
    }
    controller.activateMagicDoorBrush();
    require(controller.applyBrush(wallCenterPos), "door brush paint failed");
    require(wallTile->getItemCount() > 0,
            "door brush left the wall tile without any wall items");
    const auto wallItemAfterDoor =
        wallTile->getItem(wallTile->getItemCount() - 1)->getServerId();
    require(wallItemAfterDoor != wallItemBeforeDoor,
            "door brush did not alter the wall tile contents");
    require(tileContainsItemId(*wallTile, 6265),
            "door brush did not place the expected magic door item");
    require(controller.canSwitchDoorAt(wallCenterPos),
            "switch door controller did not identify the placed door");
    require(controller.switchDoorAt(wallCenterPos), "switch door controller failed");
    require(!tileContainsItemId(*wallTile, 6265),
            "switch door controller did not change the active door variant");
    require(controller.selectBrushFromTile(*wallTile, PickMode::Door),
            "door brush reselection failed");
    require(controller.getCurrentBrush() != nullptr &&
                controller.getCurrentBrush()->getType() == MapBrushType::Door,
            "door reselection did not activate a door brush");
    clearTileBrushOwnership(*wallTile);
    require(controller.resolveBrushFromTile(*wallTile, PickMode::Door)
                .has_value(),
            "legacy door selection fallback failed");
    require(controller.eraseBrush(wallCenterPos), "door brush erase failed");
    require(!tileContainsItemId(*wallTile, 6265),
            "door brush erase left the magic door item behind");
    require(tileContainsItemId(*wallTile, wallItemBeforeDoor),
            "door brush erase did not restore the wallized base wall item");
    require(!controller.resolveBrushFromTile(*wallTile, PickMode::Door)
                 .has_value(),
            "door selection fallback still identified a removed door");

    const MapEditor::Domain::Position decoLeftPos{44, 24, 7};
    const MapEditor::Domain::Position decoCenterPos{45, 24, 7};
    const MapEditor::Domain::Position decoRightPos{46, 24, 7};
    for (const auto &pos :
         std::array{decoLeftPos, decoCenterPos, decoRightPos}) {
      controller.setBrush(groundBrush);
      require(controller.applyBrush(pos),
              "wall decoration support ground paint failed");
      controller.setBrush(doorWallBrush);
      require(controller.applyBrush(pos), "wall decoration base wall paint failed");
    }

    controller.setBrush(mossyWallBrush);
    require(controller.applyBrush(decoCenterPos),
            "wall decoration brush paint failed");
    auto *decoTile = map.getTile(decoCenterPos);
    require(decoTile != nullptr && decoTile->getItemCount() > 1,
            "wall decoration brush did not stack over the base wall");
    const auto decorationCountBeforeRebuild =
        countOwnedItems(*decoTile, *mossyWallBrush);
    require(decorationCountBeforeRebuild > 0,
            "wall decoration brush did not place an owned decoration item");

    controller.setBrush(doorWallBrush);
    require(controller.eraseBrush(decoLeftPos),
            "wall decoration neighbor erase failed");
    decoTile = map.getTile(decoCenterPos);
    require(decoTile != nullptr, "wall decoration tile missing after rebuild");
    const auto decorationCountAfterRebuild =
        countOwnedItems(*decoTile, *mossyWallBrush);
    require(decorationCountAfterRebuild == decorationCountBeforeRebuild,
            "wall rebuild collapsed the stacked wall decoration");

    const std::array normalDoorCenters{
        MapEditor::Domain::Position{47, 20, 7},
        MapEditor::Domain::Position{51, 20, 7},
    };
    for (const auto &centerPos : normalDoorCenters) {
      controller.setBrush(groundBrush);
      require(controller.applyBrush({centerPos.x - 1, centerPos.y, centerPos.z}),
              "normal door left support ground paint failed");
      controller.setBrush(groundBrush);
      require(controller.applyBrush(centerPos),
              "normal door center ground paint failed");
      controller.setBrush(groundBrush);
      require(controller.applyBrush({centerPos.x + 1, centerPos.y, centerPos.z}),
              "normal door right support ground paint failed");
      controller.setBrush(doorWallBrush);
      require(controller.applyBrush({centerPos.x - 1, centerPos.y, centerPos.z}),
              "normal door left wall paint failed");
      require(controller.applyBrush(centerPos), "normal door center wall paint failed");
      require(controller.applyBrush({centerPos.x + 1, centerPos.y, centerPos.z}),
              "normal door right wall paint failed");
    }

    controller.activateNormalDoorBrush();
    require(controller.applyBrush(normalDoorCenters[0]),
            "normal door paint without shift failed");
    const auto *normalDoorTile = map.getTile(normalDoorCenters[0]);
    require(normalDoorTile != nullptr && normalDoorTile->getItemCount() > 0,
            "normal door tile lookup failed");
    const auto normalDoorId =
        normalDoorTile->getItem(normalDoorTile->getItemCount() - 1)->getServerId();

    controller.activateLockedDoorBrush();
    require(controller.applyBrush(normalDoorCenters[1]),
            "locked door paint failed");
    const auto *lockedDoorTile = map.getTile(normalDoorCenters[1]);
    require(lockedDoorTile != nullptr && lockedDoorTile->getItemCount() > 0,
            "locked door tile lookup failed");
    const auto lockedDoorId =
        lockedDoorTile->getItem(lockedDoorTile->getItemCount() - 1)->getServerId();
    require(normalDoorId != lockedDoorId,
            "locked door brush did not choose a different variant");

    const MapEditor::Domain::Position carpetPos{34, 20, 7};
    controller.setBrush(groundBrush);
    require(controller.applyBrush(carpetPos), "carpet test ground paint failed");
    controller.setBrush(carpetBrush);
    require(controller.applyBrush(carpetPos), "carpet brush paint failed");
    require(map.getTile(carpetPos) != nullptr &&
                map.getTile(carpetPos)->getItemCount() > 0,
            "carpet brush did not place aligned items");
    clearTileBrushOwnership(*map.getTile(carpetPos));
    require(controller.resolveBrushFromTile(*map.getTile(carpetPos),
                                            PickMode::Carpet)
                .has_value(),
            "legacy carpet selection fallback failed");

    const MapEditor::Domain::Position tableSupportPos{36, 20, 7};
    const MapEditor::Domain::Position tablePos{37, 20, 7};
    controller.setBrush(groundBrush);
    require(controller.applyBrush(tableSupportPos),
            "table support ground paint failed");
    controller.setBrush(groundBrush);
    require(controller.applyBrush(tablePos), "table test ground paint failed");
    controller.setBrush(tableBrush);
    require(controller.applyBrush(tableSupportPos),
            "table support brush paint failed");
    require(controller.applyBrush(tablePos), "table brush paint failed");
    require(map.getTile(tableSupportPos) != nullptr &&
                map.getTile(tableSupportPos)->getItemCount() > 0,
            "table support brush did not place aligned items");
    require(map.getTile(tablePos) != nullptr &&
                map.getTile(tablePos)->getItemCount() > 0,
            "table brush did not place aligned items");
    require(controller.selectBrushFromTile(*map.getTile(tablePos), PickMode::Table),
            "table brush reselection failed");
    clearTileBrushOwnership(*map.getTile(tablePos));
    require(controller.resolveBrushFromTile(*map.getTile(tablePos),
                                            PickMode::Table)
                .has_value(),
            "legacy table selection fallback failed");

    const MapEditor::Domain::Position rotatePos{38, 20, 7};
    controller.setBrush(groundBrush);
    require(controller.applyBrush(rotatePos), "rotate test ground paint failed");
    auto *rotatableRawBrush =
        dynamic_cast<MapEditor::Brushes::RawBrush *>(registry.getOrCreateRAWBrush(1650));
    require(rotatableRawBrush != nullptr, "rotatable raw brush creation failed");
    MapEditor::Domain::ItemType rotatableType;
    rotatableType.server_id = 1650;
    rotatableType.client_id = 1650;
    rotatableType.rotateTo = 1651;
    rotatableRawBrush->setCachedType(&rotatableType);
    controller.setBrush(rotatableRawBrush);
    require(controller.applyBrush(rotatePos), "rotatable raw brush paint failed");
    auto *rotateTile = map.getTile(rotatePos);
    require(rotateTile != nullptr && tileContainsItemId(*rotateTile, 1650),
            "rotatable raw brush did not place the expected item");
    require(controller.canRotateItemAt(rotatePos),
            "rotate controller did not identify rotatable item");
    require(controller.rotateItemAt(rotatePos), "rotate controller failed");
    require(tileContainsItemId(*rotateTile, 1651),
            "rotate controller did not advance item to rotateTo target");

    MapEditor::Services::Selection::SelectionService selection;
    selection.selectTile(&map, cavePos);
    require(!selection.getEntriesAt(cavePos).empty(),
            "selection did not include the painted tile entries");
    selection.selectTile(&map, rawPos);
    require(selection.getPositions().size() >= 2,
            "selection did not preserve multi-tile selection");

    const MapEditor::Domain::Position housePos{40, 20, 7};
    controller.getHouseBrush()->setHouseId(42);
    controller.activateHouseBrush();
    require(controller.applyBrush(housePos), "house brush paint failed");
    auto *houseTile = map.getTile(housePos);
    require(houseTile != nullptr && houseTile->getHouseId() == 42,
            "house brush did not assign the house id");
    auto *house = map.getHouse(42);
    require(house != nullptr, "house metadata was not created");
    house->town_id = 1;
    require(controller.selectBrushFromTile(*houseTile, PickMode::House),
            "house brush reselection failed");
    require(controller.getHouseBrush()->getHouseId() == 42,
            "house brush reselection lost the selected house id");

    const MapEditor::Domain::Position houseExitPos{41, 20, 7};
    controller.setBrush(groundBrush);
    require(controller.applyBrush(houseExitPos),
            "house exit ground paint failed");
    controller.getHouseExitBrush()->setHouseId(42);
    controller.activateHouseExitBrush();
    require(controller.applyBrush(houseExitPos), "house exit paint failed");
    require(map.getHouse(42)->entry_position == houseExitPos,
            "house exit position was not stored");
    auto *houseExitTile = map.getTile(houseExitPos);
    require(houseExitTile != nullptr, "house exit tile does not exist");
    require(controller.selectBrushFromTile(*houseExitTile, PickMode::HouseExit),
            "house exit brush reselection failed");
    require(controller.getHouseExitBrush()->getHouseId() == 42,
            "house exit reselection lost the owning house id");

    const MapEditor::Domain::Position waypointPos{42, 20, 7};
    controller.getWaypointBrush()->setWaypointName("smoke-waypoint");
    controller.activateWaypointBrush();
    require(controller.applyBrush(waypointPos), "waypoint brush paint failed");
    require(map.getWaypointAt(waypointPos) != nullptr,
            "waypoint metadata was not stored");
    auto *waypointTile = map.getTile(waypointPos);
    require(waypointTile != nullptr, "waypoint tile does not exist");
    require(controller.selectBrushFromTile(*waypointTile, PickMode::Waypoint),
            "waypoint brush reselection failed");
    require(controller.getWaypointBrush()->getWaypointName() == "smoke-waypoint",
            "waypoint reselection lost the waypoint name");

    const MapEditor::Domain::Position spawnPos{43, 20, 7};
    controller.activateSpawnBrush();
    require(controller.applyBrush(spawnPos), "spawn brush paint failed");
    auto *spawnTile = map.getTile(spawnPos);
    require(spawnTile != nullptr && spawnTile->hasSpawn(),
            "spawn metadata was not stored");
    require(controller.selectBrushFromTile(*spawnTile, PickMode::Spawn),
            "spawn brush reselection failed");
    require(controller.getCurrentBrush() != nullptr &&
                controller.getCurrentBrush()->getType() == MapBrushType::Spawn,
            "spawn reselection did not activate the spawn brush");

    const MapEditor::Domain::Position zonePos{44, 20, 7};
    controller.activatePZBrush();
    require(controller.applyBrush(zonePos), "zone brush paint failed");
    auto *zoneTile = map.getTile(zonePos);
    require(zoneTile != nullptr &&
                zoneTile->hasFlag(MapEditor::Domain::TileFlag::ProtectionZone),
            "zone flag was not stored");
    require(controller.selectBrushFromTile(*zoneTile, PickMode::ProtectionZone),
            "zone brush reselection failed");
    clearTileBrushOwnership(*zoneTile);
    require(controller.resolveBrushFromTile(*zoneTile, PickMode::ProtectionZone)
                .has_value(),
            "legacy zone selection fallback failed");

    const MapEditor::Domain::Position erasePos{45, 20, 7};
    controller.setBrush(groundBrush);
    require(controller.applyBrush(erasePos), "eraser test ground paint failed");
    controller.setBrush(rawBrush);
    require(controller.applyBrush(erasePos), "eraser test raw paint failed");
    controller.activateSpawnBrush();
    require(controller.applyBrush(erasePos), "eraser test spawn paint failed");
    controller.activatePZBrush();
    require(controller.applyBrush(erasePos), "eraser test zone paint failed");
    controller.getWaypointBrush()->setWaypointName("erase-me");
    controller.activateWaypointBrush();
    require(controller.applyBrush(erasePos), "eraser test waypoint paint failed");
    controller.getHouseBrush()->setHouseId(99);
    controller.activateHouseBrush();
    require(controller.applyBrush(erasePos), "eraser test house paint failed");
    auto *eraseTile = map.getTile(erasePos);
    require(eraseTile != nullptr, "eraser test tile missing");

    // Manually add a border item to verify it is preserved by the eraser
    auto borderItem = std::make_unique<MapEditor::Domain::Item>(9003);
    static MapEditor::Domain::ItemType borderType;
    borderType.server_id = 9003;
    borderType.client_id = 9003;
    borderType.flags = MapEditor::Domain::ItemFlag::AlwaysOnBottom;
    borderType.always_on_top_order = 1;
    borderItem->setType(&borderType);
    eraseTile->addItem(std::move(borderItem));

    controller.activateEraserBrush();
    require(controller.applyBrush(erasePos), "eraser brush paint failed");
    require(eraseTile->getGround() != nullptr && eraseTile->getItemCount() == 1 &&
                eraseTile->getItem(0)->getServerId() == 9003 &&
                !eraseTile->hasSpawn() &&
                !eraseTile->hasFlag(MapEditor::Domain::TileFlag::ProtectionZone) &&
                eraseTile->getHouseId() == 0 &&
                map.getWaypointAt(erasePos) == nullptr,
            "eraser brush did not clear painted tile state while preserving borders");
    eraseTile->clearItems();
    auto smartRes = controller.resolveBrushFromTile(*eraseTile, PickMode::Smart);
    require(smartRes.has_value() && smartRes->brush == groundBrush,
            "erased tile should resolve to ground brush");

    const fs::path housesXml = tempDir / "houses.xml";
    require(MapEditor::IO::HouseXmlWriter::write(housesXml, map),
            "houses.xml write failed");
    MapEditor::Domain::ChunkedMap houseRoundTrip;
    const auto houseRead = MapEditor::IO::HouseXmlReader::read(housesXml, houseRoundTrip);
    require(houseRead.success, "houses.xml read failed");
    const auto *roundTripHouse = houseRoundTrip.getHouse(42);
    require(roundTripHouse != nullptr &&
                roundTripHouse->entry_position == houseExitPos,
            "house entry position was not preserved");

    const fs::path spawnsXml = tempDir / "spawns.xml";
    require(MapEditor::IO::SpawnXmlWriter::write(spawnsXml, map),
            "spawns.xml write failed");
    MapEditor::Domain::ChunkedMap spawnRoundTrip;
    const auto spawnRead = MapEditor::IO::SpawnXmlReader::read(spawnsXml, spawnRoundTrip);
    require(spawnRead.success, "spawns.xml read failed");
    const auto *roundTripSpawnTile = spawnRoundTrip.getTile(spawnPos);
    require(roundTripSpawnTile != nullptr && roundTripSpawnTile->hasSpawn(),
            "spawn state was not preserved");

    const fs::path otbmPath = tempDir / "brush-smoke.otbm";
    const auto writeResult = MapEditor::IO::OtbmWriter::write(otbmPath, map);
    require(writeResult.success, "OTBM write failed");
    const auto readResult = MapEditor::IO::OtbmReader::read(otbmPath);
    require(readResult.success && readResult.map != nullptr,
            "OTBM read failed");
    const auto *roundTripWaypoint = readResult.map->getWaypointAt(waypointPos);
    require(roundTripWaypoint != nullptr &&
                roundTripWaypoint->name == "smoke-waypoint",
            "waypoint OTBM round-trip failed");
    const auto *roundTripHouseTile = readResult.map->getTile(housePos);
    require(roundTripHouseTile != nullptr &&
                roundTripHouseTile->getHouseId() == 42,
            "house tile OTBM round-trip failed");

    fs::remove_all(tempDir, cleanupError);
    std::cout << "Brush smoke OK" << std::endl;
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "Brush smoke failed: " << ex.what() << std::endl;
    return 1;
  }
}
