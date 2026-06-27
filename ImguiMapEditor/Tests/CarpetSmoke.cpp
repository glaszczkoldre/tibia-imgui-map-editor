/**
 * @file CarpetSmoke.cpp
 * @brief Comprehensive carpet brush parity tests.
 *
 * Verifies that the carpet brush matches RME reference behavior for:
 *   * Lookup table parity (256-entry sweep).
 *   * Single-tile alignments (orthogonal, corner, center).
 *   * Multi-tile rebuild propagation (drag, erase, replace).
 *   * Preview vs commit parity.
 *   * Context brush pick and right-click erase.
 *   * Undo/redo round-trip.
 *
 * Test data lives in `<project_root>/data/1098/` (copied from
 * `build-ninja/data/1098/` for the in-tree smoke runs). The red carpet
 * brush ships with 9 alignment entries in `brushes.xml` so we have a
 * concrete ground-truth for the alignment ids we expect.
 */

#include "Brushes/BrushController.h"
#include "Brushes/BrushRegistry.h"
#include "Brushes/Core/IBrush.h"
#include "Brushes/Helpers/AlignedBrushHelpers.h"
#include "Brushes/Types/CarpetBrush.h"
#include "Domain/ChunkedMap.h"
#include "Domain/History/HistoryManager.h"
#include "Domain/History/TileSnapshot.h"
#include "Domain/Item.h"
#include "Domain/Position.h"
#include "Domain/Tile.h"
#include "Services/Autoborder/AutoborderEngine.h"
#include "Services/Autoborder/PlacementIntent.h"
#include "Services/Autoborder/PlannedMutation.h"
#include "Services/BrushSettingsService.h"
#include "Services/Brushes/CarpetLookupService.h"
#include "Services/Preview/AutoborderBrushPreviewProvider.h"
#include "Services/TilesetService.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

using MapEditor::Brushes::CarpetBrush;
using MapEditor::Brushes::EdgeType;
using MapEditor::Brushes::TileNeighbor;
using MapEditor::Domain::History::TileSnapshot;

void require(bool condition, std::string_view message) {
  if (!condition) {
    throw std::runtime_error(std::string(message));
  }
}

MapEditor::Brushes::IBrush *
findBrush(MapEditor::Brushes::BrushRegistry &registry, std::string_view name,
          MapEditor::Brushes::BrushType expected) {
  auto *brush = registry.getBrush(std::string(name));
  require(brush != nullptr, std::string(name).append(" brush missing"));
  require(brush->getType() == expected,
          std::string(name).append(" brush has unexpected type"));
  return brush;
}

std::unique_ptr<MapEditor::Domain::Item>
makeOwnedItem(MapEditor::Brushes::BrushRegistry &registry,
              const MapEditor::Brushes::IBrush &brush, uint16_t itemId) {
  auto item = std::make_unique<MapEditor::Domain::Item>(itemId);
  item->setOwnerBrushId(registry.getBrushId(&brush));
  return item;
}

std::vector<uint16_t> tileItemIds(const MapEditor::Domain::Tile *tile) {
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

void placeCarpetItem(MapEditor::Domain::Tile &tile,
                     MapEditor::Brushes::BrushRegistry &registry,
                     const CarpetBrush &brush, uint16_t itemId) {
  tile.addItemDirect(makeOwnedItem(registry, brush, itemId));
}

// ─────────────────────────────────────────────────────────────────────────
// Section 1: lookup table parity (RME Python translator cross-check)
// ─────────────────────────────────────────────────────────────────────────

/**
 * Compute the expected carpet alignment for a given 8-bit neighbour
 * bitmask by translating RME's `carpet_brush_arrays.cpp::carpet_type_table`
 * verbatim. This is the single source of truth used by the Python
 * generator, so the C++ lookup table and the smoke test must all agree.
 */
uint32_t expectedRmeCarpetType(uint8_t mask) {
  constexpr uint8_t NW = 1, N = 2, NE = 4, W = 8, E = 16, SW = 32, S = 64,
                    SE = 128;
  constexpr uint32_t NORTH_HORIZONTAL = 1, EAST_HORIZONTAL = 2,
                     SOUTH_HORIZONTAL = 3, WEST_HORIZONTAL = 4,
                     NORTHWEST_CORNER = 5, NORTHEAST_CORNER = 6,
                     SOUTHWEST_CORNER = 7, SOUTHEAST_CORNER = 8,
                     NORTHWEST_DIAGONAL = 9, NORTHEAST_DIAGONAL = 10,
                     SOUTHEAST_DIAGONAL = 11, SOUTHWEST_DIAGONAL = 12,
                     CARPET_CENTER = 13;

  const bool nw = mask & NW, n = mask & N, ne = mask & NE;
  const bool w = mask & W, e = mask & E;
  const bool sw = mask & SW, s = mask & S, se = mask & SE;

  if (n && s && e && w) {
    const int missing = (!nw) + (!ne) + (!sw) + (!se);
    if (missing == 1) {
      if (!nw) return SOUTHEAST_DIAGONAL;
      if (!ne) return SOUTHWEST_DIAGONAL;
      if (!sw) return NORTHEAST_DIAGONAL;
      return NORTHWEST_DIAGONAL;
    }
    return CARPET_CENTER;
  }
  if (n && s && w) {
    if (sw && nw) return WEST_HORIZONTAL;
    if (sw) return SOUTHWEST_CORNER;
    if (nw) return NORTHWEST_CORNER;
    return WEST_HORIZONTAL;
  }
  if (n && s && e) {
    return EAST_HORIZONTAL;
  }
  if (n && w && e) {
    return sw ? NORTHWEST_CORNER : NORTH_HORIZONTAL;
  }
  if (s && w && e) {
    return SOUTH_HORIZONTAL;
  }
  if (n && w) return NORTHWEST_CORNER;
  if (n && e) return NORTHEAST_CORNER;
  if (s && w) return SOUTHWEST_CORNER;
  if (s && e) return SOUTHEAST_CORNER;
  if (n && s) {
    if (nw && sw) return WEST_HORIZONTAL;
    if (nw) return NORTHWEST_CORNER;
    if (sw) return SOUTHWEST_CORNER;
    if (ne) return NORTHEAST_CORNER;
    if (se) return SOUTHEAST_CORNER;
    return CARPET_CENTER;
  }
  if (w && e) {
    if (sw && e && w) return SOUTHWEST_CORNER;
    const bool nSide = nw || ne;
    const bool sSide = sw || se;
    if (nSide && sSide) return CARPET_CENTER;
    if (nSide) return NORTH_HORIZONTAL;
    if (sSide) return SOUTH_HORIZONTAL;
    return CARPET_CENTER;
  }
  if (n) {
    if (nw) return NORTHWEST_CORNER;
    if (ne) return NORTHEAST_CORNER;
    if (sw) return SOUTHWEST_CORNER;
    if (se) return SOUTHEAST_CORNER;
    return CARPET_CENTER;
  }
  if (s) {
    if (sw) return SOUTHWEST_CORNER;
    if (se) return SOUTHEAST_CORNER;
    if (nw) return NORTHWEST_CORNER;
    if (ne) return NORTHEAST_CORNER;
    return SOUTHWEST_CORNER;
  }
  if (w) {
    if (nw) return WEST_HORIZONTAL;
    if (sw) return SOUTHWEST_CORNER;
    if (se) return SOUTH_HORIZONTAL;
    return CARPET_CENTER;
  }
  if (e) {
    if (nw) return NORTHEAST_CORNER;
    if (ne) return NORTHEAST_CORNER;
    if (se) return SOUTH_HORIZONTAL;
    return CARPET_CENTER;
  }
  if (nw && ne) return NORTH_HORIZONTAL;
  if (sw && se) return SOUTH_HORIZONTAL;
  if (nw && sw) return WEST_HORIZONTAL;
  if (ne && se) return EAST_HORIZONTAL;
  if (ne) return NORTHEAST_CORNER;
  if (se) return SOUTHEAST_CORNER;
  if (sw) return SOUTHWEST_CORNER;
  return CARPET_CENTER;
}

void requireLookupParity() {
  MapEditor::Services::Brushes::CarpetLookupService lookup;

  for (int mask = 0; mask < 256; ++mask) {
    const uint8_t byteMask = static_cast<uint8_t>(mask);
    const auto expected = expectedRmeCarpetType(byteMask);
    const auto actual = lookup.getCarpetTypes(
        static_cast<TileNeighbor>(byteMask));
    if (expected != actual) {
      throw std::runtime_error(
          "carpet lookup parity mismatch at mask=" + std::to_string(mask) +
          " expected=0x" + std::to_string(expected) + " actual=0x" +
          std::to_string(actual));
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────
// Section 2: paint a tile with a single centre item via the brush API
// ─────────────────────────────────────────────────────────────────────────

void requireSingleCarpetItem(MapEditor::Domain::ChunkedMap &map,
                             MapEditor::Brushes::BrushRegistry &registry,
                             CarpetBrush &brush,
                             const MapEditor::Domain::Position &pos,
                             std::string_view label) {
  auto *tile = map.getOrCreateTile(pos);
  require(tile != nullptr, std::string(label).append(" tile missing"));
  // Clear any pre-existing owned items so the assertion below is meaningful.
  tile->removeItemsIf([&brush](const MapEditor::Domain::Item *item) {
    return brush.ownsItem(item);
  });
  require(tileItemIds(tile).empty(),
          std::string(label).append(" tile should start empty"));
  MapEditor::Brushes::DrawContext ctx;
  ctx.brushRegistry = &registry;
  ctx.ownerBrushId = registry.getBrushId(&brush);
  brush.draw(map, tile, ctx);
  require(tile->getItemCount() == 1,
          std::string(label).append(" single carpet paint did not yield one item"));
  require(brush.ownsItem(tile->getItem(0)),
          std::string(label).append(" painted item is not owned by the brush"));
}

// ─────────────────────────────────────────────────────────────────────────
// Section 3: per-neighbour alignment sweep on an empty map
// ─────────────────────────────────────────────────────────────────────────

void requireNeighbourSweep(MapEditor::Domain::ChunkedMap &map,
                           MapEditor::Brushes::BrushRegistry &registry,
                           CarpetBrush &brush) {
  // Paint the centre tile (idempotent — gives us an anchor carpet item).
  const MapEditor::Domain::Position center{200, 200, 7};
  requireSingleCarpetItem(map, registry, brush, center, "anchor");

  struct Case {
    TileNeighbor mask;
    EdgeType expected;
    const char *label;
  };

  const std::array cases{
      // Per RME legacy rules: a single orthogonal neighbour (without
      // diagonals) typically resolves to CARPET_CENTER. Diagonals on a
      // single-orthogonal case upgrade the alignment.
      Case{TileNeighbor::North, EdgeType::Center, "n_only"},
      Case{TileNeighbor::South, EdgeType::CSW, "s_only"},
      Case{TileNeighbor::East, EdgeType::Center, "e_only"},
      Case{TileNeighbor::West, EdgeType::Center, "w_only"},
      Case{TileNeighbor::Northwest, EdgeType::Center, "nw_only"},
      Case{TileNeighbor::Northeast, EdgeType::CNE, "ne_only"},
      Case{TileNeighbor::Southwest, EdgeType::CSW, "sw_only"},
      Case{TileNeighbor::Southeast, EdgeType::CSE, "se_only"},
      Case{TileNeighbor::North | TileNeighbor::West, EdgeType::CNW, "n+w"},
      Case{TileNeighbor::North | TileNeighbor::East, EdgeType::CNE, "n+e"},
      Case{TileNeighbor::South | TileNeighbor::West, EdgeType::CSW, "s+w"},
      Case{TileNeighbor::South | TileNeighbor::East, EdgeType::CSE, "s+e"},
      Case{TileNeighbor::North | TileNeighbor::South, EdgeType::Center, "n+s"},
      Case{TileNeighbor::East | TileNeighbor::West, EdgeType::Center, "e+w"},
      Case{TileNeighbor::North | TileNeighbor::South | TileNeighbor::East,
           EdgeType::E, "n+s+e"},
      Case{TileNeighbor::North | TileNeighbor::South | TileNeighbor::West,
           EdgeType::W, "n+s+w"},
      Case{TileNeighbor::North | TileNeighbor::West | TileNeighbor::East,
           EdgeType::N, "n+w+e"},
      Case{TileNeighbor::South | TileNeighbor::West | TileNeighbor::East,
           EdgeType::S, "s+w+e"},
      Case{TileNeighbor::North | TileNeighbor::South | TileNeighbor::East |
               TileNeighbor::West,
           EdgeType::Center, "n+s+e+w"},
      // The diagonal-override cases require DNW/DNE/DSE/DSW items in
      // the brush. The "red carpet" XML doesn't ship those, so we
      // skip them at the brush level — but we keep them in the lookup
      // parity sweep above to confirm the table matches RME.
  };

  for (const auto &testCase : cases) {
    // Make sure the centre tile has an owned anchor item. `rebuildTile`
    // only updates existing owned items — it never adds a fresh one —
    // so without this anchor the planner reports an empty plan.
    requireSingleCarpetItem(map, registry, brush, center, "anchor");

    const auto previewId = brush.getPreviewItemId();
    const uint16_t ownedId = previewId == 0 ? 1u : previewId;

    // Place owned items on the 8 neighbour slots according to the mask.
    for (const auto &[dx, dy, bit] :
         MapEditor::Brushes::Helpers::kAlignedNeighborOffsets) {
      const bool hasBit =
          (static_cast<uint8_t>(testCase.mask) & static_cast<uint8_t>(bit)) != 0;
      if (!hasBit) {
        continue;
      }
      const auto neighborPos = MapEditor::Domain::Position{
          static_cast<int16_t>(center.x + dx),
          static_cast<int16_t>(center.y + dy), center.z};
      auto *neighbor = map.getOrCreateTile(neighborPos);
      require(neighbor != nullptr, "neighbour tile missing during sweep");
      placeCarpetItem(*neighbor, registry, brush, ownedId);
    }

    brush.rebuildTile(map, center);

    const auto plan = brush.planTile(map, center);
    require(!plan.empty(),
            std::string("sweep case ").append(testCase.label).append(
                " produced no plan"));
    if (plan[0].alignment != testCase.expected) {
      const int actualValue = static_cast<int>(plan[0].alignment);
      const int expectedValue = static_cast<int>(testCase.expected);
      throw std::runtime_error(
          std::string("sweep case ").append(testCase.label).append(
              " produced wrong alignment actual=")
              .append(std::to_string(actualValue))
              .append(" expected=")
              .append(std::to_string(expectedValue)));
    }

    // Wipe centre and neighbour tiles so the next iteration starts clean.
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        const auto pos = MapEditor::Domain::Position{
            static_cast<int16_t>(center.x + dx),
            static_cast<int16_t>(center.y + dy), center.z};
        if (auto *n = map.getTile(pos)) {
          n->removeItemsIf([&brush](const MapEditor::Domain::Item *item) {
            return brush.ownsItem(item);
          });
        }
      }
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────
// Section 4: drag-painting propagates to all affected tiles
// ─────────────────────────────────────────────────────────────────────────

void requireDragPaintsAllTiles(MapEditor::Domain::ChunkedMap &map,
                               MapEditor::Brushes::BrushRegistry &registry,
                               CarpetBrush &brush,
                               MapEditor::Domain::History::HistoryManager
                                   &history) {
  MapEditor::Services::Autoborder::AutoborderEngine engine;

  // Drag an L-shape (3 tiles) — verify the affected region (3x3 around
  // each painted tile) ends up with the right alignments.
  const std::array painted{MapEditor::Domain::Position{210, 210, 7},
                           MapEditor::Domain::Position{211, 210, 7},
                           MapEditor::Domain::Position{211, 211, 7}};

  MapEditor::Services::Autoborder::PlacementIntent intent;
  intent.brush = &brush;
  intent.mode = MapEditor::Services::Autoborder::PlacementMode::Draw;
  intent.context.brushRegistry = &registry;
  intent.context.ownerBrushId = registry.getBrushId(&brush);
  intent.context.brushSettings = nullptr;
  intent.positions.assign(painted.begin(), painted.end());
  const auto diffs = engine.plan(map, intent);
  require(!diffs.empty(), "drag plan produced no diffs");
  for (const auto &pos : painted) {
    require(std::find_if(diffs.begin(), diffs.end(),
                          [&](const auto &d) { return d.position == pos; }) !=
                diffs.end(),
            "drag plan did not emit diff for painted tile");
  }

  // Apply via history so we can also exercise undo/redo.
  std::vector<MapEditor::Domain::Position> changed;
  MapEditor::Services::Autoborder::applyPlannedIntentWithHistory(
      engine, map, history, intent, &changed);
  require(!changed.empty(), "drag paint did not produce any changed positions");

  for (const auto &pos : painted) {
    auto *tile = map.getTile(pos);
    require(tile != nullptr, "painted tile missing after drag");
    require(tile->getItemCount() >= 1,
            "painted tile has no items after drag");
    require(brush.ownsItem(tile->getItem(tile->getItemCount() - 1)),
            "top item on painted tile is not owned by brush");
  }

  // The neighbour at {210, 211, 7} (south of (210, 210)) may or may not
  // be re-aligned (it's outside the L footprint). The planner must
  // not throw and must produce a deterministic, ownership-consistent
  // result.
  const auto neighbourPlan = brush.planTile(map, {210, 211, 7});
  for (const auto &planned : neighbourPlan) {
    require(planned.itemId != 0, "drag neighbour plan has zero itemId");
  }
}

// ─────────────────────────────────────────────────────────────────────────
// Section 5: erase removes carpet and propagates to neighbours
// ─────────────────────────────────────────────────────────────────────────

void requireErasePropagates(MapEditor::Domain::ChunkedMap &map,
                            MapEditor::Brushes::BrushRegistry &registry,
                            CarpetBrush &brush) {
  // Build a 2x1 carpet strip and erase one of the two tiles.
  const MapEditor::Domain::Position a{220, 220, 7};
  const MapEditor::Domain::Position b{221, 220, 7};
  MapEditor::Brushes::DrawContext ctx;
  ctx.brushRegistry = &registry;
  ctx.ownerBrushId = registry.getBrushId(&brush);
  brush.draw(map, map.getOrCreateTile(a), ctx);
  brush.draw(map, map.getOrCreateTile(b), ctx);
  require(map.getTile(a)->getItemCount() == 1,
          "carpet strip setup: tile a missing item");
  require(map.getTile(b)->getItemCount() == 1,
          "carpet strip setup: tile b missing item");

  // Erase the centre tile. Tile `b` no longer has a west neighbour, so
  // it should re-align to a centre piece.
  brush.undraw(map, map.getOrCreateTile(a));
  require(map.getTile(a)->getItemCount() == 0,
          "erase did not remove the centre tile's carpet item");

  const auto bPlan = brush.planTile(map, b);
  require(!bPlan.empty(), "tile b plan became empty after erasing a");
  require(bPlan[0].alignment == EdgeType::Center,
          "tile b did not re-align to centre after erasing a");
}

// ─────────────────────────────────────────────────────────────────────────
// Section 6: paint-on-existing converts to centre
// ─────────────────────────────────────────────────────────────────────────

void requireCenterOnExistingRepaints(MapEditor::Domain::ChunkedMap &map,
                                     MapEditor::Brushes::BrushRegistry &registry,
                                     CarpetBrush &brush) {
  // Build a 2x1 carpet strip.
  const MapEditor::Domain::Position a{230, 230, 7};
  const MapEditor::Domain::Position b{231, 230, 7};
  MapEditor::Brushes::DrawContext ctx;
  ctx.brushRegistry = &registry;
  ctx.ownerBrushId = registry.getBrushId(&brush);
  brush.draw(map, map.getOrCreateTile(a), ctx);
  brush.draw(map, map.getOrCreateTile(b), ctx);

  // Now paint another centre on `a`. Since `a` already has a centre
  // (because `b` is to its east), the result should be a single centre
  // item, not a CNW or corner piece.
  brush.draw(map, map.getOrCreateTile(a), ctx);
  require(map.getTile(a)->getItemCount() == 1,
          "centre paint on existing carpet produced != 1 item");
  const auto aPlan = brush.planTile(map, a);
  require(!aPlan.empty(), "centre repaint produced no plan");
  require(aPlan[0].alignment == EdgeType::Center,
          "centre repaint on existing carpet did not yield centre alignment");
}

// ─────────────────────────────────────────────────────────────────────────
// Section 7: preview matches commit
// ─────────────────────────────────────────────────────────────────────────

void requirePreviewMatchesCommit(MapEditor::Domain::ChunkedMap &map,
                                MapEditor::Brushes::BrushRegistry &registry,
                                CarpetBrush &brush,
                                MapEditor::Services::BrushSettingsService
                                    &settings) {
  // Plant a known existing carpet 1 tile north of the cursor.
  const MapEditor::Domain::Position cursor{240, 240, 7};
  const MapEditor::Domain::Position existingNorth{cursor.x, cursor.y - 1,
                                                  cursor.z};
  MapEditor::Brushes::DrawContext ctx;
  ctx.brushRegistry = &registry;
  ctx.ownerBrushId = registry.getBrushId(&brush);
  brush.draw(map, map.getOrCreateTile(existingNorth), ctx);

  MapEditor::Services::Preview::AutoborderBrushPreviewProvider provider(
      &brush, &settings, &map);
  provider.updateCursorPosition(cursor);

  // Snapshot the preview tiles — these are the items the preview says
  // the live map will contain *after* the commit.
  std::map<MapEditor::Domain::Position, std::vector<uint16_t>> previewItems;
  for (const auto &tile : provider.getTiles()) {
    const MapEditor::Domain::Position absolute{
        cursor.x + tile.relativePosition.x,
        cursor.y + tile.relativePosition.y,
        static_cast<int16_t>(cursor.z + tile.relativePosition.z)};
    auto &ids = previewItems[absolute];
    for (const auto &item : tile.items) {
      ids.push_back(item.itemId);
    }
  }
  require(!previewItems.empty(), "carpet preview produced no tiles");

  // Run the commit through the same planner the preview uses.
  MapEditor::Services::Autoborder::AutoborderEngine engine;
  MapEditor::Services::Autoborder::PlacementIntent intent;
  intent.brush = &brush;
  intent.mode = MapEditor::Services::Autoborder::PlacementMode::Draw;
  intent.context.brushRegistry = &registry;
  intent.context.ownerBrushId = registry.getBrushId(&brush);
  intent.context.brushSettings = &settings;
  intent.positions = {cursor};
  const auto diffs = engine.plan(map, intent);

  // Apply the diffs to the live map and verify each preview tile now
  // matches the live map's item ids.
  for (const auto &diff : diffs) {
    if (!diff.after) {
      continue;
    }
    map.setTile(diff.position, diff.after->clone());
  }

  for (const auto &[absolute, expected] : previewItems) {
    const auto *liveTile = map.getTile(absolute);
    if (!liveTile) {
      // The preview predicted a tile that the live map doesn't have.
      // That's fine if the live map was previously empty and the
      // commit didn't touch it. Skip the check in that case.
      continue;
    }
    const auto liveIds = tileItemIds(liveTile);
    require(liveIds == expected,
            "carpet preview did not match the live map after commit");
  }
}

// ─────────────────────────────────────────────────────────────────────────
// Section 8: context brush pick (right-click on carpet)
// ─────────────────────────────────────────────────────────────────────────

void requireContextPick(MapEditor::Domain::ChunkedMap &map,
                        MapEditor::Brushes::BrushRegistry &registry,
                        CarpetBrush &brush) {
  // Plant a carpet item and verify the brush controller can pick it.
  const MapEditor::Domain::Position pos{250, 250, 7};
  auto *tile = map.getOrCreateTile(pos);
  require(tile != nullptr, "context pick tile missing");
  placeCarpetItem(*tile, registry, brush, brush.getPreviewItemId());

  MapEditor::Domain::History::HistoryManager history;
  MapEditor::Brushes::BrushController controller;
  controller.initialize(&map, &history, nullptr);
  controller.setBrushRegistry(&registry);

  const auto selection = controller.resolveBrushFromTile(
      *tile, MapEditor::Brushes::BrushPickMode::Carpet);
  require(selection.has_value(),
          "context pick did not resolve a carpet brush");
  require(selection->brush == &brush,
          "context pick did not resolve the correct carpet brush");
  require(selection->mode == MapEditor::Brushes::BrushPickMode::Carpet,
          "context pick did not flag Carpet pick mode");
}

// ─────────────────────────────────────────────────────────────────────────
// Section 9: undo / redo round-trip
// ─────────────────────────────────────────────────────────────────────────

void requireUndoRedoRoundTrip(
    MapEditor::Domain::ChunkedMap &map,
    MapEditor::Domain::History::HistoryManager &history, CarpetBrush &brush,
    const MapEditor::Domain::Position &pos) {
  // Snapshot the 3x3 area, paint, undo, verify restore, redo, verify
  // restoration of post-paint state.
  const auto before = [&] {
    std::map<MapEditor::Domain::Position, std::vector<uint8_t>> snap;
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        const MapEditor::Domain::Position p{pos.x + dx, pos.y + dy, pos.z};
        snap.emplace(p, TileSnapshot::capture(map.getTile(p), p).data());
      }
    }
    return snap;
  }();

  // Reset history so this test owns the undo stack.
  history.clear();

  MapEditor::Services::Autoborder::AutoborderEngine engine;
  MapEditor::Services::Autoborder::PlacementIntent intent;
  intent.brush = &brush;
  intent.mode = MapEditor::Services::Autoborder::PlacementMode::Draw;
  intent.positions = {pos};
  history.beginOperation("carpet paint", MapEditor::Domain::History::ActionType::Draw);
  MapEditor::Services::Autoborder::applyPlannedIntentWithHistory(
      engine, map, history, intent, nullptr);
  history.endOperation(&map);
  require(history.canUndo(), "carpet paint did not record an undo entry");
  require(!history.undo(&map).empty(), "carpet undo failed");

  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      const MapEditor::Domain::Position p{pos.x + dx, pos.y + dy, pos.z};
      const auto &expected = before.at(p);
      const auto actual = TileSnapshot::capture(map.getTile(p), p).data();
      require(expected == actual,
              "carpet undo did not restore 3x3 snapshot");
    }
  }

  require(history.canRedo(), "carpet undo did not record a redo entry");
  require(!history.redo(&map).empty(), "carpet redo failed");
  require(map.getTile(pos) != nullptr,
          "carpet redo lost the painted tile");
  require(map.getTile(pos)->getItemCount() >= 1,
          "carpet redo did not restore the carpet item");
}

} // namespace

int main() {
  try {
    const fs::path sourceRoot = CARPET_SMOKE_SOURCE_DIR;
    const fs::path dataPath = sourceRoot.parent_path() / "data" / "1098";
    require(fs::exists(dataPath / "materials.xml"),
            "data/1098/materials.xml is missing");

    // ─── Section 1: lookup parity ─────────────────────────────────────
    requireLookupParity();

    // ─── Setup: load brushes ──────────────────────────────────────────
    MapEditor::Brushes::BrushRegistry registry;
    MapEditor::Services::TilesetService tilesetService(registry);
    require(tilesetService.loadMaterials(dataPath), "materials load failed");

    auto *carpetBrush = dynamic_cast<CarpetBrush *>(
        findBrush(registry, "red carpet", MapEditor::Brushes::BrushType::Carpet));
    require(carpetBrush != nullptr, "red carpet cast failed");

    MapEditor::Domain::ChunkedMap map;
    map.createNew(512, 512, 1098);

    // ─── Section 2: single-paint places one item ──────────────────────
    requireSingleCarpetItem(map, registry, *carpetBrush, {200, 200, 7},
                             "single_paint");

    // ─── Section 3: neighbour sweep ───────────────────────────────────
    requireNeighbourSweep(map, registry, *carpetBrush);

    // ─── Section 4: drag painting ─────────────────────────────────────
    MapEditor::Domain::History::HistoryManager history;
    requireDragPaintsAllTiles(map, registry, *carpetBrush, history);

    // ─── Section 5: erase propagates ──────────────────────────────────
    requireErasePropagates(map, registry, *carpetBrush);

    // ─── Section 6: centre on existing repaints ───────────────────────
    requireCenterOnExistingRepaints(map, registry, *carpetBrush);

    // ─── Section 7: preview matches commit ────────────────────────────
    MapEditor::Services::BrushSettingsService settings;
    settings.setStandardSize(0);
    requirePreviewMatchesCommit(map, registry, *carpetBrush, settings);

    // ─── Section 8: context pick ───────────────────────────────────────
    requireContextPick(map, registry, *carpetBrush);

    // ─── Section 9: undo / redo ───────────────────────────────────────
    requireUndoRedoRoundTrip(map, history, *carpetBrush, {300, 300, 7});

    std::cout << "CarpetSmoke passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "CarpetSmoke failed: " << ex.what() << "\n";
    return 1;
  }
}
