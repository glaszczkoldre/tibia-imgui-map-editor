#pragma once

/**
 * @file AlignedBrushHelpers.h
 * @brief Shared utilities for "aligned tile" brushes (carpet, table, wall).
 *
 * Two brushes (CarpetBrush, TableBrush — and to a lesser extent WallBrush)
 * share the same mechanical pattern: each owns a small set of items indexed
 * by `EdgeType`/`TableAlign`, paints a center piece, and then re-aligns the
 * 3x3 neighbourhood so that border/corner pieces appear around the painted
 * tile based on the owned-neighbour bitmask.
 *
 * The functions in this header are deliberately *free functions* (not
 * member methods) so that the carpet and table brushes can share them
 * without an artificial base class, and so the autoborder engine and the
 * preview provider can both consume the same planner. See the
 * `docs/carpet-parity-report.md` file for the motivation.
 */

#include "Brushes/Behaviors/WeightedSelection.h"
#include "Brushes/Core/IBrush.h"
#include "Brushes/Enums/BrushEnums.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Tile.h"
#include "Domain/Item.h"
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace MapEditor::Brushes::Helpers {

/**
 * Neighbor offsets used by every alignment brush. Order matches the bit
 * positions in `TileNeighbor` (NW, N, NE, W, E, SW, S, SE).
 */
inline constexpr std::array<std::tuple<int, int, TileNeighbor>, 8>
kAlignedNeighborOffsets{{
    {-1, -1, TileNeighbor::Northwest},
    {0, -1, TileNeighbor::North},
    {1, -1, TileNeighbor::Northeast},
    {-1, 0, TileNeighbor::West},
    {1, 0, TileNeighbor::East},
    {-1, 1, TileNeighbor::Southwest},
    {0, 1, TileNeighbor::South},
    {1, 1, TileNeighbor::Southeast},
}};

/**
 * Build a `TileNeighbor` bitmask of all 8 neighbours that contain an item
 * "owned" by the given brush. The center tile itself is not consulted.
 */
[[nodiscard]] inline TileNeighbor
computeOwnedNeighborMask(const Domain::ChunkedMap &map,
                         const IBrush &brush, const Domain::Position &pos) {
  TileNeighbor mask = TileNeighbor::None;
  for (const auto &[dx, dy, bit] : kAlignedNeighborOffsets) {
    const auto *neighbor = map.getTile(pos.x + dx, pos.y + dy, pos.z);
    if (!neighbor) {
      continue;
    }
    bool owned = false;
    for (const auto &item : neighbor->getItems()) {
      if (item && brush.ownsItem(item.get())) {
        owned = true;
        break;
      }
    }
    if (owned) {
      mask |= bit;
    }
  }
  return mask;
}

/**
 * Collect pointers to every item in the tile that is owned by the given
 * brush. Returned pointers are raw, non-owning; the caller must not outlive
 * the tile.
 */
[[nodiscard]] inline std::vector<Domain::Item *>
collectOwnedItems(Domain::Tile &tile, const IBrush &brush) {
  std::vector<Domain::Item *> result;
  result.reserve(tile.getItemCount());
  for (const auto &item : tile.getItems()) {
    if (item && brush.ownsItem(item.get())) {
      result.push_back(item.get());
    }
  }
  return result;
}

/**
 * Pick a weighted-random item id from a list of (id, weight) pairs.
 * Treats a 0 weight as weight 1 (matches the existing CarpetBrush/TableBrush
 * behaviour). Returns 0 if the list is empty.
 */
[[nodiscard]] inline uint16_t
selectWeightedItem(const std::vector<std::pair<uint16_t, uint32_t>> &items) {
  if (items.empty()) {
    return 0;
  }
  std::vector<uint32_t> weights;
  weights.reserve(items.size());
  for (const auto &[_, w] : items) {
    weights.push_back(w == 0 ? 1u : w);
  }
  const auto index = WeightedSelection::select(weights);
  return index ? items[*index].first : items.front().first;
}

} // namespace MapEditor::Brushes::Helpers
