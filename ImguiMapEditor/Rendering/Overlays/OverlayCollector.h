#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <glm/vec2.hpp>
#include <string>
#include <unordered_map>
#include <vector>


namespace MapEditor::Domain {
class Tile;
class Item;
} // namespace MapEditor::Domain

namespace MapEditor::Rendering {

struct OverlayEntry {
  const Domain::Tile *tile;
  glm::vec2 screen_pos;
  const std::string *waypoint_name =
      nullptr; // Pointer to name string in Waypoint struct
};

/**
 * Spawn position + radius for spawn area tinting.
 */
struct SpawnRadiusEntry {
  int32_t center_x;
  int32_t center_y;
  int32_t floor;
  int32_t radius;
  int32_t creature_count = 0;
};

struct OverlayCollector {
  std::vector<OverlayEntry> tooltips;
  std::vector<OverlayEntry> spawns;
  std::vector<OverlayEntry> waypoints;

  // Spawn positions for radius tinting (populated during tile iteration)
  std::vector<SpawnRadiusEntry> spawn_radii;

  // OPTIMIZATION: Spatial Grid for O(1) spawn radius lookup
  // Using 64x64 cells (power of 2 for fast shifting)
  static constexpr int CELL_SHIFT = 6; // 2^6 = 64
  static constexpr int CELL_SIZE = 1 << CELL_SHIFT;

  // Map from (grid_x, grid_y) to list of indices in spawn_radii
  std::unordered_map<uint64_t, std::vector<size_t>> spatial_grid;

  void clear() {
    tooltips.clear();
    spawns.clear();
    waypoints.clear();
    spawn_radii.clear();
    spatial_grid.clear();
  }

  /**
   * Check if a tile position is within any spawn's radius.
   * OPTIMIZED: Uses spatial grid to check only nearby spawns.
   */
  bool isWithinAnySpawnRadius(int32_t x, int32_t y, int32_t z) const {
    // If no spawns, early exit
    if (spawn_radii.empty())
      return false;

    // Fast path: Lookup in spatial grid
    uint64_t key = getGridKey(x, y);
    auto it = spatial_grid.find(key);

    // If the cell is empty, no spawns are near enough to touch this tile
    if (it == spatial_grid.end())
      return false;

    // Check only candidates in this cell
    for (size_t idx : it->second) {
      const auto &spawn = spawn_radii[idx];
      if (spawn.floor != z)
        continue;

      int32_t dx = std::abs(x - spawn.center_x);
      int32_t dy = std::abs(y - spawn.center_y);

      // Use Chebyshev distance (square radius)
      if (dx <= spawn.radius && dy <= spawn.radius) {
        return true;
      }
    }
    return false;
  }

  /**
   * Register a spawn for radius tinting.
   * OPTIMIZED: Populates spatial grid.
   */
  void addSpawnRadius(int32_t x, int32_t y, int32_t z, int32_t radius,
                      int32_t creature_count = 0) {
    size_t idx = spawn_radii.size();
    spawn_radii.push_back({x, y, z, radius, creature_count});

    // Determine grid cells this spawn covers
    // We use bitwise shift which behaves as floor division for 2's complement
    // This correctly handles negative coordinates.
    int32_t min_grid_x = (x - radius) >> CELL_SHIFT;
    int32_t max_grid_x = (x + radius) >> CELL_SHIFT;
    int32_t min_grid_y = (y - radius) >> CELL_SHIFT;
    int32_t max_grid_y = (y + radius) >> CELL_SHIFT;

    for (int32_t gy = min_grid_y; gy <= max_grid_y; ++gy) {
      for (int32_t gx = min_grid_x; gx <= max_grid_x; ++gx) {
        uint64_t key = makeKey(gx, gy);
        spatial_grid[key].push_back(idx);
      }
    }
  }

  /**
   * Collect overlay entries from a tile (spawns, tooltips).
   * Encapsulates tooltip eligibility logic.
   *
   * @param tile The tile to analyze
   * @param screen_x Screen X position
   * @param screen_y Screen Y position
   * @param show_tooltips Whether tooltip collection is enabled
   */
  void collectFromTile(const Domain::Tile &tile, float screen_x, float screen_y,
                       bool show_tooltips);

    /**
     * Directly add a tooltip entry (used by optimized render path).
     */
    void addTooltip(const Domain::Tile* tile, float screen_x, float screen_y) {
        tooltips.push_back({tile, {screen_x, screen_y}, nullptr});
    }

    /**
     * Directly add a spawn entry (used by optimized render path).
     */
    void addSpawn(const Domain::Tile* tile, float screen_x, float screen_y) {
        spawns.push_back({tile, {screen_x, screen_y}, nullptr});
    }

    /**
     * Helper to check if an item needs a tooltip.
     * Used by TileRenderer/ItemRenderer to avoid redundant iteration.
     */
    static bool needsTooltip(const Domain::Item* item);

    /**
     * Helper to check if a ground tile needs a tooltip.
     */
    static bool needsTooltip(const Domain::Tile* tile);

private:
  static uint64_t getGridKey(int32_t x, int32_t y) {
    // x >> 6 is equivalent to floor(x / 64)
    return makeKey(x >> CELL_SHIFT, y >> CELL_SHIFT);
  }

  static uint64_t makeKey(int32_t gx, int32_t gy) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(gx)) << 32) |
           static_cast<uint64_t>(static_cast<uint32_t>(gy));
  }
};

} // namespace MapEditor::Rendering
