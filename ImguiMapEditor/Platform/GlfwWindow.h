#pragma once
#include "IWindow.h"
struct GLFWwindow;

namespace MapEditor {
namespace Platform {

/**
 * GLFW-based implementation of IWindow.
 * 
 * Handles GLFW initialization, window creation, OpenGL context setup,
 * and cleanup. This class owns the GLFW window and is non-copyable.
 * 
 * NOTE: Only one GlfwWindow instance is supported due to GLFW's
 * static error callback requirement. Creating multiple instances
 * will trigger an assertion failure.
 */
class GlfwWindow : public IWindow {
public:
    GlfwWindow();  // Asserts that no other instance exists
    ~GlfwWindow() override;
    
    // Non-copyable, non-movable (owns GLFW state)
    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;
    GlfwWindow(GlfwWindow&&) = delete;
    GlfwWindow& operator=(GlfwWindow&&) = delete;
    
    bool initialize(int width, int height, const char* title) override;
    void shutdown() override;
    
    void pollEvents() override;
    void swapBuffers() override;
    bool shouldClose() const override;
    
    void getSize(int& width, int& height) const override;
    void getFramebufferSize(int& width, int& height) const override;
    
    void* getNativeHandle() const override { return window_; }
    
    int getGLVersionMajor() const override { return gl_version_major_; }
    int getGLVersionMinor() const override { return gl_version_minor_; }
    
    // Display error recovery
    bool hasDisplayError() const { return display_error_; }
    void clearDisplayError() { display_error_ = false; }
    bool tryRecoverDisplay();
    
    // Window maximize state
    bool isMaximized() const;
    void setMaximized(bool maximized);
    
private:
    bool initializeGLFW(int width, int height, const char* title);
    bool initializeOpenGL();
    
    GLFWwindow* window_ = nullptr;
    int gl_version_major_ = 3;
    int gl_version_minor_ = 3;
    bool display_error_ = false;
    
    // Static callback needs access to instance
    static GlfwWindow* s_instance_;
};

} // namespace Platform
} // namespace MapEditor
