#pragma once
#include "Rendering/Animation/AnimationTicks.h"
#include "Rendering/Core/IRenderPass.h"

#include "Rendering/Core/RenderTarget.h"
#include "Rendering/Frame/RenderState.h"
#include "Rendering/Visibility/ChunkVisibilityManager.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace MapEditor {

namespace Domain {
class ChunkedMap;
}

namespace Services {
class SpriteManager;
}

namespace Rendering {

class TileRenderer;
class SpriteBatch;
class ChunkRenderingStrategy;

/**
 * Renders ghost (transparent) floors above/below the current floor.
 * Extracted from MapRenderer for better separation of concerns.
 */
class GhostFloorRenderer : public IRenderPass {
public:
  GhostFloorRenderer(TileRenderer &tile_renderer, SpriteBatch &batch,
                     ChunkVisibilityManager &visibility,
                     Services::SpriteManager &sprites);

  // Destructor required for unique_ptr of forward-declared class
  ~GhostFloorRenderer() override;

  /**
   * Render ghost floors based on current view settings in context.
   */
  void render(const RenderContext &context) override;

private:
  /**
   * Internal helper: Render a single ghost floor with parallax offset.
   */
  void renderSingleFloor(const Domain::ChunkedMap &map, RenderState &state,
                         const VisibleBounds &bounds, int current_floor,
                         int ghost_floor, float zoom, float alpha,
                         const glm::vec2 &viewport_size,
                         const glm::vec2 &camera_pos, const glm::mat4 &mvp,
                         const AnimationTicks &anim_ticks,
                         std::vector<uint32_t> &missing_sprites);

private:
  TileRenderer &tile_renderer_;
  SpriteBatch &sprite_batch_;
  ChunkVisibilityManager &chunk_visibility_;
  Services::SpriteManager &sprite_manager_;
  std::unique_ptr<ChunkRenderingStrategy> chunk_strategy_;

  static constexpr float TILE_SIZE = 32.0f;
};

} // namespace Rendering
} // namespace MapEditor
