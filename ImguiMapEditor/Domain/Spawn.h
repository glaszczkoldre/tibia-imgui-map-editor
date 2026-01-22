#pragma once
#include "Position.h"
#include <cstdint>
#include <string>


namespace MapEditor {
namespace Domain {

/**
 * Represents a spawn point on the map.
 *
 * The spawn stores only the center position and radius.
 * Creatures are stored directly on tiles (per-tile storage like RME).
 * When saving, the SpawnXmlWriter scans tiles within radius to find creatures.
 */
struct Spawn {
  Position position;
  int32_t radius;

  // Selection state for visual feedback during rendering
  // mutable because selection doesn't affect logical constness
  mutable bool selected = false;

  Spawn(Position pos = Position(), int32_t r = 0) : position(pos), radius(r) {}

  // Selection accessors (like RME's Spawn class)
  // const because selection is visual-only state
  bool isSelected() const { return selected; }
  void select() const { selected = true; }
  void deselect() const { selected = false; }
};

} // namespace Domain
} // namespace MapEditor
