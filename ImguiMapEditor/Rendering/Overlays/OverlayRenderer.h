#pragma once
#include "Domain/ChunkedMap.h"
#include "Domain/CopyBuffer.h"
#include "IOverlayRenderer.h"
#include "OverlayCollector.h"
#include "OverlaySpriteCache.h"
#include "Services/ClientDataService.h"
#include "Services/CreatureSimulator.h"
#include "Services/SpriteManager.h"
#include "Services/ViewSettings.h"
#include "SpawnLabelOverlay.h"
#include "TooltipOverlay.h"
#include "WaypointOverlay.h"

// ItemPreviewRenderer.h removed - now using unified PreviewRenderer
#include "Core/Config.h"
#include <glm/glm.hpp>
#include <imgui.h>
#include <memory>

namespace MapEditor {
namespace Domain {
}
namespace Rendering {

/**
 * Coordinator for all map overlay rendering.
 * Delegates to specialized sub-renderers for each overlay type.
 * Implements IOverlayRenderer interface.
 *
 * Single responsibility: coordinate overlay rendering.
 */
class OverlayRenderer : public IOverlayRenderer {
public:
  OverlayRenderer();
  ~OverlayRenderer() override = default;

  /**
   * Render all enabled overlays.
   */
  void render(Domain::ChunkedMap *map, Services::ClientDataService *client_data,
              Services::SpriteManager *sprite_manager,
              OverlaySpriteCache *overlay_cache,
              Services::CreatureSimulator *simulator,
              const Services::ViewSettings &settings,
              const glm::vec2 &viewport_pos, const glm::vec2 &viewport_size,
              const glm::vec2 &camera_pos, float zoom, int current_floor,
              const OverlayCollector *collector = nullptr) override;

  /**
   * Set LOD mode to enable/disable simplified rendering.
   * Propagates to sub-renderers.
   */
  void setLODMode(bool enabled);

  // NOTE: renderDraggedItems, renderPastePreview and renderBrushPreview removed
  // Now handled by unified PreviewService/PreviewRenderer

private:
  // Sub-renderers
  SpawnLabelOverlay spawn_renderer_;
  WaypointOverlay waypoint_renderer_;
  TooltipOverlay tooltip_renderer_;
  // ItemPreviewRenderer removed - now using unified PreviewRenderer

  // Cached state for coordinate transforms
  glm::vec2 viewport_pos_;
  glm::vec2 viewport_size_;
  glm::vec2 camera_pos_;
  float zoom_;

  static constexpr float TILE_SIZE = Config::Rendering::TILE_SIZE;
};

} // namespace Rendering
} // namespace MapEditor
