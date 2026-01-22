#pragma once
#include "Domain/ChunkedMap.h"
#include "Domain/Tile.h"
#include "Rendering/Animation/AnimationTicks.h"
#include "Rendering/Backend/TileInstance.h"
#include "Rendering/Frame/RenderState.h"
#include "Rendering/Tile/ChunkSpriteCache.h"
#include "Rendering/Visibility/ChunkVisibilityManager.h"
#include <cstdint>
#include <vector>

namespace MapEditor {

namespace Services {
class SpriteManager;
}

namespace Rendering {
class TileRenderer;
class SpriteBatch;
class AtlasManager;

/**
 * Strategy pattern for rendering a map chunk.
 * Encapsulates the logic for traversing a chunk and generating rendering
 * commands. Supports different rendering paths (Cached/VBO vs
 * Dynamic/Immediate) based on zoom level and interactivity requirements.
 */
class ChunkRenderingStrategy {
public:
  /**
   * Rendering context passed down through the traversal.
   * Holds references to shared resources and per-frame state.
   */
  struct Context {
    RenderState &state;
    const AnimationTicks &anim_ticks;
    std::vector<uint32_t> &missing_sprites;
    int &tiles_rendered;
    int floor_z;
    float floor_offset;
    int32_t chunk_wx;
    int32_t chunk_wy;
    float chunk_screen_x;
    float chunk_screen_y;

    Context(RenderState &s, const AnimationTicks &ticks,
            std::vector<uint32_t> &missing, int &rendered, int z, float offset,
            const Domain::Chunk &chunk);
  };

  ChunkRenderingStrategy(TileRenderer &tile_renderer, SpriteBatch &batch,
                         Services::SpriteManager &sprites);

  /**
   * Render a chunk using the cached VBO path (Zoomed Out / Static).
   * Generates cache if invalid.
   * Assumes SpriteBatch is in Tile mode (beginTileBatch called).
   */
  void renderCached(const Domain::Chunk &chunk, const Context &ctx);

  /**
   * Render a previously generated cached chunk VBO.
   * Assumes SpriteBatch is in Tile mode.
   */
  void renderFromCache(ChunkSpriteCache::CachedChunk *cached);

  /**
   * Render a chunk using the dynamic immediate path (Zoomed In / Animated).
   * Iterates tiles and queues sprites to the batch.
   * Assumes SpriteBatch is in Sprite mode (begin called).
   */
  void renderDynamic(const Domain::Chunk &chunk, const Context &ctx);

  /**
   * DEPRECATED: Render only the edge of a chunk.
   * Replaced by uniform rendering strategy.
   */
  void renderEdge(const Domain::Chunk &chunk, const Context &ctx,
                  const VisibleBounds &bounds);

private:
  void generateCachedChunk(const Domain::Chunk &chunk, const Context &ctx,
                           ChunkSpriteCache::CachedChunk *cached);

  TileRenderer &tile_renderer_;
  SpriteBatch &sprite_batch_;
  Services::SpriteManager &sprite_manager_;
};

} // namespace Rendering
} // namespace MapEditor
