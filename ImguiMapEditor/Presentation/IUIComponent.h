#pragma once

namespace MapEditor {
namespace Presentation {

/**
 * Abstract interface for UI components.
 * 
 * Enables:
 * - Consistent lifecycle management for all UI elements
 * - Dependency injection for testability
 * - Standardized visibility control
 */
class IUIComponent {
public:
    virtual ~IUIComponent() = default;
    
    /**
     * Render the component.
     * Called every frame when the component is visible.
     */
    virtual void render() = 0;
    
    /**
     * Set visibility state.
     * @param visible true to show, false to hide
     */
    virtual void setVisible(bool visible) = 0;
    
    /**
     * Get visibility state.
     * @return true if visible
     */
    virtual bool isVisible() const = 0;
};

} // namespace Presentation
} // namespace MapEditor
