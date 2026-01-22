#pragma once
#include <cstdint>
#include <glad/glad.h>
#include <vector>

namespace MapEditor {
namespace Rendering {

// Forward declaration
struct AtlasRegion;

/**
 * GPU Lookup Table for sprite ID → UV/layer resolution.
 *
 * Allows caching sprite IDs without baking async-dependent UV data.
 * The shader uses this LUT to resolve sprite IDs to texture coordinates
 * at draw time, eliminating cache invalidation on sprite loads.
 *
 * Uses SSBO (GL 4.3+) or TBO (GL 3.3+ fallback) for O(1) lookup in shader.
 *
 * ARCHITECTURE:
 * - CPU caches TileInstance with sprite_id (stable)
 * - GPU shader looks up UV/layer from this LUT (dynamic)
 * - Sprite loading updates LUT only, no VBO rebuilds
 */
class SpriteAtlasLUT {
public:
  /**
   * LUT entry for a single sprite.
   * Aligned to 32 bytes for GPU efficiency.
   */
  struct Entry {
    float u_min = 0.0f;
    float v_min = 0.0f;
    float u_max = 0.0f;
    float v_max = 0.0f;
    float layer = 0.0f;
    float valid = 0.0f;     // 1.0 if sprite is loaded, 0.0 for placeholder
    float _pad[2] = {0, 0}; // Alignment to 32 bytes
  };
  static_assert(sizeof(Entry) == 32,
                "Entry must be 32 bytes for GPU alignment");

  // Maximum sprite ID supported (Primary 0-1M, Secondary 1M-2M)
  // Covers primary + secondary client sprites
  static constexpr uint32_t MAX_SPRITES = 2000000;

  SpriteAtlasLUT();
  ~SpriteAtlasLUT();

  // Non-copyable
  SpriteAtlasLUT(const SpriteAtlasLUT &) = delete;
  SpriteAtlasLUT &operator=(const SpriteAtlasLUT &) = delete;

  // Moveable
  SpriteAtlasLUT(SpriteAtlasLUT &&other) noexcept;
  SpriteAtlasLUT &operator=(SpriteAtlasLUT &&other) noexcept;

  /**
   * Initialize GPU resources.
   * @return true if successful, false on GPU error
   */
  bool initialize();

  /**
   * Update a single sprite entry in the LUT.
   * Called when a sprite finishes loading.
   * @param sprite_id Sprite ID (index into LUT)
   * @param region Atlas region containing UV and layer data
   */
  void update(uint32_t sprite_id, const AtlasRegion &region);

  /**
   * Update multiple sprite entries in batch.
   * More efficient than multiple single updates.
   * @param sprite_id Starting sprite ID
   * @param regions Vector of AtlasRegion pointers
   */
  void updateBatch(
      const std::vector<std::pair<uint32_t, const AtlasRegion *>> &entries);

  /**
   * Mark a sprite as placeholder (not yet loaded).
   * Shader will use fallback texture.
   */
  void markPlaceholder(uint32_t sprite_id);

  /**
   * Bind LUT for shader access.
   * For SSBO: binds to binding point
   * For TBO: binds texture to slot
   * @param binding_point SSBO binding point or texture slot
   */
  void bind(uint32_t binding_point = 0) const;

  /**
   * Get GPU buffer ID for direct access.
   */
  GLuint getBufferId() const { return buffer_id_; }

  /**
   * Get texture ID (TBO path only).
   */
  GLuint getTextureId() const { return texture_id_; }

  /**
   * Check if using SSBO (GL 4.3+) or TBO fallback.
   */
  bool usesSSBO() const { return use_ssbo_; }

  /**
   * Check if LUT is initialized and ready.
   */
  bool isValid() const { return buffer_id_ != 0; }

  /**
   * Check if initialization was completed successfully.
   */
  bool isInitialized() const { return initialized_; }

  /**
   * Clear all entries (mark as invalid/placeholder).
   */
  void clear();

private:
  void uploadEntry(uint32_t sprite_id);
  void uploadRange(uint32_t start_id, uint32_t count);

  GLuint buffer_id_ = 0;
  GLuint texture_id_ = 0; // For TBO fallback only
  std::vector<Entry> cpu_data_;
  bool use_ssbo_ = false;
  bool initialized_ = false;

  // Dirty tracking for batch uploads
  uint32_t dirty_start_ = UINT32_MAX;
  uint32_t dirty_end_ = 0;
};

} // namespace Rendering
} // namespace MapEditor
