#pragma once
#include <cstdint>

namespace MapEditor {
namespace Utils {

/**
 * Utility for RGBA pixel blending operations.
 * Used by compositing functions to blend sprite tiles onto canvases.
 */
namespace ImageBlending {

/**
 * Blend a source RGBA pixel onto a destination RGBA pixel.
 * Uses standard alpha compositing (src over dst).
 *
 * @param src Pointer to source RGBA pixel (4 bytes)
 * @param dst Pointer to destination RGBA pixel (4 bytes, modified in-place)
 */
inline void blendPixel(const uint8_t *src, uint8_t *dst) {
  const uint8_t src_a = src[3];

  // Fully transparent - no change
  if (src_a == 0) {
    return;
  }

  // Fully opaque - direct copy
  if (src_a == 255) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = 255;
    return;
  }

  // Alpha blend
  const float alpha = src_a / 255.0f;
  const float inv_alpha = 1.0f - alpha;
  dst[0] = static_cast<uint8_t>(src[0] * alpha + dst[0] * inv_alpha);
  dst[1] = static_cast<uint8_t>(src[1] * alpha + dst[1] * inv_alpha);
  dst[2] = static_cast<uint8_t>(src[2] * alpha + dst[2] * inv_alpha);
  dst[3] = 255;
}

/**
 * Copy a 32x32 sprite tile onto a larger canvas with alpha blending.
 *
 * @param src Source sprite RGBA data (32x32 = 4096 bytes)
 * @param dst Destination canvas RGBA data
 * @param dst_width Width of destination canvas in pixels
 * @param dest_x X offset in destination canvas
 * @param dest_y Y offset in destination canvas
 * @param dst_height Height of destination canvas in pixels (defaults to
 * dst_width for square)
 */
inline void blendSpriteTile(const uint8_t *src, uint8_t *dst, int dst_width,
                            int dest_x, int dest_y, int dst_height = -1) {
  if (!src || !dst)
    return;
  if (dst_height < 0)
    dst_height = dst_width; // Default to square canvas

  constexpr int TILE_SIZE = 32;

  for (int y = 0; y < TILE_SIZE; ++y) {
    // Bounds check: skip rows outside destination
    if (dest_y + y < 0 || dest_y + y >= dst_height)
      continue;

    for (int x = 0; x < TILE_SIZE; ++x) {
      // Bounds check: skip columns outside destination
      if (dest_x + x < 0 || dest_x + x >= dst_width)
        continue;

      const int src_idx = (y * TILE_SIZE + x) * 4;
      const int dst_idx = ((dest_y + y) * dst_width + (dest_x + x)) * 4;
      blendPixel(src + src_idx, dst + dst_idx);
    }
  }
}

} // namespace ImageBlending
} // namespace Utils
} // namespace MapEditor
