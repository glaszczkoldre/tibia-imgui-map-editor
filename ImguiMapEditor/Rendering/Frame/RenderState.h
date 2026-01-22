#pragma once
#include "Rendering/Light/LightManager.h"
#include "Rendering/Overlays/OverlayCollector.h"
#include "Rendering/Tile/ChunkSpriteCache.h"
#include <memory>

namespace MapEditor {

namespace Services {
class ClientDataService;
}

namespace Rendering {

/**
 * Per-session rendering state.
 *
 * Extracted from MapRenderer to enable independent render caches per
 * EditorSession. Each open map tab owns its own RenderState, preventing cache
 * conflicts when switching between maps that share the same coordinate space.
 *
 * USAGE:
 *   MapRenderer::render(map, *session.getRenderState(), width, height);
 */
class RenderState {
public:
  explicit RenderState(Services::ClientDataService *client_data = nullptr);
  ~RenderState();

  // Non-copyable, movable
  RenderState(const RenderState &) = delete;
  RenderState &operator=(const RenderState &) = delete;
  RenderState(RenderState &&) = default;
  RenderState &operator=(RenderState &&) = default;

  // === Per-session chunk cache ===
  ChunkSpriteCache chunk_cache;

  // === Per-session lighting ===
  std::unique_ptr<LightManager> light_manager;

  // === Per-session overlay data (collected during render) ===
  OverlayCollector overlay_collector;

  // === State tracking for cache invalidation ===
  float last_zoom = 0.0f;
  uint8_t last_ambient_light = 255;

  // === Invalidation methods ===

  /**
   * Invalidate all cached data.
   * Called on map switch, major settings change.
   */
  void invalidateAll();

  /**
   * Invalidate a specific chunk.
   * Called when a tile in this chunk is modified.
   */
  void invalidateChunk(int32_t chunk_x, int32_t chunk_y, int8_t floor);

  /**
   * Invalidate light at a specific position.
   */
  void invalidateLight(int32_t x, int32_t y);
};

} // namespace Rendering
} // namespace MapEditor
