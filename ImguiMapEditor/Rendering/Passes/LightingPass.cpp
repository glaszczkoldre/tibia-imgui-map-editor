#include "Rendering/Passes/LightingPass.h"
#include "Rendering/Frame/RenderState.h"
#include "Rendering/Light/LightManager.h"
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

  // Auto-invalidate if lighting config changes
  uint32_t config_hash = config.computeHash();
  if (config_hash != context.state.last_config_hash) {
    context.state.light_manager->invalidateAll();
    context.state.last_config_hash = config_hash;
  }

  context.state.light_manager->renderClientVisible(
      context.map, context.viewport_width, context.viewport_height,
      context.camera.getX(), context.camera.getY(), context.camera.getZoom(),
      static_cast<int>(context.current_floor),
      config, context.light_visibility_origin);
}

} // namespace Rendering
} // namespace MapEditor
