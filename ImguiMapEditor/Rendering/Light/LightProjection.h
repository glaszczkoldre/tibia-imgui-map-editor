#pragma once

#include <cstdint>

#include "Core/Config.h"

namespace MapEditor::Rendering {

inline int32_t projectedFloorOffsetTiles(int16_t current_floor,
                                         int16_t tile_floor) {
    if (tile_floor <= Config::Map::GROUND_LAYER) {
        return Config::Map::GROUND_LAYER - tile_floor;
    }
    return current_floor - tile_floor;
}

} // namespace MapEditor::Rendering
