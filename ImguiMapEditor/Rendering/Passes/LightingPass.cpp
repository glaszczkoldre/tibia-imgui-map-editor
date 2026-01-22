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

  Domain::LightConfig config;
  config.enabled = true;
  config.ambient_level =
      static_cast<uint8_t>(context.view_settings->map_ambient_light);
  config.ambient_color = 215; // Default white-ish (from MapRenderer.cpp)

  // Auto-invalidate if ambient light changes
  // Note: RenderState tracking logic moved here
  if (config.ambient_level != context.state.last_ambient_light) {
    context.state.light_manager->invalidateAll();
    context.state.last_ambient_light = config.ambient_level;
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
