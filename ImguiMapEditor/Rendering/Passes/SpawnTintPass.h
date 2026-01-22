#pragma once
#include "Domain/ChunkedMap.h"
#include "Rendering/Backend/SpriteBatch.h"
#include "Rendering/Overlays/OverlayCollector.h"
#include "Rendering/Visibility/ChunkVisibilityManager.h"
#include "Services/ViewSettings.h"
#include <glm/glm.hpp>

namespace MapEditor {

namespace Services {
class SpriteManager;
}

namespace Rendering {

/**
 * Renders spawn-related overlays:
 * - Cyan tint for tiles within spawn radius
 * - Orange center indicator for spawn tiles
 *
 * Extracted from TileRenderer for better separation of concerns.
 */
class SpawnTintPass {
public:
  SpawnTintPass(SpriteBatch &batch, Services::SpriteManager &sprites);

  /**
   * Queue spawn radius overlay (cyan tint) if tile is within any spawn radius.
   * @param screen_x Screen X position
   * @param screen_y Screen Y position
   * @param size Tile size in pixels
   * @param alpha Base alpha for rendering
   * @param collector Overlay collector for spawn radius lookup
   * @param tile_x Tile world X coordinate
   * @param tile_y Tile world Y coordinate
   * @param tile_z Tile world Z coordinate
   * @param tile_has_spawn Whether this specific tile has a spawn
   */
  void queueRadiusOverlay(float screen_x, float screen_y, float size,
                          float alpha, const OverlayCollector *collector,
                          int tile_x, int tile_y, int tile_z,
                          bool tile_has_spawn);

  /**
   * Queue spawn center indicator (orange square) at spawn tile center.
   * @param screen_x Screen X position
   * @param screen_y Screen Y position
   * @param size Tile size in pixels
   * @param alpha Base alpha for rendering
   */
  void queueCenterIndicator(float screen_x, float screen_y, float size,
                            float alpha);

  /**
   * Render spawn overlays (tints and indicators) using collected data.
   * This decoupled rendering ensures overlays appear even when tile rendering
   * is batched/cached.
   * Uses World Coordinates (unscaled) to match the TerrainPass MVP matrix.
   */
  void renderFromCollector(const OverlayCollector &collector, int floor_z,
                           float floor_offset, float alpha);

  /**
   * Collect all spawns within the visible area (plus radius) into the overlay
   * collector. Used for calculating the spawn radius overlay tint.
   *
   * @param map The map to query
   * @param floor_z The current floor being rendered
   * @param bounds Visible bounds of the viewport
   * @param collector The overlay collector to populate
   * @param settings View settings (to check if spawns/radius are enabled)
   * @param buffer Reusable buffer for chunk pointers to avoid allocations
   */
  static void collectVisibleSpawns(const Domain::ChunkedMap &map, int floor_z,
                                   const VisibleBounds &bounds,
                                   OverlayCollector &collector,
                                   const Services::ViewSettings &settings,
                                   std::vector<Domain::Chunk *> &buffer);

private:
  SpriteBatch &sprite_batch_;
  Services::SpriteManager &sprite_manager_;
};

} // namespace Rendering
} // namespace MapEditor
