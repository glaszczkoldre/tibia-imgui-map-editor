#pragma once

#include "Rendering/Core/IRenderPass.h"
#include <memory>

namespace MapEditor {

namespace Services {
class SpriteManager;
}

namespace Rendering {

class TileRenderer;
class ChunkVisibilityManager;
class SpriteBatch;
class ShadeRenderer;
class SpawnTintPass;
class ChunkRenderingStrategy;
class FrameDataCollector;

/**
 * Main rendering pass for the map terrain.
 * Renders visible floors, tiles, objects, and creatures using the "Painter's
 * Algorithm" (Back-to-front, bottom-to-top).
 */
class TerrainPass : public IRenderPass {
public:
  TerrainPass(TileRenderer &tile_renderer, ChunkVisibilityManager &visibility,
              SpriteBatch &batch, Services::SpriteManager &sprite_manager,
              FrameDataCollector &frame_data_collector);
  ~TerrainPass() override;

  void render(const RenderContext &context) override;

  /**
   * Enable/disable LOD mode to force simplified rendering (e.g. forced
   * batching).
   */
  void setLODMode(bool enabled) override;

private:
  bool is_lod_active_ = false;

  bool was_lod_active_ = false;
  int last_floor_ = -100;
  bool was_show_all_floors_ = false;

  TileRenderer &tile_renderer_;
  ChunkVisibilityManager &chunk_visibility_;
  SpriteBatch &sprite_batch_;
  Services::SpriteManager &sprite_manager_;
  FrameDataCollector &frame_data_collector_;

  // Components owned by this pass
  std::unique_ptr<ShadeRenderer> shade_renderer_;
  std::unique_ptr<SpawnTintPass> spawn_renderer_;
  std::unique_ptr<ChunkRenderingStrategy> chunk_strategy_;

  // Helper for rendering a single floor
  void renderMainFloor(const RenderContext &context, int floor);
};

} // namespace Rendering
} // namespace MapEditor
