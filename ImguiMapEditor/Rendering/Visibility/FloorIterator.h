#pragma once

#include <algorithm>
#include "Core/Config.h"

namespace MapEditor {
namespace Rendering {

/**
 * Floor range calculated for multi-floor rendering (RME painter's algorithm).
 */
struct FloorRange {
    int start_z;      ///< First floor to render (highest Z, furthest back)
    int end_z;        ///< Main floor being viewed
    int super_end_z;  ///< Last floor to render (lowest Z for underground cutoff)
};

/**
 * Pure algorithm class for floor iteration logic.
 * Matches RME_Readonly's MapDrawer::SetupVars() floor calculation.
 * 
 * CONCERN: Floor iteration algorithms only. No GPU calls, no rendering.
 */
class FloorIterator {
public:
    static constexpr float TILE_SIZE = Config::Rendering::TILE_SIZE;

    /**
     * Calculate the range of floors to render based on current floor.
     * 
     * RME Logic:
     * - Above ground (floor <= 7): start_z = 7, end_z = floor, super_end_z = 0
     * - Underground (floor > 7): start_z = min(15, floor+2), end_z = floor, super_end_z = 8
     * 
     * @param current_floor The floor the user is viewing
     * @return FloorRange with start, end, and super_end values
     */
    static FloorRange calculateRange(int current_floor) {
        FloorRange range;
        
        if (current_floor <= Config::Map::GROUND_LAYER) {
            // Above ground: render from ground level down to current floor
            range.start_z = Config::Map::GROUND_LAYER;
            range.end_z = current_floor;
            range.super_end_z = 0;
        } else {
            // Underground: render from floor+2 down to floor 8
            range.start_z = std::min((int)Config::Map::MAX_FLOOR, current_floor + 2);
            range.end_z = current_floor;
            range.super_end_z = 8;
        }
        
        return range;
    }

    /**
     * Calculate the pixel offset for a tile based on floor difference.
     * Creates the parallax/isometric depth effect.
     * 
     * RME Logic:
     * - Above/at ground (z <= 7): offset = (7 - z) * TileSize
     * - Underground (z > 7): offset = (current_floor - z) * TileSize
     * 
     * @param current_floor The floor being viewed
     * @param tile_z The floor of the tile being rendered
     * @return Offset in pixels to subtract from both X and Y
     */
    static float getFloorOffset(int current_floor, int tile_z) {
        if (tile_z <= Config::Map::GROUND_LAYER) {
            return static_cast<float>(Config::Map::GROUND_LAYER - tile_z) * TILE_SIZE;
        }
        return static_cast<float>(current_floor - tile_z) * TILE_SIZE;
    }

    /**
     * Determine if shade overlay should be drawn at this floor.
     * Shade is drawn at the transition between ghost floors and main floor.
     * 
     * @param map_z Current floor in iteration
     * @param range Floor range being rendered
     * @param show_shade User preference for shade visibility
     * @return True if shade should be drawn
     */
    static bool shouldDrawShade(int map_z, const FloorRange& range, bool show_shade) {
        return show_shade && 
               map_z == range.end_z && 
               range.start_z != range.end_z;
    }

    /**
     * Check if a floor should be rendered (tiles drawn).
     * Floors between end_z and start_z are rendered.
     * 
     * @param map_z Current floor in iteration
     * @param range Floor range being rendered
     * @return True if tiles should be rendered on this floor
     */
    static bool shouldRenderFloor(int map_z, const FloorRange& range) {
        return map_z >= range.end_z;
    }
    
    /**
     * Calculate floor range with show_all_floors toggle support.
     * When show_all_floors is false, only renders the current floor.
     * 
     * @param current_floor The floor the user is viewing
     * @param show_all_floors Whether to show multiple floors
     * @return FloorRange adjusted based on toggle
     */
    static FloorRange calculateRangeWithToggle(int current_floor, bool show_all_floors) {
        if (!show_all_floors) {
            // Single floor mode
            FloorRange range;
            range.start_z = current_floor;
            range.end_z = current_floor;
            range.super_end_z = current_floor;
            return range;
        }
        return calculateRange(current_floor);
    }
    
    /**
     * Check if ghost higher floor should be rendered.
     * Ghost floor is rendered at reduced alpha to show floor above.
     * 
     * @param current_floor The floor being viewed
     * @param ghost_higher_enabled User toggle state
     * @return Floor number to render as ghost, or -1 if none
     */
    static int getGhostHigherFloor(int current_floor, bool ghost_higher_enabled) {
        if (!ghost_higher_enabled) return -1;
        if (current_floor <= 0) return -1;  // Can't ghost above floor 0
        return current_floor - 1;
    }
    
    /**
     * Check if ghost lower floor should be rendered.
     * 
     * @param current_floor The floor being viewed  
     * @param ghost_lower_enabled User toggle state
     * @return Floor number to render as ghost, or -1 if none
     */
    static int getGhostLowerFloor(int current_floor, bool ghost_lower_enabled) {
        if (!ghost_lower_enabled) return -1;
        if (current_floor >= Config::Map::MAX_FLOOR) return -1;  // Can't ghost below floor 15
        return current_floor + 1;
    }
    
    /// Ghost floor alpha (RME uses 96/255 ≈ 0.38)
    static constexpr float GHOST_ALPHA = 96.0f / 255.0f;
};

} // namespace Rendering
} // namespace MapEditor
