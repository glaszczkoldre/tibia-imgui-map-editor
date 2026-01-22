#pragma once
#include "Interfaces/IRibbonPanel.h"
#include <imgui.h>
#include <vector>
#include <memory>

namespace MapEditor {
namespace UI {
namespace Ribbon {

/**
 * Central controller for the ribbon UI system.
 * 
 * Manages a collection of ribbon panels and handles their layout
 * using ImGui's docking system. By default, all panels are docked
 * horizontally in a top ribbon area. Users can detach panels and
 * dock them elsewhere.
 */
class RibbonController {
public:
    RibbonController();
    ~RibbonController() = default;
    
    // Non-copyable
    RibbonController(const RibbonController&) = delete;
    RibbonController& operator=(const RibbonController&) = delete;
    
    /**
     * Add a panel to the ribbon.
     * Panels are rendered in the order they are added.
     * @param panel Panel to add (ownership transferred)
     */
    void AddPanel(std::unique_ptr<IRibbonPanel> panel);
    
    /**
     * Render the ribbon and all its panels.
     * Call this once per frame from the main render loop.
     */
    void Render();
    
    /**
     * Set the ribbon height in pixels.
     * @param height Height in pixels (default: 50)
     */
    void SetRibbonHeight(float height) { ribbon_height_ = height; }
    
    /**
     * Get the current ribbon height.
     * @return Height in pixels
     */
    float GetRibbonHeight() const { return ribbon_height_; }

private:
    /**
     * Set up the initial docking layout.
     * Called on the first frame to dock all panels into the top of main dockspace.
     */
    void SetupInitialDockLayout();
    
    std::vector<std::unique_ptr<IRibbonPanel>> panels_;
    float ribbon_height_ = 50.0f;
    bool first_frame_ = true;
};

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor
