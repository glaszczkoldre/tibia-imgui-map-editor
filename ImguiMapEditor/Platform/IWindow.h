#pragma once

#include <cstdint>

namespace MapEditor {
namespace Platform {

/**
 * Abstract window interface for platform abstraction.
 * 
 * This interface decouples the application from specific windowing
 * libraries (GLFW, SDL, etc.) allowing for:
 * - Unit testing with mock windows
 * - Future platform migration
 * - Clean separation of concerns
 */
class IWindow {
public:
    virtual ~IWindow() = default;
    
    /**
     * Initialize the window and graphics context.
     * @param width Initial window width
     * @param height Initial window height  
     * @param title Window title
     * @return true if successful
     */
    virtual bool initialize(int width, int height, const char* title) = 0;
    
    /**
     * Shutdown and release all resources.
     */
    virtual void shutdown() = 0;
    
    /**
     * Poll for window/input events.
     */
    virtual void pollEvents() = 0;
    
    /**
     * Swap front/back buffers.
     */
    virtual void swapBuffers() = 0;
    
    /**
     * Check if window should close.
     */
    virtual bool shouldClose() const = 0;
    
    /**
     * Get window size in screen coordinates.
     */
    virtual void getSize(int& width, int& height) const = 0;
    
    /**
     * Get framebuffer size in pixels (may differ from window size on HiDPI).
     */
    virtual void getFramebufferSize(int& width, int& height) const = 0;
    
    /**
     * Get native window handle for ImGui/other libraries.
     */
    virtual void* getNativeHandle() const = 0;
    
    /**
     * Get OpenGL major version.
     */
    virtual int getGLVersionMajor() const = 0;
    
    /**
     * Get OpenGL minor version.
     */
    virtual int getGLVersionMinor() const = 0;
};

} // namespace Platform
} // namespace MapEditor
