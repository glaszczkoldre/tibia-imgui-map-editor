#pragma once

#include <cstdint>

namespace MapEditor {
namespace Rendering {

/**
 * Rectangle bounds for map regions
 */
struct MinimapBounds {
    int32_t min_x = 0;
    int32_t min_y = 0;
    int32_t max_x = 0;
    int32_t max_y = 0;
    
    int32_t getWidth() const { return max_x - min_x + 1; }
    int32_t getHeight() const { return max_y - min_y + 1; }
    
    bool contains(int32_t x, int32_t y) const {
        return x >= min_x && x <= max_x && y >= min_y && y <= max_y;
    }
};

/**
 * Interface for minimap data source.
 * Decouples the minimap renderer from specific map implementations.
 */
class IMinimapDataSource {
public:
    virtual ~IMinimapDataSource() = default;
    
    /**
     * Get minimap color for a tile at given position.
     * @return Color index (0-255) where 0 = transparent
     */
    virtual uint8_t getTileColor(int32_t x, int32_t y, int16_t z) const = 0;
    
    /**
     * Get the bounds of the loaded map.
     */
    virtual MinimapBounds getMapBounds() const = 0;
    
    /**
     * Check if a tile exists at the given position.
     */
    virtual bool hasTile(int32_t x, int32_t y, int16_t z) const = 0;
};

} // namespace Rendering
} // namespace MapEditor
