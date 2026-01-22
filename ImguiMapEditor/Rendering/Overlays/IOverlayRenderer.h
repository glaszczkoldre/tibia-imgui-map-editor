#pragma once
#include "Domain/ChunkedMap.h"
#include "Domain/CopyBuffer.h"
#include "Services/ClientDataService.h"
#include "Services/CreatureSimulator.h"
#include "Services/SpriteManager.h"
#include "Services/ViewSettings.h"
#include <glm/glm.hpp>
#include <imgui.h>
#include <vector>


namespace MapEditor {
namespace Rendering {
struct OverlayCollector;
class OverlaySpriteCache; // Forward declaration

/**
 * Interface for overlay rendering.
 * Allows different overlay rendering implementations and enables
 * testing/mocking.
 */
class IOverlayRenderer {
public:
  virtual ~IOverlayRenderer() = default;

  /**
   * Render all enabled overlays based on settings.
   */
  virtual void render(
      Domain::ChunkedMap *map, Services::ClientDataService *client_data,
      Services::SpriteManager *sprite_manager,
      OverlaySpriteCache *overlay_cache, Services::CreatureSimulator *simulator,
      const Services::ViewSettings &settings, const glm::vec2 &viewport_pos,
      const glm::vec2 &viewport_size, const glm::vec2 &camera_pos, float zoom,
      int current_floor, const OverlayCollector *collector = nullptr) = 0;

  // NOTE: renderDraggedItems and renderPastePreview removed
  // Now handled by unified PreviewService/PreviewRenderer
};

} // namespace Rendering
} // namespace MapEditor
