#pragma once

#include <cstdint>

namespace MapEditor {

// Forward declarations
namespace Domain {
    class ChunkedMap;
}
namespace Services {
    class ViewSettings;
}

namespace Rendering {

class RenderState;
struct AnimationTicks;

/**
 * Abstract interface for map rendering.
 * 
 * Enables:
 * - Dependency injection for testability
 * - Future Vulkan migration (IRenderer -> VulkanRenderer)
 * - Decoupling UI from rendering implementation
 */
class IRenderer {
public:
    virtual ~IRenderer() = default;
    
    /**
     * Initialize the renderer's GPU resources.
     * @return true if successful
     */
    virtual bool initialize() = 0;
    
    /**
     * Render the map to a texture.
     * @param map The map to render
     * @param state Per-session render state (cache, lighting, overlays)
     * @param viewport_width Width of the render viewport
     * @param viewport_height Height of the render viewport
     * @param anim_ticks Animation timing state
     */
    virtual void render(const Domain::ChunkedMap& map, 
                       RenderState& state,
                       int viewport_width, int viewport_height,
                       const AnimationTicks& anim_ticks) = 0;
    
    /**
     * Get the output texture ID for display.
     * @return OpenGL texture ID
     */
    virtual uint32_t getTextureId() const = 0;
    
    /**
     * Set camera position in map coordinates.
     */
    virtual void setCameraPosition(float x, float y) = 0;
    
    /**
     * Set zoom level.
     */
    virtual void setZoom(float zoom) = 0;
    
    /**
     * Set the current floor to render.
     */
    virtual void setFloor(int floor) = 0;
    
    /**
     * Connect view settings for rendering options.
     */
    virtual void setViewSettings(Services::ViewSettings* settings) = 0;
    
    // Performance metrics
    virtual int getLastDrawCallCount() const = 0;
    virtual int getLastSpriteCount() const = 0;
};

} // namespace Rendering
} // namespace MapEditor
