#pragma once
#include "LightTexture.h"
#include "LightOverlay.h"
#include "LightGatherer.h"
#include "Domain/ChunkedMap.h"
#include "Domain/LightTypes.h"
#include "Domain/Position.h"
#include <memory>
#include <optional>
#include <vector>

namespace MapEditor {
namespace Services {
    class ClientDataService;
}

namespace Rendering {

class LightManager {
public:
    LightManager(Services::ClientDataService* client_data);
    ~LightManager();

    // Non-copyable
    LightManager(const LightManager&) = delete;
    LightManager& operator=(const LightManager&) = delete;

    bool initialize();

    /**
     * Render the light overlay for the current viewport.
     * 
     * @param start_floor First floor to gather lights from (highest Z)
     * @param end_floor Last floor to gather lights from (lowest Z)
     *                  When start_floor == end_floor, only that floor is used
     * @param injected_lights Optional extra lights to add (e.g., player light in preview)
     */
    void render(const MapEditor::Domain::ChunkedMap& map,
                int viewport_width, int viewport_height,
                float camera_x, float camera_y, 
                float zoom, int current_floor,
                int start_floor, int end_floor,
                const MapEditor::Domain::LightConfig& config,
                const std::vector<MapEditor::Domain::LightSource>* injected_lights = nullptr);

    /**
     * Render using RME's light-mode floor visibility range.
     *
     * RME ignores the editor's show-all-floors range when collecting lights and
     * instead asks the client visibility algorithm which floors are actually
     * visible through ground/roof blockers.
     */
    void renderClientVisible(const MapEditor::Domain::ChunkedMap& map,
                             int viewport_width, int viewport_height,
                             float camera_x, float camera_y,
                             float zoom, int current_floor,
                             const MapEditor::Domain::LightConfig& config,
                             std::optional<MapEditor::Domain::Position> visibility_origin = std::nullopt,
                             const std::vector<MapEditor::Domain::LightSource>* injected_lights = nullptr);

    /**
     * Invalidate light cache for a specific tile position.
     * Should be called when tiles change.
     */
    void invalidateTile(int32_t x, int32_t y);

    /**
     * Invalidate all light cache (e.g. ambient light change).
     */
    void invalidateAll();

private:
    void computeViewportLight(const ViewportLightBuffer& light_buffer,
                              const MapEditor::Domain::LightConfig& config);

    Services::ClientDataService* client_data_;
    
    std::unique_ptr<LightTexture> texture_;
    std::unique_ptr<LightOverlay> overlay_;
    std::unique_ptr<LightGatherer> gatherer_;
    ViewportLightBuffer light_buffer_;

    std::vector<uint8_t> viewport_buffer_;

    // State tracking for optimization
    int last_start_x_ = 0;
    int last_start_y_ = 0;
    int last_width_tiles_ = 0;
    int last_height_tiles_ = 0;
    int last_floor_ = -1;
    int last_start_floor_ = -1;
    int last_end_floor_ = -1;
    MapEditor::Domain::LightConfig last_config_;
    bool force_update_ = true;
};

} // namespace Rendering
} // namespace MapEditor
