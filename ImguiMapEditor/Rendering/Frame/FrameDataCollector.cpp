#include "Rendering/Frame/FrameDataCollector.h"
#include "Rendering/Overlays/WaypointOverlay.h"
#include "Rendering/Passes/SpawnTintPass.h"
#include "Services/SpriteManager.h"


namespace MapEditor {
namespace Rendering {

void FrameDataCollector::beginFrame() {
  missing_sprites_.clear();
  // Note: chunk_buffer_ is cleared by collectSpawns when needed
}

void FrameDataCollector::collectSpawns(const Domain::ChunkedMap &map,
                                       int floor_z, const VisibleBounds &bounds,
                                       OverlayCollector &collector,
                                       const Services::ViewSettings &settings) {
  SpawnTintPass::collectVisibleSpawns(map, floor_z, bounds, collector, settings,
                                      chunk_buffer_);
}

void FrameDataCollector::collectWaypoints(
    const Domain::ChunkedMap &map, int floor_z, const VisibleBounds &bounds,
    OverlayCollector &collector, const Services::ViewSettings &settings,
    float floor_offset) {
  WaypointOverlay::collectVisibleWaypoints(map, floor_z, bounds, collector,
                                           settings, floor_offset);
}

void FrameDataCollector::endFrame(Services::SpriteManager *sprites) {
  if (sprites && !missing_sprites_.empty()) {
    sprites->requestSpritesAsync(missing_sprites_);
  }
}

} // namespace Rendering
} // namespace MapEditor
