#include "OverlayRenderer.h"
#include "Core/Config.h"
#include <cmath>

namespace MapEditor {
namespace Rendering {

OverlayRenderer::OverlayRenderer()
    : viewport_pos_(0.0f), viewport_size_(0.0f), camera_pos_(0.0f),
      zoom_(1.0f) {}

void OverlayRenderer::render(
    Domain::ChunkedMap *map, Services::ClientDataService *client_data,
    Services::SpriteManager *sprite_manager, OverlaySpriteCache *overlay_cache,
    Services::CreatureSimulator *simulator,
    const Services::ViewSettings &settings, const glm::vec2 &viewport_pos,
    const glm::vec2 &viewport_size, const glm::vec2 &camera_pos, float zoom,
    int current_floor, const OverlayCollector *collector) {
  // Cache state for sub-renderers
  viewport_pos_ = viewport_pos;
  viewport_size_ = viewport_size;
  camera_pos_ = camera_pos;
  zoom_ = zoom;

  if (!map)
    return;

  // PERFORMANCE: Skip detailed overlays at very low zoom (LOD mode)
  // At ≤20% zoom, tiles are ~6px and overlays are illegible anyway
  bool skip_detailed_overlays =
      (zoom <= Config::Performance::OVERLAY_ZOOM_THRESHOLD);

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->PushClipRect(ImVec2(viewport_pos.x, viewport_pos.y),
                          ImVec2(viewport_pos.x + viewport_size.x,
                                 viewport_pos.y + viewport_size.y),
                          true);

  // If collector is provided, use optimized path
  if (collector) {
    // Skip spawn indicators/borders at very low zoom - they're too small to see
    if (!skip_detailed_overlays) {
      // Render spawns and creatures (method handles its own
      // show_spawns/show_creatures gating)
      spawn_renderer_.renderFromCollector(
          draw_list, collector, map, client_data, sprite_manager, overlay_cache,
          simulator, settings, settings.show_spawns, settings.show_creatures,
          camera_pos, viewport_pos, viewport_size, current_floor, zoom);
    }

    if (settings.show_waypoints && !skip_detailed_overlays) {
      waypoint_renderer_.renderFromCollector(draw_list, collector->waypoints,
                                             camera_pos, viewport_pos,
                                             viewport_size, zoom);
    }

    if (settings.show_tooltips && !skip_detailed_overlays) {
      tooltip_renderer_.renderFromCollector(draw_list, collector->tooltips,
                                            camera_pos, viewport_pos,
                                            viewport_size, zoom);
    }

    // NOTE: Invalid items are now rendered inline in TileRenderer for proper
    // Z-order. No deferred overlay rendering needed.
  }

  draw_list->PopClipRect();
  draw_list->PopClipRect();
}

void OverlayRenderer::setLODMode(bool enabled) {
  spawn_renderer_.setLODMode(enabled);
  tooltip_renderer_.setLODMode(enabled);
}

// NOTE: renderDraggedItems, renderPastePreview and renderBrushPreview removed
// Now handled by unified PreviewService/PreviewRenderer

} // namespace Rendering
} // namespace MapEditor
