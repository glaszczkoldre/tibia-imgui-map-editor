#pragma once
#include "Domain/Position.h"
#include "../../Core/Config.h"
#include <glm/glm.hpp>

namespace MapEditor {
namespace Rendering {
namespace Utils {

/**
 * Transforms a world tile position to screen coordinates.
 * Applies camera offset, zoom, and floor-based parallax.
 */
inline glm::vec2 tileToScreen(const Domain::Position& pos,
                              const glm::vec2& camera_pos,
                              const glm::vec2& viewport_pos,
                              const glm::vec2& viewport_size,
                              float zoom) {
    float floor_offset = 0.0f;
    if (pos.z <= Config::Map::GROUND_LAYER) {
        floor_offset = static_cast<float>(Config::Map::GROUND_LAYER - pos.z) * Config::Rendering::TILE_SIZE * zoom;
    }

    glm::vec2 offset(
        static_cast<float>(pos.x) - camera_pos.x,
        static_cast<float>(pos.y) - camera_pos.y
    );
    offset *= Config::Rendering::TILE_SIZE * zoom;

    offset.x -= floor_offset;
    offset.y -= floor_offset;

    return viewport_pos + viewport_size * 0.5f + offset;
}

} // namespace Utils
} // namespace Rendering
} // namespace MapEditor
