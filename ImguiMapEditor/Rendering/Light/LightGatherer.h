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
 * RME-style viewport light buffer.
 *
 * All lights across all processed floors are appended to one vector in
 * top-to-bottom floor order. Each viewport tile stores the first light index
 * allowed to affect it; lower indices were collected from floors above a solid
 * ground tile and are blocked for that projected tile position.
 */
struct ViewportLightBuffer {
    std::vector<MapEditor::Domain::LightSource> lights;
    std::vector<uint32_t> tile_start;
    int width = 0;
    int height = 0;
    int origin_x = 0;
    int origin_y = 0;
    
    void init(int ox, int oy, int w, int h) {
        origin_x = ox;
        origin_y = oy;
        width = w;
        height = h;
        lights.clear();
        tile_start.assign(w * h, 0);
    }
    
    uint32_t getTileStart(int tx, int ty) const {
        int lx = tx - origin_x;
        int ly = ty - origin_y;
        if (lx < 0 || lx >= width || ly < 0 || ly >= height) return 0;
        return tile_start[ly * width + lx];
    }
    
    void setTileStart(int tx, int ty, uint32_t start) {
        int lx = tx - origin_x;
        int ly = ty - origin_y;
        if (lx < 0 || lx >= width || ly < 0 || ly >= height) return;
        tile_start[ly * width + lx] = start;
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
     * Gather lights and ground occlusion into a single RME-style viewport buffer.
     */
    void gatherViewportLightBuffer(
        const MapEditor::Domain::ChunkedMap& map,
        Services::ClientDataService* client_data,
        int16_t current_floor,
        int16_t start_floor,
        int16_t end_floor,
        ViewportLightBuffer& light_buffer);
    
private:
    void addLight(ViewportLightBuffer& light_buffer,
                  int32_t x, int32_t y,
                  uint8_t color, uint8_t intensity,
                  int16_t floor);

    void registerGroundOcclusionForViewport(
        const MapEditor::Domain::ChunkedMap& map,
        Services::ClientDataService* client_data,
        int16_t current_floor,
        int16_t floor,
        uint32_t floor_light_start,
        ViewportLightBuffer& light_buffer);

    void gatherLightsForViewportFloor(
        const MapEditor::Domain::ChunkedMap& map,
        Services::ClientDataService* client_data,
        int16_t current_floor,
        int16_t floor,
        ViewportLightBuffer& light_buffer);
};

} // namespace Rendering
} // namespace MapEditor
