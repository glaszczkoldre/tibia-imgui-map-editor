#pragma once

#include <vector>
#include <cstdint>
#include "Domain/ChunkedMap.h"
#include "Domain/LightTypes.h"
namespace MapEditor {

namespace Services {
    class ClientDataService;
}

namespace Rendering {

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
     * 
     * @param map The map to gather lights from
     * @param chunk_x Chunk X coordinate
     * @param chunk_y Chunk Y coordinate
     * @param client_data Client data service for item type lookup
     * @param start_floor First floor to gather from (highest Z)
     * @param end_floor Last floor to gather from (lowest Z)
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

private:
    /**
     * Helper to gather lights from a single neighbor chunk on a specific floor.
     */
    void gatherLightsFromNeighborChunk(
        const MapEditor::Domain::ChunkedMap& map,
        int32_t target_cx, int32_t target_cy,
        Services::ClientDataService* client_data,
        int16_t floor, int32_t floor_offset);

    std::vector<MapEditor::Domain::LightSource> lights_;
};

} // namespace Rendering
} // namespace MapEditor
