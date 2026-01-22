#include "OverlayManager.h"
#include "Application/EditorSession.h"
#include "Core/Config.h"
#include "Rendering/Overlays/GridOverlay.h"
#include "Rendering/Overlays/OverlayRenderer.h"
#include "Rendering/Overlays/PreviewOverlay.h"
#include "Rendering/Overlays/SelectionOverlay.h"
#include "Rendering/Overlays/StatusOverlay.h"
#include "Services/Selection/SelectionService.h"

namespace MapEditor {
namespace Rendering {

OverlayManager::OverlayManager() {
  grid_overlay_ = std::make_unique<GridOverlay>();
  status_overlay_ = std::make_unique<StatusOverlay>();
  selection_overlay_ = std::make_unique<SelectionOverlay>();
  preview_overlay_ = std::make_unique<PreviewOverlay>();
  overlay_renderer_ = std::make_unique<OverlayRenderer>();
}

OverlayManager::~OverlayManager() {
  // Unregister from any bound selection service
  if (bound_selection_service_) {
    bound_selection_service_->removeObserver(selection_overlay_.get());
  }
}

void OverlayManager::render(ImDrawList *draw_list,
                            const UI::MapViewCamera &camera,
                            const AppLogic::EditorSession *session,
                            bool is_hovered, float framerate) {
  if (!draw_list)
    return;

  // Orchestration logic can be added here.
  // Currently MapPanel orchestrates by calling get*Overlay().render(...)
}

GridOverlay &OverlayManager::getGridOverlay() { return *grid_overlay_; }

StatusOverlay &OverlayManager::getStatusOverlay() { return *status_overlay_; }

SelectionOverlay &OverlayManager::getSelectionOverlay() {
  return *selection_overlay_;
}

PreviewOverlay &OverlayManager::getPreviewOverlay() {
  return *preview_overlay_;
}

OverlayRenderer &OverlayManager::getOverlayRenderer() {
  return *overlay_renderer_;
}

void OverlayManager::setLODMode(bool enabled) {
  if (overlay_renderer_) {
    overlay_renderer_->setLODMode(enabled);
  }
}

void OverlayManager::bindSelectionService(
    Services::Selection::SelectionService *service) {
  // Unregister from previous service if any
  if (bound_selection_service_ && selection_overlay_) {
    bound_selection_service_->removeObserver(selection_overlay_.get());
  }

  bound_selection_service_ = service;

  // Register with new service
  if (service && selection_overlay_) {
    service->addObserver(selection_overlay_.get());
  }
}

} // namespace Rendering
} // namespace MapEditor

