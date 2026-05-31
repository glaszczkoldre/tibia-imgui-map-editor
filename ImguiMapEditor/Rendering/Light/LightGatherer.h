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
 * Viewport-level ground blocking data.
 * For each tile position in the viewport, records the nearest floor (lowest Z)
 * with solid ground. Lights from floors BELOW (higher Z) the blocking floor are blocked.
 * 
 * This is the equivalent of RME's TileLight::start mechanism, but using
 * floor numbers instead of light indices.
 */
struct ViewportGroundBlocking {
    std::vector<int16_t> blocking_floor;  // -1 = no blocking, >=0 = floor with solid ground
    int width = 0;
    int height = 0;
    int origin_x = 0;  // tile-space origin (start_x from LightManager)
    int origin_y = 0;
    
    void init(int ox, int oy, int w, int h) {
        origin_x = ox;
        origin_y = oy;
        width = w;
        height = h;
        blocking_floor.assign(w * h, -1);
    }
    
    // Get blocking floor for a tile at absolute tile coords (tx, ty)
    int16_t getBlockingFloor(int tx, int ty) const {
        int lx = tx - origin_x;
        int ly = ty - origin_y;
        if (lx < 0 || lx >= width || ly < 0 || ly >= height) return -1;
        return blocking_floor[ly * width + lx];
    }
    
    // Set blocking floor for a tile at absolute tile coords (tx, ty)
    void setBlockingFloor(int tx, int ty, int16_t floor) {
        int lx = tx - origin_x;
        int ly = ty - origin_y;
        if (lx < 0 || lx >= width || ly < 0 || ly >= height) return;
        int idx = ly * width + lx;
        // Take the nearest floor (lowest Z) with solid ground
        if (blocking_floor[idx] < 0 || floor < blocking_floor[idx]) {
            blocking_floor[idx] = floor;
        }
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
     */
    void gatherForChunkMultiFloor(
        const MapEditor::Domain::ChunkedMap& map,
        int32_t chunk_x, int32_t chunk_y,
        Services::ClientDataService* client_data,
        int16_t start_floor,
        int16_t end_floor);
    
    /**
     * Register ground blocking for viewport tiles from a specific chunk/floor.
     * Does NOT collect lights — only populates the blocking grid.
     * Used in the pre-pass to build complete blocking data before light computation.
     */
    void registerGroundBlockingForChunk(
        const MapEditor::Domain::ChunkedMap& map,
        int32_t chunk_x, int32_t chunk_y,
        Services::ClientDataService* client_data,
        int16_t floor,
        int32_t floor_offset,
        ViewportGroundBlocking& viewport_blocking);
    
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

    /**
     * Add a light source with deduplication.
     * If the last light has the same position, color, and floor, merge (keep higher intensity).
     */
    void addLight(int32_t x, int32_t y, uint8_t color, uint8_t intensity, int16_t floor);

    std::vector<MapEditor::Domain::LightSource> lights_;
};

} // namespace Rendering
} // namespace MapEditor
