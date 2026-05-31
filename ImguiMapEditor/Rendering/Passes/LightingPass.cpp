#include "Rendering/Passes/LightingPass.h"
#include "Rendering/Frame/RenderState.h"
#include "Rendering/Light/LightManager.h"
#include "Rendering/Visibility/FloorIterator.h"
#include "Services/ViewSettings.h"
#include <memory>


namespace MapEditor {
namespace Rendering {

void LightingPass::render(const RenderContext &context) {
  if (!context.view_settings || !context.view_settings->map_lighting_enabled) {
    return;
  }

  if (!context.state.light_manager) {
    return;
  }

  // Build LightConfig from ViewSettings (new server light + client slider)
  Domain::LightConfig config;
  config.enabled = true;
  config.global_light.intensity = context.view_settings->server_light_intensity;
  config.global_light.color = context.view_settings->server_light_color;
  config.client_slider = static_cast<uint8_t>(context.view_settings->map_ambient_light);
  config.camera_floor = context.current_floor;
  // Legacy fields (kept for backward compat)
  config.ambient_color = config.global_light.color;
  config.ambient_level = config.global_light.intensity;

  // Auto-invalidate if server light or slider changes
  uint32_t config_hash = static_cast<uint32_t>(config.global_light.intensity) |
                         (static_cast<uint32_t>(config.global_light.color) << 8) |
                         (static_cast<uint32_t>(config.client_slider) << 16);
  if (config_hash != context.state.last_config_hash) {
    context.state.light_manager->invalidateAll();
    context.state.last_config_hash = config_hash;
  }

  // Calculate floor range based on show_all_floors setting
  bool show_all_floors = context.view_settings->show_all_floors;
  FloorRange floor_range = FloorIterator::calculateRangeWithToggle(
      context.current_floor, show_all_floors);

  // Invalidate if floor range changed (e.g., toggling show_all_floors)
  if (floor_range.start_z != last_start_floor_ || 
      floor_range.super_end_z != last_end_floor_) {
    context.state.light_manager->invalidateAll();
    last_start_floor_ = floor_range.start_z;
    last_end_floor_ = floor_range.super_end_z;
  }

  context.state.light_manager->render(
      context.map, context.viewport_width, context.viewport_height,
      context.camera.getX(), context.camera.getY(), context.camera.getZoom(),
      static_cast<int>(context.current_floor),
      floor_range.start_z, floor_range.super_end_z,
      config);
}

} // namespace Rendering
} // namespace MapEditor
