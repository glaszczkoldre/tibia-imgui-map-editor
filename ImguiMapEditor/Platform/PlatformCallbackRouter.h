#pragma once

#include <functional>
#include <cstdint>

struct GLFWwindow;

namespace MapEditor {

namespace AppLogic {
    class HotkeyController;
}

namespace Platform {

/**
 * Routes GLFW input callbacks to application handlers.
 * 
 * Single responsibility: platform input callback management.
 * Chains callbacks with ImGui to ensure both systems receive input.
 */
class PlatformCallbackRouter {
public:
    using KeyCallback = void(*)(GLFWwindow*, int, int, int, int);
    using EditorStateGetter = std::function<bool()>;

    /**
     * Initialize callback routing.
     * @param window GLFW window
     * @param hotkey_controller Controller to receive hotkey events
     * @param is_editor_state Function to check if in editor state
     */
    void initialize(GLFWwindow* window, 
                    AppLogic::HotkeyController* hotkey_controller,
                    EditorStateGetter is_editor_state);

private:
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    // Static instance data (GLFW uses C callbacks)
    static PlatformCallbackRouter* s_instance;
    
    GLFWwindow* window_ = nullptr;
    KeyCallback imgui_key_callback_ = nullptr;
    AppLogic::HotkeyController* hotkey_controller_ = nullptr;
    EditorStateGetter is_editor_state_;
};

} // namespace Platform
} // namespace MapEditor
