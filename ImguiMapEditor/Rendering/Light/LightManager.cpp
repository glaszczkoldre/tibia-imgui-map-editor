#include "LightManager.h"

#include <algorithm>
#include <cmath>

#include <spdlog/spdlog.h>

#include "LightColorPalette.h"
#include "Core/Config.h"
#include "Rendering/Light/LightProjection.h"
#include "Services/ClientDataService.h"
#include "Rendering/Visibility/FloorVisibilityCalculator.h"

namespace MapEditor {
namespace Rendering {

LightManager::LightManager(Services::ClientDataService* client_data)
    : client_data_(client_data)
{
}

LightManager::~LightManager() = default;

bool LightManager::initialize() {
    texture_ = std::make_unique<LightTexture>();
    overlay_ = std::make_unique<LightOverlay>();
    gatherer_ = std::make_unique<LightGatherer>();

    if (!texture_->initialize()) {
        spdlog::error("LightManager: Failed to initialize texture");
        return false;
    }
    if (!overlay_->initialize()) {
        spdlog::error("LightManager: Failed to initialize overlay");
        return false;
    }
    
    return true;
}

void LightManager::invalidateTile(int32_t x, int32_t y) {
    (void)x;
    (void)y;
    force_update_ = true;
}

void LightManager::invalidateAll() {
    force_update_ = true;
}

void LightManager::renderClientVisible(
    const Domain::ChunkedMap& map,
    int viewport_width, int viewport_height,
    float camera_x, float camera_y,
    float zoom, int current_floor,
    const Domain::LightConfig& config,
    std::optional<Domain::Position> visibility_origin,
    const std::vector<Domain::LightSource>* injected_lights)
{
    FloorVisibilityCalculator floor_visibility(client_data_);
    const int visibility_x =
        visibility_origin ? visibility_origin->x : static_cast<int>(camera_x);
    const int visibility_y =
        visibility_origin ? visibility_origin->y : static_cast<int>(camera_y);
    const int start_floor = floor_visibility.calcLastVisibleFloor(current_floor);
    const int end_floor = floor_visibility.calcFirstVisibleFloor(
        map, visibility_x, visibility_y, current_floor);

    render(map, viewport_width, viewport_height, camera_x, camera_y, zoom,
           current_floor, start_floor, end_floor, config, injected_lights);
}

void LightManager::render(const Domain::ChunkedMap& map,
                          int viewport_width, int viewport_height,
                          float camera_x, float camera_y, 
                          float zoom, int current_floor,
                          int start_floor, int end_floor,
                          const Domain::LightConfig& config,
                          const std::vector<Domain::LightSource>* injected_lights)
{
    if (!config.enabled) return;

    // 1. Calculate tile bounds FIRST (to check against cache)
    float tiles_x = viewport_width / (32.0f * zoom);
    float tiles_y = viewport_height / (32.0f * zoom);
    
    int start_x = static_cast<int>(std::floor(camera_x - tiles_x / 2)) - 1;
    int end_x = static_cast<int>(std::ceil(camera_x + tiles_x / 2)) + 2;
    int start_y = static_cast<int>(std::floor(camera_y - tiles_y / 2)) - 1;
    int end_y = static_cast<int>(std::ceil(camera_y + tiles_y / 2)) + 2;
    
    int width_tiles = end_x - start_x;
    int height_tiles = end_y - start_y;
    
    if (width_tiles <= 0 || height_tiles <= 0) return;

    // OPTIMIZATION: Check if visible region, floor range, or config changed
    bool bounds_changed =
        start_x != last_start_x_ ||
        start_y != last_start_y_ ||
        width_tiles != last_width_tiles_ ||
        height_tiles != last_height_tiles_ ||
        current_floor != last_floor_ ||
        start_floor != last_start_floor_ ||
        end_floor != last_end_floor_;

    bool config_changed =
        config.global_light.color != last_config_.global_light.color ||
        config.global_light.intensity != last_config_.global_light.intensity ||
        config.client_slider != last_config_.client_slider ||
        config.camera_floor != last_config_.camera_floor ||
        config.enabled != last_config_.enabled;

    // If bounds and config match, we can reuse the texture
    // But we MUST recalculate screen coordinates because sub-pixel camera movement
    // changes where the texture is drawn, even if the integer tile range is same.
    if (!bounds_changed && !config_changed && !force_update_) {
        float world_x = start_x * 32.0f;
        float world_y = start_y * 32.0f;
        float screen_x = (world_x - camera_x * 32.0f) * zoom + viewport_width / 2.0f;
        float screen_y = (world_y - camera_y * 32.0f) * zoom + viewport_height / 2.0f;
        float screen_w = width_tiles * 32.0f * zoom;
        float screen_h = height_tiles * 32.0f * zoom;

        overlay_->apply(texture_->getTextureId(),
                       glm::vec4(screen_x, screen_y, screen_w, screen_h),
                       glm::vec2(viewport_width, viewport_height));
        return;
    }

    // Update state
    last_start_x_ = start_x;
    last_start_y_ = start_y;
    last_width_tiles_ = width_tiles;
    last_height_tiles_ = height_tiles;
    last_floor_ = current_floor;
    last_start_floor_ = start_floor;
    last_end_floor_ = end_floor;
    last_config_ = config;
    force_update_ = false;

    // 2. Gather one RME-style viewport light buffer and compute brightness globally.
    size_t required_size = width_tiles * height_tiles * 4;
    if (viewport_buffer_.size() < required_size) {
        viewport_buffer_.resize(required_size);
    }

    light_buffer_.init(start_x, start_y, width_tiles, height_tiles);
    gatherer_->gatherViewportLightBuffer(
        map, client_data_, static_cast<int16_t>(current_floor),
        static_cast<int16_t>(start_floor), static_cast<int16_t>(end_floor),
        light_buffer_);

    if (injected_lights) {
        for (const auto& light : *injected_lights) {
            Domain::LightSource projected = light;
            const int32_t floor_offset = projectedFloorOffsetTiles(
                static_cast<int16_t>(current_floor), light.source_floor);
            projected.x -= floor_offset;
            projected.y -= floor_offset;

            if (!light_buffer_.lights.empty()) {
                auto& prev = light_buffer_.lights.back();
                if (prev.x == projected.x && prev.y == projected.y &&
                    prev.color == projected.color &&
                    prev.source_floor == projected.source_floor) {
                    prev.intensity = std::max(prev.intensity, projected.intensity);
                    continue;
                }
            }

            light_buffer_.lights.push_back(projected);
        }
    }

    computeViewportLight(light_buffer_, config);

    // 3. Upload and Render
    texture_->upload(viewport_buffer_, width_tiles, height_tiles);
    
    // Calc screen rect
    float world_x = start_x * 32.0f; // TILE_SIZE
    float world_y = start_y * 32.0f;
    
    float screen_x = (world_x - camera_x * 32.0f) * zoom + viewport_width / 2.0f;
    float screen_y = (world_y - camera_y * 32.0f) * zoom + viewport_height / 2.0f;
    float screen_w = width_tiles * 32.0f * zoom;
    float screen_h = height_tiles * 32.0f * zoom;
    
    overlay_->apply(texture_->getTextureId(),
                   glm::vec4(screen_x, screen_y, screen_w, screen_h),
                   glm::vec2(viewport_width, viewport_height));
}

void LightManager::computeViewportLight(const ViewportLightBuffer& light_buffer,
                                        const Domain::LightConfig& config)
{
    const bool above_ground = config.camera_floor <= Config::Map::GROUND_LAYER;
    const uint8_t ambient_color = above_ground
        ? config.global_light.color
        : Config::Lighting::DEFAULT_SERVER_LIGHT_COLOR;
    const uint8_t ambient_intensity = above_ground
        ? std::max(config.client_slider, config.global_light.intensity)
        : config.client_slider;

    float ambient_r, ambient_g, ambient_b;
    LightColorPalette::from8bitFloat(ambient_color, ambient_r, ambient_g, ambient_b);
    const float ambient_scale = static_cast<float>(ambient_intensity) / 255.0f;
    ambient_r *= ambient_scale;
    ambient_g *= ambient_scale;
    ambient_b *= ambient_scale;
    
    uint8_t base_r = static_cast<uint8_t>(std::min(ambient_r * 255.0f, 255.0f));
    uint8_t base_g = static_cast<uint8_t>(std::min(ambient_g * 255.0f, 255.0f));
    uint8_t base_b = static_cast<uint8_t>(std::min(ambient_b * 255.0f, 255.0f));
    
    // Fill with ambient first
    const size_t pixel_count =
        static_cast<size_t>(light_buffer.width) * static_cast<size_t>(light_buffer.height);
    for (size_t i = 0; i < pixel_count; ++i) {
        const size_t base = i * 4;
        viewport_buffer_[base + 0] = base_r;
        viewport_buffer_[base + 1] = base_g;
        viewport_buffer_[base + 2] = base_b;
        viewport_buffer_[base + 3] = 255;
    }
    
    // Iterate lights FIRST, then only tiles affected by each light
    for (size_t light_index = 0; light_index < light_buffer.lights.size(); ++light_index) {
        const auto& light = light_buffer.lights[light_index];
        // Pre-compute light color once per light
        float lr, lg, lb;
        LightColorPalette::from8bitFloat(light.color, lr, lg, lb);
        
        // Calculate bounding box of affected tiles (in viewport-local coords)
        int radius = light.intensity;
        int min_x = std::max(0, light.x - radius - light_buffer.origin_x);
        int max_x = std::min(light_buffer.width - 1,
                             light.x + radius - light_buffer.origin_x);
        int min_y = std::max(0, light.y - radius - light_buffer.origin_y);
        int max_y = std::min(light_buffer.height - 1,
                             light.y + radius - light_buffer.origin_y);
        
        // Skip if light doesn't affect this viewport
        if (min_x >= light_buffer.width || max_x < 0 ||
            min_y >= light_buffer.height || max_y < 0) {
            continue;
        }
        
        // Only iterate tiles within light's bounding box
        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {
                const size_t tile_index =
                    static_cast<size_t>(y) * static_cast<size_t>(light_buffer.width) +
                    static_cast<size_t>(x);
                if (light_index < light_buffer.tile_start[tile_index]) {
                    continue;
                }

                int tile_x = light_buffer.origin_x + x;
                int tile_y = light_buffer.origin_y + y;
                
                float dx = (static_cast<float>(tile_x) + 0.5f) - (static_cast<float>(light.x) + 0.5f);
                float dy = (static_cast<float>(tile_y) + 0.5f) - (static_cast<float>(light.y) + 0.5f);
                float dist_sq = dx*dx + dy*dy;
                
                // Skip tiles outside actual radius (bounding box overestimates)
                float radius_sq = static_cast<float>(radius * radius);
                if (dist_sq > radius_sq) continue;
                
                float distance = std::sqrt(dist_sq);
                float intensity = (-distance + static_cast<float>(light.intensity)) * 0.2f;
                
                if (intensity < 0.01f) continue;
                if (intensity > 1.0f) intensity = 1.0f;
                
                // Update pixel with max blending
                const size_t base = tile_index * 4;
                int r = std::max(static_cast<int>(viewport_buffer_[base + 0]),
                                 static_cast<int>(lr * intensity * 255.0f));
                int g = std::max(static_cast<int>(viewport_buffer_[base + 1]),
                                 static_cast<int>(lg * intensity * 255.0f));
                int b = std::max(static_cast<int>(viewport_buffer_[base + 2]),
                                 static_cast<int>(lb * intensity * 255.0f));

                viewport_buffer_[base + 0] = static_cast<uint8_t>(std::min(r, 255));
                viewport_buffer_[base + 1] = static_cast<uint8_t>(std::min(g, 255));
                viewport_buffer_[base + 2] = static_cast<uint8_t>(std::min(b, 255));
                viewport_buffer_[base + 3] = 255;
            }
        }
    }
}

} // namespace Rendering
} // namespace MapEditor
