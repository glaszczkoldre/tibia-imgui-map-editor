#pragma once

#include <vector>
#include <cstdint>
#include <array>
#include "Domain/ChunkedMap.h"
#include "Domain/LightTypes.h"
namespace MapEditor {

namespace Services {
    class ClientDataService;
}

namespace Rendering {

/**
 * Per-tile ground blocking data for a chunk.
 * Records the highest floor (lowest Z) with solid ground above each tile.
 * Lights from floors above blocking_floor are blocked.
 */
struct GroundBrightness {
    // For each tile (32x32), the floor with solid ground that blocks lights from above.
    // Value of -1 means no blocking (all lights pass through).
    std::array<int16_t, 32 * 32> blocking_floor;
    
    GroundBrightness() {
        blocking_floor.fill(-1);
    }
};

/**
 * Collects light sources from visible tiles.
 * 
 * Single responsibility: iterate chunks/tiles and extract
 * light data from items that have light properties.
 */
class LightGatherer {
public:
    /**
     * Clear all collected lights for a new frame.
     */
    void clear();
    
    /**
     * Gather all light sources relevant to a specific chunk.
     * scans the target chunk AND its 8 neighbors (3x3 grid) to account for light spilling.
     */
    void gatherForChunk(
        const MapEditor::Domain::ChunkedMap& map,
        int32_t chunk_x, int32_t chunk_y,
        Services::ClientDataService* client_data,
        int16_t floor);
    
    /**
     * Gather all light sources from multiple floors for a specific chunk.
     * Applies isometric offset to light positions based on floor difference.
     * Also populates ground_brightness_ for light blocking.
     */
    void gatherForChunkMultiFloor(
        const MapEditor::Domain::ChunkedMap& map,
        int32_t chunk_x, int32_t chunk_y,
        Services::ClientDataService* client_data,
        int16_t start_floor,
        int16_t end_floor);
    
    /**
     * Get the collected light sources.
     */
    const std::vector<MapEditor::Domain::LightSource>& getLights() const { return lights_; }
    
    /**
     * Get number of light sources collected.
     */
    size_t getLightCount() const { return lights_.size(); }
    
    /**
     * Get the ground blocking data for the target chunk.
     * Only valid after gatherForChunkMultiFloor() has been called.
     */
    const GroundBrightness& getGroundBrightness() const { return ground_brightness_; }

private:
    /**
     * Helper to gather lights from a single neighbor chunk on a specific floor.
     */
    void gatherLightsFromNeighborChunk(
        const MapEditor::Domain::ChunkedMap& map,
        int32_t target_cx, int32_t target_cy,
        Services::ClientDataService* client_data,
        int16_t floor, int32_t floor_offset,
        bool is_target_chunk);

    /**
     * Add a light source with deduplication.
     * If the last light has the same position, color, and floor, merge (keep higher intensity).
     */
    void addLight(int32_t x, int32_t y, uint8_t color, uint8_t intensity, int16_t floor);

    std::vector<MapEditor::Domain::LightSource> lights_;
    GroundBrightness ground_brightness_;
};

} // namespace Rendering
} // namespace MapEditor
