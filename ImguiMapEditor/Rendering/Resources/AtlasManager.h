#pragma once
#include "Rendering/Resources/TextureAtlas.h"
#include <cstdint>
#include <deque>
#include <functional>
#include <unordered_map>
#include <vector>

namespace MapEditor {
namespace Rendering {

/**
 * Manages a single texture array atlas and provides sprite → region lookup.
 *
 * Uses a single GL_TEXTURE_2D_ARRAY that expands automatically as needed.
 * Sprites are stored by their sprite_id for O(1) lookup during rendering.
 */
class AtlasManager {
public:
  // Max sprite ID for O(1) direct lookup (covers most Tibia sprites)
  static constexpr uint32_t DIRECT_LOOKUP_SIZE = 100000;

  AtlasManager() = default;
  ~AtlasManager() = default;

  // Non-copyable
  AtlasManager(const AtlasManager &) = delete;
  AtlasManager &operator=(const AtlasManager &) = delete;

  // Moveable
  AtlasManager(AtlasManager &&) = default;
  AtlasManager &operator=(AtlasManager &&) = default;

  /**
   * Add a sprite to the atlas.
   * @param sprite_id Unique sprite ID for later lookup
   * @param rgba_data 32x32x4 bytes of RGBA pixel data
   * @return Pointer to the region info (owned by this manager), or nullptr on
   * failure
   */
  const AtlasRegion *addSprite(uint32_t sprite_id, const uint8_t *rgba_data);

  /**
   * Get the atlas region for an already-added sprite.
   * Uses O(1) array lookup for sprite_id < DIRECT_LOOKUP_SIZE, hash otherwise.
   * @param sprite_id Sprite ID
   * @return Pointer to region, or nullptr if sprite not found
   */
  inline const AtlasRegion *getRegion(uint32_t sprite_id) const {
    // Fast path: direct array lookup for common sprites
    if (sprite_id < DIRECT_LOOKUP_SIZE) {
      return direct_lookup_[sprite_id]; // O(1)
    }
    // Slow path: hash lookup for rare high-ID sprites
    auto it = sprite_regions_.find(sprite_id);
    return it != sprite_regions_.end() ? it->second : nullptr;
  }

  /**
   * Check if a sprite has been added.
   */
  bool hasSprite(uint32_t sprite_id) const;

  /**
   * Bind the texture array to a texture slot.
   * @param slot Texture unit to bind to
   */
  void bind(uint32_t slot = 0) const;

  /**
   * Get the number of layers in the texture array.
   */
  size_t getLayerCount() const;

  /**
   * Get a region containing a single white pixel.
   */
  const AtlasRegion *getWhitePixel();

  static constexpr uint32_t WHITE_PIXEL_ID = 0xFFFFFFFF;
  static constexpr uint32_t INVALID_PLACEHOLDER_ID = 0xFFFFFFFE;

  /**
   * Get the AtlasRegion for the "invalid item" placeholder sprite.
   * This is a red 32x32 square used for items with no valid ItemType.
   * Created lazily on first access.
   */
  const AtlasRegion *getInvalidItemPlaceholder();

  /**
   * Get total number of sprites.
   */
  size_t getTotalSpriteCount() const { return sprite_regions_.size(); }

  /**
   * Clear atlas and sprite mappings.
   */
  void clear();

  /**
   * Iterate over all sprites in the atlas.
   * @param callback Function to call for each sprite (id, region)
   */
  void forEachSprite(
      std::function<void(uint32_t, const AtlasRegion &)> callback) const;

  /**
   * Add a sprite from a PBO (Pixel Buffer Object).
   */
  const AtlasRegion *addSpriteFromPBO(uint32_t sprite_id,
                                      const uint8_t *pbo_offset);

  /**
   * Get texture ID for direct access.
   */
  GLuint getTextureId() const;

  /**
   * Get atlas version. Incremented when texture object changes.
   * Used by SpriteBatch to detect stale bindings.
   */
  uint64_t getAtlasVersion() const { return atlas_.getVersion(); }

private:
  bool ensureInitialized();

  TextureAtlas atlas_;

  // Use std::deque for stable storage of AtlasRegions.
  // This ensures pointers to elements remain valid even when new elements are added (no reallocation copy).
  std::deque<AtlasRegion> region_storage_;

  // Map stores pointers to elements in region_storage_
  std::unordered_map<uint32_t, AtlasRegion *> sprite_regions_;

  // O(1) direct lookup for sprite IDs < DIRECT_LOOKUP_SIZE
  std::vector<const AtlasRegion *> direct_lookup_{DIRECT_LOOKUP_SIZE, nullptr};
};

} // namespace Rendering
} // namespace MapEditor
