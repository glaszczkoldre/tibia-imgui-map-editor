#pragma once
#include "Outfit.h"
#include "Position.h"
#include <cstdint>
#include <string>


namespace MapEditor {
namespace Domain {

/**
 * Represents a creature placed on a tile.
 * Stored directly on tiles (like RME), not as offsets from spawn.
 */
struct Creature {
  std::string name;
  int32_t spawn_time = 60; // Respawn time in seconds
  int32_t direction = 2;   // 0=North, 1=East, 2=South, 3=West (default: South)
  Outfit outfit;

  // Position is stored for standalone creature operations (e.g., brush
  // placement) When on a tile, the tile's position is authoritative
  int32_t x = 0, y = 0, z = 0;

  // Selection state for visual feedback during rendering
  // mutable because selection doesn't affect logical constness
  mutable bool selected = false;

  void setPosition(const Position &pos) {
    x = pos.x;
    y = pos.y;
    z = pos.z;
  }

  void setOutfit(const Outfit &o) { outfit = o; }
  void setName(const std::string &n) { name = n; }

  // Selection accessors (like RME's Creature class)
  // const because selection is visual-only state
  bool isSelected() const { return selected; }
  void select() const { selected = true; }
  void deselect() const { selected = false; }

  Creature() = default;

  explicit Creature(const std::string &creature_name, int32_t time = 60,
                    int32_t dir = 2)
      : name(creature_name), spawn_time(time), direction(dir) {}
};

} // namespace Domain
} // namespace MapEditor
