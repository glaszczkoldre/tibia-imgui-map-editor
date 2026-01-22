#pragma once
#include "Rendering/Utils/MathUtils.h"
#include <cmath>

namespace MapEditor {
namespace Rendering {

/**
 * Calculates visible tile bounds from camera and viewport parameters.
 */
struct VisibleBounds {
  int start_x = 0;
  int start_y = 0;
  int end_x = 0;
  int end_y = 0;

  /**
   * Calculate visible tile bounds for a given camera/viewport configuration.
   * @param camera_x Camera X position in tile coordinates
   * @param camera_y Camera Y position in tile coordinates
   * @param zoom Current zoom level
   * @param viewport_width Viewport width in pixels
   * @param viewport_height Viewport height in pixels
   * @param tile_size Size of a tile in pixels
   * @return Calculated bounds with margin
   */
  static VisibleBounds calculate(float camera_x, float camera_y, float zoom,
                                 int viewport_width, int viewport_height,
                                 float tile_size) {
    float tiles_x = viewport_width / (tile_size * zoom);
    float tiles_y = viewport_height / (tile_size * zoom);

    VisibleBounds bounds;
    bounds.start_x =
        Math::safe_float_to_int(std::floor(camera_x - tiles_x / 2)) - 1;
    bounds.end_x =
        Math::safe_float_to_int(std::ceil(camera_x + tiles_x / 2)) + 2;
    bounds.start_y =
        Math::safe_float_to_int(std::floor(camera_y - tiles_y / 2)) - 1;
    bounds.end_y =
        Math::safe_float_to_int(std::ceil(camera_y + tiles_y / 2)) + 2;
    return bounds;
  }

  /**
   * Expand bounds for parallax effect (RME-style multi-floor rendering).
   */
  VisibleBounds withFloorOffset(int floor_diff) const {
    VisibleBounds expanded = *this;
    expanded.start_x -= floor_diff;
    expanded.start_y -= floor_diff;
    expanded.end_x += floor_diff;
    expanded.end_y += floor_diff;
    return expanded;
  }
};

} // namespace Rendering
} // namespace MapEditor
