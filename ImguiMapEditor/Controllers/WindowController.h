#pragma once
#include "Platform/GlfwWindow.h"
#include "Services/ConfigService.h"
namespace MapEditor {
namespace Core {

/**
 * Controller for the main application window.
 * Handles window initialization, configuration persistence, event polling,
 * and display error recovery.
 */
class WindowController {
public:
    WindowController() = default;

    // Non-copyable/movable (holds GlfwWindow)
    WindowController(const WindowController&) = delete;
    WindowController& operator=(const WindowController&) = delete;

    /**
     * Initialize the window using configuration settings.
     * @return true if successful
     */
    bool initialize(const Services::ConfigService& config);

    /**
     * Save window state (size, maximized) to configuration.
     */
    void saveState(Services::ConfigService& config) const;

    /**
     * Shut down the window logic.
     */
    void shutdown();

    /**
     * Process events and handle display recovery.
     * @return false if the frame should be skipped (e.g. display error/recovery), true to continue.
     */
    bool update();

    /**
     * Get the underlying GLFW window wrapper.
     */
    Platform::GlfwWindow& getWindow() { return window_; }
    const Platform::GlfwWindow& getWindow() const { return window_; }

    /**
     * Check if the window should close (user requested).
     */
    bool shouldClose() const { return window_.shouldClose(); }

private:
    Platform::GlfwWindow window_;
};

} // namespace Core
} // namespace MapEditor
