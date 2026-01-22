#pragma once
#include "Core/Config.h"
#include "Domain/ItemType.h"
#include "Domain/Tile.h"
#include "Rendering/Tile/TileColor.h"
#include "Services/ViewSettings.h"


namespace MapEditor {
namespace Rendering {

/**
 * Pure algorithm class for tile/item color filtering.
 * Matches RME_Readonly's MapDrawer::DrawTile() color logic.
 *
 * CONCERN: Color calculation algorithms only. No GPU calls, no rendering.
 */
class ColorFilter {
public:
  /**
   * Calculate ground color based on tile properties and view settings.
   *
   * RME Logic (from DrawTile):
   * - Blocking: g *= 2/3, b *= 2/3
   * - House (current): r /= 2
   * - House (other): r /= 2, g /= 2
   * - PZ: r /= 2, b /= 2
   * - PVP Zone: g = r/4, b *= 2/3
   * - No Logout: b /= 2
   * - No PvP: g /= 2
   *
   * @param tile The tile being rendered
   * @param settings View settings for toggle states
   * @param current_house_id Currently selected house (for highlighting)
   * @return TileColor with RGB values 0.0-1.0
   */
  static TileColor calculateGroundColor(const Domain::Tile &tile,
                                        const Services::ViewSettings &settings,
                                        uint32_t current_house_id = 0) {
    TileColor color(1.0f, 1.0f, 1.0f);

    // Blocking overlay (yellow-ish)
    if (settings.show_blocking && isBlocking(tile)) {
      color.g *= (2.0f / 3.0f);
      color.b *= (2.0f / 3.0f);
    }

    // House highlighting
    if (settings.show_houses && tile.isHouseTile()) {
      if (tile.getHouseId() == current_house_id) {
        color.r /= 2.0f; // Cyan for current house
      } else {
        color.r /= 2.0f; // Blue-ish for other houses
        color.g /= 2.0f;
      }
    } else if (settings.show_special_tiles && isPZ(tile)) {
      color.r /= 2.0f; // Green-ish for PZ
      color.b /= 2.0f;
    }

    // PVP Zone (orange-ish)
    if (settings.show_special_tiles && isPvpZone(tile)) {
      color.g = color.r / 4.0f;
      color.b *= (2.0f / 3.0f);
    }

    // No Logout (red-ish)
    if (settings.show_special_tiles && isNoLogout(tile)) {
      color.b /= 2.0f;
    }

    // No PVP (magenta-ish)
    if (settings.show_special_tiles && isNoPvp(tile)) {
      color.g /= 2.0f;
    }

    return color;
  }

  /**
   * Apply spawn radius magenta tint to ground color.
   * Called when a tile is within a spawn's radius.
   *
   * @param color Current tile color
   * @param in_spawn_radius Whether tile is within any spawn radius
   * @param settings View settings for toggle states
   * @return Modified TileColor with magenta tint if applicable
   */
  static TileColor
  applySpawnRadiusTint(TileColor color, bool in_spawn_radius,
                       const Services::ViewSettings &settings) {
    // Require BOTH show_spawns AND show_spawn_radius to be enabled
    if (!settings.show_spawns || !settings.show_spawn_radius ||
        !in_spawn_radius) {
      return color;
    }

    // Apply magenta tint (lerp towards magenta based on factor)
    float factor = Config::Colors::SPAWN_RADIUS_TINT_FACTOR;
    color.r = color.r * (1.0f - factor) +
              Config::Colors::SPAWN_RADIUS_TINT_R * factor;
    color.g = color.g * (1.0f - factor) +
              Config::Colors::SPAWN_RADIUS_TINT_G * factor;
    color.b = color.b * (1.0f - factor) +
              Config::Colors::SPAWN_RADIUS_TINT_B * factor;

    return color;
  }

  /**
   * Calculate item color based on item type and ground color.
   *
   * RME Logic (from DrawTile):
   * - Borders inherit ground color
   * - Non-borders reset to white (or apply house shader if enabled)
   *
   * @param item The item being rendered
   * @param groundColor Color calculated for ground
   * @param isBorder Whether item is a border
   * @return TileColor with RGB values 0.0-1.0
   */
  static TileColor calculateItemColor(const Domain::ItemType *itemType,
                                      const TileColor &groundColor,
                                      bool isBorder) {
    if (isBorder) {
      // Borders use same color as ground
      return groundColor;
    }
    // Non-borders reset to white
    return TileColor(1.0f, 1.0f, 1.0f);
  }

private:
  // Helper functions to check tile properties
  static bool isBlocking(const Domain::Tile &tile) {
    // Check if tile has blocking items
    if (tile.hasGround()) {
      const auto *ground = tile.getGround();
      if (ground && ground->getType() && ground->getType()->is_blocking) {
        return true;
      }
    }
    for (const auto &item : tile.getItems()) {
      if (item->getType() && item->getType()->is_blocking) {
        return true;
      }
    }
    return false;
  }

  static bool isPZ(const Domain::Tile &tile) {
    return tile.hasFlag(Domain::TileFlag::ProtectionZone);
  }

  static bool isPvpZone(const Domain::Tile &tile) {
    return tile.hasFlag(Domain::TileFlag::PvpZone);
  }

  static bool isNoLogout(const Domain::Tile &tile) {
    return tile.hasFlag(Domain::TileFlag::NoLogout);
  }

  static bool isNoPvp(const Domain::Tile &tile) {
    return tile.hasFlag(Domain::TileFlag::NoPvp);
  }

public:
  /**
   * Apply item highlight color tinting based on item count.
   *
   * RME Logic (from DrawTile):
   * Uses factor array [0.75, 0.6, 0.48, 0.40, 0.33] based on item count 1-5+.
   * Reduces R and G channels to show tile density as yellow→brown heat map.
   * Excludes tiles where topmost item is a border.
   *
   * @param color Current tile color
   * @param item_count Number of items on tile
   * @param topmost_is_border Whether the topmost item is a border (skip
   * highlighting)
   * @return Modified TileColor
   */
  static TileColor applyItemHighlight(TileColor color, size_t item_count,
                                      bool topmost_is_border) {
    if (item_count == 0 || topmost_is_border) {
      return color;
    }

    // RME factor array: more items = darker
    static constexpr float factor[5] = {0.75f, 0.6f, 0.48f, 0.40f, 0.33f};
    size_t idx = (item_count < 5 ? item_count : 5) - 1;

    color.r *= factor[idx];
    color.g *= factor[idx];

    return color;
  }
};

} // namespace Rendering
} // namespace MapEditor
