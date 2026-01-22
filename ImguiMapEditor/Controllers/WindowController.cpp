#include "Controllers/WindowController.h"
#include "Core/Config.h"
#include <thread>
#include <chrono>

namespace MapEditor {
namespace Core {

bool WindowController::initialize(const Services::ConfigService& config) {
    int width = config.getWindowWidth();
    int height = config.getWindowHeight();

    // Default fallback if config returns 0/invalid (though ConfigService usually handles defaults)
    if (width <= 0) width = Config::Window::DEFAULT_WIDTH;
    if (height <= 0) height = Config::Window::DEFAULT_HEIGHT;

    if (!window_.initialize(width, height, "Tibia Map Editor")) {
        return false;
    }

    // Apply maximized state from config
    if (config.getWindowMaximized()) {
        window_.setMaximized(true);
    }

    return true;
}

void WindowController::saveState(Services::ConfigService& config) const {
    int width = 0;
    int height = 0;
    bool is_maximized = window_.isMaximized();

    window_.getSize(width, height);
    config.setWindowState(width, height, is_maximized);
}

void WindowController::shutdown() {
    window_.shutdown();
}

bool WindowController::update() {
    window_.pollEvents();

    // Handle display error (e.g., monitor went to sleep)
    if (window_.hasDisplayError()) {
        // Try to recover - if display is back, continue normally
        if (!window_.tryRecoverDisplay()) {
            // Display still unavailable - sleep briefly and skip this frame
            std::this_thread::sleep_for(std::chrono::milliseconds(Config::Window::DISPLAY_RECOVERY_DELAY_MS));
            return false;
        }
    }

    return true;
}

} // namespace Core
} // namespace MapEditor
