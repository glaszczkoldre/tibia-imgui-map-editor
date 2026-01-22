#pragma once
#include "Rendering/Minimap/MinimapRenderer.h"
#include "Rendering/Minimap/ChunkedMapMinimapSource.h"
#include "Domain/Position.h"
#include <imgui.h>
#include <memory>
#include <functional>

namespace MapEditor {

namespace Domain {
    class ChunkedMap;
}

namespace Services {
    class ClientDataService;
}

namespace AppLogic {
    class EditorSession;  // Forward declaration
}

namespace UI {

/**
 * Callback for viewport sync when user clicks on minimap
 */
using ViewportSyncCallback = std::function<void(int32_t x, int32_t y, int16_t z)>;

/**
 * ImGui-based minimap window with RME-style controls.
 * Features:
 * - Zoom buttons (1:1, 1:2, 1:4, 1:8, 1:16)
 * - Floor up/down buttons
 * - Click-to-center main viewport
 */
class MinimapWindow {
public:
    MinimapWindow();
    ~MinimapWindow();
    
    /**
     * Set map and client data sources
     */
    void setMap(Domain::ChunkedMap* map, Services::ClientDataService* clientData);
    
    /**
     * Set callback for syncing main viewport on click
     */
    void setViewportSyncCallback(ViewportSyncCallback callback);
    
    /**
     * Sync minimap view with main camera position
     */
    void syncWithCamera(int32_t x, int32_t y, int16_t floor);
    
    /**
     * Render the minimap window
     * @param p_visible Optional visibility flag for ImGui to manage
     */
    void render(bool* p_visible = nullptr);
    
    /**
     * Window visibility
     */
    bool isVisible() const { return visible_; }
    void setVisible(bool visible) { visible_ = visible; }
    void toggleVisible() { visible_ = !visible_; }
    
    /**
     * Get current floor (independent from main view)
     */
    int16_t getCurrentFloor() const { return renderer_.getFloor(); }
    
    /**
     * Save current minimap state to session for tab switching
     */
    void saveState(AppLogic::EditorSession& session);
    
    /**
     * Restore minimap state from session after tab switch
     */
    void restoreState(const AppLogic::EditorSession& session);

private:
    void renderToolbar();
    void renderMinimapImage();
    void handleMouseClick();
    
    Rendering::MinimapRenderer renderer_;
    std::unique_ptr<Rendering::ChunkedMapMinimapSource> data_source_;
    
    ViewportSyncCallback viewport_sync_callback_;
    
    bool visible_ = true;
    
    // Cached viewport info for overlay
    int32_t main_camera_x_ = 0;
    int32_t main_camera_y_ = 0;
    int16_t last_synced_floor_ = 7;  // Track last floor to detect main map floor changes
    int main_viewport_width_ = 0;
    int main_viewport_height_ = 0;
    
    // Drag state for panning
    bool is_dragging_ = false;
    ImVec2 drag_start_screen_{0, 0};
    int32_t drag_start_center_x_ = 0;
    int32_t drag_start_center_y_ = 0;
};

} // namespace UI
} // namespace MapEditor
