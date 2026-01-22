#pragma once
#include <cstdint>

namespace MapEditor {
namespace Rendering {

/**
 * Per-tile instance data for ID-based GPU caching.
 *
 * CRITICAL ARCHITECTURE PRINCIPLE:
 * This struct stores ONLY stable data that doesn't depend on async sprite
 * loading state. The sprite_id is resolved to UV coordinates in the GPU
 * shader via SpriteAtlasLUT, eliminating cache invalidation on sprite loads.
 *
 * GPU vertex layout:
 *   location 2: aRect (x, y, w, h)
 *   location 3: aSpriteId (uint32)
 *   location 4: aTint (r, g, b, a)
 *   location 5: aFlags (uint32)
 */
struct TileInstance {
  // Screen position (top-left corner)
  float x = 0.0f;
  float y = 0.0f;

  // Size in pixels
  float w = 0.0f;
  float h = 0.0f;

  // Sprite ID - resolved to UV in shader via SpriteAtlasLUT
  // This is the KEY change: we store the ID, not the resolved UV
  uint32_t sprite_id = 0;

  // Flags for shader-side decisions
  // Bit 0-7: Animation frame index
  // Bit 8: Is selected
  // Bit 9: Is highlighted
  // Bit 10-15: Reserved
  uint32_t flags = 0;

  // Color tint (lighting, selection highlight, etc.)
  float r = 1.0f;
  float g = 1.0f;
  float b = 1.0f;
  float a = 1.0f;

  // Padding to align to 48 bytes (GPU cache line friendly)
  float _pad[2] = {0.0f, 0.0f};

  // Flag bit positions
  static constexpr uint32_t FLAG_SELECTED = 1 << 8;
  static constexpr uint32_t FLAG_HIGHLIGHTED = 1 << 9;
  static constexpr uint32_t FLAG_ANIMATION_MASK = 0xFF;
};

static_assert(sizeof(TileInstance) == 48,
              "TileInstance must be 48 bytes for GPU alignment");

} // namespace Rendering
} // namespace MapEditor
