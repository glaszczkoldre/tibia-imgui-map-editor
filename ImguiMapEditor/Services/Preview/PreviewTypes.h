#pragma once
#include "Domain/Position.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace MapEditor::Services::Preview {

/**
 * Style for rendering preview items.
 */
enum class PreviewStyle {
  Ghost,   // Semi-transparent blue tint (default)
  Outline, // Colored outline only
  Tinted   // Custom color tint
};

/**
 * Single item within a preview tile.
 * Lightweight POD structure for efficient storage.
 */
struct PreviewItemData {
  uint32_t itemId = 0;          // Server item ID
  uint16_t subtype = 0;         // Stack count or fluid type
  float elevationOffset = 0.0f; // Accumulated elevation for stacked items

  PreviewItemData() = default;
  PreviewItemData(uint32_t id, uint16_t sub = 0, float elev = 0.0f)
      : itemId(id), subtype(sub), elevationOffset(elev) {}
};

/**
 * A tile containing preview items, creature, or spawn.
 * Position is relative to anchor (0,0,0 = cursor position).
 */
struct PreviewTileData {
  Domain::Position relativePosition{0, 0, 0};
  std::vector<PreviewItemData> items;

  // Creature name for creature drag preview (outfit looked up from
  // CreatureType)
  std::optional<std::string> creature_name;

  // Spawn indicator for spawn drag preview
  bool has_spawn = false;
  int spawn_radius = 0; // Radius for drawing spawn border rectangle

  // Zone brush color overlay (for Flag, Eraser, House, Waypoint)
  // ARGB format, 0 means no zone overlay
  uint32_t zone_color = 0;

  PreviewTileData() = default;
  explicit PreviewTileData(const Domain::Position &pos)
      : relativePosition(pos) {}
  PreviewTileData(int x, int y, int z = 0)
      : relativePosition(x, y, static_cast<int16_t>(z)) {}

  bool empty() const {
    return items.empty() && !creature_name.has_value() && !has_spawn &&
           zone_color == 0;
  }

  void addItem(uint32_t itemId, uint16_t subtype = 0, float elevation = 0.0f) {
    items.emplace_back(itemId, subtype, elevation);
  }
};

/**
 * Bounding box for preview culling.
 * All coordinates are relative to anchor.
 */
struct PreviewBounds {
  int minX = 0, maxX = 0;
  int minY = 0, maxY = 0;
  int minZ = 0, maxZ = 0;

  PreviewBounds() = default;

  bool contains(int x, int y, int z) const {
    return x >= minX && x <= maxX && y >= minY && y <= maxY && z >= minZ &&
           z <= maxZ;
  }

  void expand(int x, int y, int z) {
    if (x < minX)
      minX = x;
    if (x > maxX)
      maxX = x;
    if (y < minY)
      minY = y;
    if (y > maxY)
      maxY = y;
    if (z < minZ)
      minZ = z;
    if (z > maxZ)
      maxZ = z;
  }

  void expand(const Domain::Position &pos) { expand(pos.x, pos.y, pos.z); }

  int width() const { return maxX - minX + 1; }
  int height() const { return maxY - minY + 1; }
  int depth() const { return maxZ - minZ + 1; }

  static PreviewBounds fromSingle() {
    return PreviewBounds(); // Default constructor gives all zeros
  }
};

} // namespace MapEditor::Services::Preview
