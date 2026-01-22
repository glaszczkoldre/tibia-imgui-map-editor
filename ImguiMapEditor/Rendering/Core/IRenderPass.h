#pragma once

#include "Domain/ChunkedMap.h"
#include "Rendering/Animation/AnimationTicks.h"
#include "Rendering/Camera/ViewCamera.h"
#include "Rendering/Frame/RenderState.h"
#include "Rendering/Visibility/VisibleBounds.h"
#include <glm/glm.hpp>

namespace MapEditor {
namespace Services {
struct ViewSettings;
}

namespace Rendering {

class SpriteBatch;
class IRenderPass;

/**
 * Context containing all necessary state for a render pass.
 * Replaces the long argument lists passed around in MapRenderer.
 */
struct RenderContext {
  const Domain::ChunkedMap &map;
  RenderState &state;
  const AnimationTicks &anim_ticks;
  const ViewCamera &camera;

  // Frame metrics
  int viewport_width;
  int viewport_height;

  // Shared resources
  SpriteBatch *sprite_batch;

  // Calculated for this frame
  glm::mat4 mvp_matrix;
  VisibleBounds visible_bounds;
  int current_floor;

  // Access to frame-local buffers
  std::vector<uint32_t> &missing_sprites_buffer;

  // View settings (optional, can be nullptr)
  const Services::ViewSettings *view_settings;
};

/**
 * Interface for a discrete rendering pass.
 * Allows decoupling specific rendering effects from the main MapRenderer.
 */
class IRenderPass {
public:
  virtual ~IRenderPass() = default;

  /**
   * Execute the rendering pass.
   * @param context Common rendering context with map, camera, and frame state.
   * @param context Common rendering context with map, camera, and frame state.
   */
  virtual void render(const RenderContext &context) = 0;

  /**
   * Set LOD mode to enable/disable simplified rendering.
   * Default implementation does nothing.
   */
  virtual void setLODMode(bool enabled) {}
};

} // namespace Rendering
} // namespace MapEditor
