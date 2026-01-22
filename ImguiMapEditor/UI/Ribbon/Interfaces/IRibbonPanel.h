#pragma once

namespace MapEditor {
namespace UI {
namespace Ribbon {

/**
 * Abstract interface for ribbon panels.
 * 
 * Each panel represents a logical group of controls (File, Edit, View, etc.)
 * that can be docked in the ribbon or detached as a floating window.
 */
class IRibbonPanel {
public:
    virtual ~IRibbonPanel() = default;
    
    /**
     * Get the display name for this panel.
     * Used as the window title when the panel is floating.
     * @return Display name (e.g., "File", "View")
     */
    virtual const char* GetPanelName() const = 0;
    
    /**
     * Get the unique ImGui window ID for this panel.
     * Should include ### prefix for stable ID (e.g., "File###RibbonFile")
     * @return Unique window ID
     */
    virtual const char* GetPanelID() const = 0;
    
    /**
     * Render the panel's content.
     * Called between ImGui::Begin() and ImGui::End() by RibbonController.
     */
    virtual void Render() = 0;
};

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor
