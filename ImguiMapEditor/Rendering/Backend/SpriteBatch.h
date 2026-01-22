#pragma once
#include "Core/Config.h"
#include "Rendering/Backend/MultiDrawIndirectRenderer.h"
#include "Rendering/Backend/RingBuffer.h"
#include "Rendering/Backend/TileInstance.h"
#include "Rendering/Core/GLHandle.h"
#include "Rendering/Core/Shader.h"
#include "Rendering/Resources/AtlasManager.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace MapEditor {

namespace Services {
class SpriteManager; // Forward declaration for CPU fallback
}

namespace Rendering {

// Forward declaration for ID-based rendering
class SpriteAtlasLUT;

/**
 * Per-sprite instance data for instanced rendering.
 * Each sprite needs position, size, UV coordinates, color tint, and atlas
 * layer.
 *
 * Layout matches vertex attributes:
 *   location 2: aRect (x, y, w, h)
 *   location 3: aUV (u_min, v_min, u_max, v_max)
 *   location 4: aTint (r, g, b, a)
 *   location 5: aLayer (atlas_layer as float for compatibility)
 */
struct SpriteInstance {
  float x, y;                // Screen position (top-left)
  float w, h;                // Size in pixels
  float u_min, v_min;        // UV top-left in atlas
  float u_max, v_max;        // UV bottom-right in atlas
  float r, g, b, a;          // Color tint/alpha (1,1,1,1 = no tint)
  float atlas_layer;         // Which layer in the texture array
  float _pad1, _pad2, _pad3; // Padding to 64 bytes for alignment
};

enum class BatchMode {
  Sprites, // Default mode: Dynamic sprites (UV-based)
  Tiles    // Cached mode: Static tiles (ID-based, VBOs)
};

/**
 * High-performance batched sprite renderer using instanced drawing.
 *
 * OPTIMIZATIONS OVER NAIVE APPROACH:
 * 1. Triple-buffered persistent-mapped buffer (no CPU-GPU sync stalls)
 * 2. Flat vector (cache-friendly)
 * 3. Single draw call per atlas texture
 * 4. Fence synchronization for async GPU pipeline
 * 5. DIRECT DATA WRITING: Removed intermediate "PendingSprite" wrapper and
 * sorting since texture arrays handle atlas selection in shader.
 *
 * Usage:
 *   batch.begin(projection);
 *   for each visible tile:
 *       for each sprite in tile:
 *           batch.draw(x, y, w, h, atlasRegion);
 *   batch.end(atlasManager);  // Issues draw calls
 *
 * Performance:
 *   140+ FPS with persistent mapping and direct vector writing.
 */
class SpriteBatch {
public:
  // 64MB buffer = ~1.4M sprites (48 bytes each with tint) - enough for extreme
  // zoomed-out views
  static constexpr size_t MAX_SPRITES_PER_BATCH =
      Config::Performance::MAX_SPRITES_PER_BATCH;
  static constexpr size_t MAX_ATLASES = Config::Performance::MAX_ATLASES;

  SpriteBatch();
  ~SpriteBatch();

  // Non-copyable, but movable
  SpriteBatch(const SpriteBatch &) = delete;
  SpriteBatch &operator=(const SpriteBatch &) = delete;
  SpriteBatch(SpriteBatch &&other) noexcept;
  SpriteBatch &operator=(SpriteBatch &&other) noexcept;

  /**
   * Initialize GPU resources (shader, VAO, VBOs, RingBuffer).
   * Must be called once before use.
   * @return true if successful
   */
  bool initialize();

  /**
   * Begin a new sprite batch (Dynamic/UV mode). Clears any pending sprites.
   * @param projection The orthographic projection matrix
   */
  void begin(const glm::mat4 &projection);

  /**
   * Begin a new tile batch (Cached/ID mode).
   * Sets up the tile shader and binds shared resources (Atlas, LUT) once.
   * Subsequent calls to drawTileInstances() will assume this state.
   *
   * @param projection The orthographic projection matrix
   * @param atlas_manager Atlas manager to bind to texture units
   * @param lut Sprite lookup table for ID → UV resolution
   */
  void beginTileBatch(const glm::mat4 &projection,
                      const AtlasManager &atlas_manager, SpriteAtlasLUT &lut);

  /**
   * Queue a sprite for rendering with default white tint.
   * @param x Screen X position (top-left)
   * @param y Screen Y position (top-left)
   * @param w Width in pixels
   * @param h Height in pixels
   * @param region Atlas region containing UV coordinates
   */
  void draw(float x, float y, float w, float h, const AtlasRegion &region);

  /**
   * Queue a sprite with color tint for ghost/overlay effects.
   * @param x Screen X position (top-left)
   * @param y Screen Y position (top-left)
   * @param w Width in pixels
   * @param h Height in pixels
   * @param region Atlas region containing UV coordinates
   * @param r Red tint (0.0-1.0)
   * @param g Green tint (0.0-1.0)
   * @param b Blue tint (0.0-1.0)
   * @param a Alpha (0.0-1.0, 1.0 = opaque)
   */
  void draw(float x, float y, float w, float h, const AtlasRegion &region,
            float r, float g, float b, float a);

  /**
   * Render a VBO containing TileInstance data (ID-based format).
   * Uses tile_batch shader with GPU-side sprite ID resolution.
   *
   * @param vbo The VBO containing TileInstance data
   * @param count Number of instances to draw
   * @param atlas_manager Atlas manager for texture binding
   * @param lut Sprite lookup table for ID → UV resolution
   */
  void drawTileInstances(GLuint vbo, size_t count,
                         const AtlasManager &atlas_manager,
                         SpriteAtlasLUT &lut);

  /**
   * Render all queued sprites.
   * Uploads to persistent-mapped buffer, issues draw calls.
   *
   * @param atlas_manager The atlas manager to bind textures from
   */
  void end(const AtlasManager &atlas_manager);

  /**
   * End the current tile batch.
   * Restores default state (unbinds tile shader/VAO).
   */
  void endTileBatch();

  /**
   * Set global tint color for all subsequent draws in this batch.
   * Useful for applying global alpha or color overlays (e.g. ghost floors).
   * Resets to (1,1,1,1) in begin().
   */
  void setGlobalTint(float r, float g, float b, float a);

  /**
   * Pre-allocate capacity for the pending sprites vector.
   * Use this if you know the approximate number of sprites to be drawn
   * to avoid dynamic reallocation overhead during the frame.
   * @param capacity Number of sprites to reserve space for
   */
  void ensureCapacity(size_t capacity);

  /**
   * Get number of draw calls issued in last end() call.
   */
  int getDrawCallCount() const { return draw_call_count_; }

  /**
   * Get number of sprites rendered in last end() call.
   */
  int getSpriteCount() const { return sprite_count_; }

private:
  void flush(const AtlasManager &atlas_manager);

  /**
   * Lazy initialization of tile shader and VAO.
   * Called by both beginTileBatch() and drawTileInstances().
   * @return true if initialization succeeded
   */
  bool ensureTileShaderInitialized();

  std::unique_ptr<Shader> shader_;      // Legacy UV-based shader
  std::unique_ptr<Shader> tile_shader_; // New ID-based shader

  // OpenGL resources (RAII managed)
  DeferredVAOHandle vao_;
  DeferredVBOHandle quad_vbo_; // Unit quad vertices (static)
  DeferredVBOHandle quad_ebo_; // Unit quad indices (static)

  // Triple-buffered persistent-mapped instance buffer
  RingBuffer ring_buffer_;

  // Flat vector of sprites (cache-friendly!)
  // Optimized: Stores SpriteInstance directly, avoiding intermediate copy
  std::vector<SpriteInstance> pending_sprites_;

  glm::mat4 projection_{1.0f};
  bool in_batch_ = false;
  BatchMode mode_ = BatchMode::Sprites;

  // Multi-draw indirect renderer (GL 4.3+)
  MultiDrawIndirectRenderer mdi_renderer_;
  bool use_mdi_ = false; // Set to true if GL 4.3+ available

  // State caching to avoid redundant GL calls
  GLuint last_bound_vao_ = 0;

  // VAO for TileInstance rendering (different vertex layout)
  DeferredVAOHandle tile_vao_;
  bool tile_shader_initialized_ = false;

  // Stats
  int draw_call_count_ = 0;
  int sprite_count_ = 0;
};

} // namespace Rendering
} // namespace MapEditor
