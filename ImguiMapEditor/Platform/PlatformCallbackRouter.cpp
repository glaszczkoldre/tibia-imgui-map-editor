#include "PlatformCallbackRouter.h"
#include "Controllers/HotkeyController.h"
#include <GLFW/glfw3.h>

namespace MapEditor {
namespace Platform {

PlatformCallbackRouter* PlatformCallbackRouter::s_instance = nullptr;

void PlatformCallbackRouter::initialize(GLFWwindow* window,
                                         AppLogic::HotkeyController* hotkey_controller,
                                         EditorStateGetter is_editor_state) {
    window_ = window;
    hotkey_controller_ = hotkey_controller;
    is_editor_state_ = std::move(is_editor_state);
    
    s_instance = this;
    
    // Get ImGui's callback (it was installed during ImGui backend init)
    imgui_key_callback_ = glfwSetKeyCallback(window, nullptr);
    
    // Set our callback, which will chain to ImGui's
    glfwSetKeyCallback(window, &PlatformCallbackRouter::keyCallback);
}

void PlatformCallbackRouter::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (!s_instance) return;
    
    // 1. Call ImGui callback first
    if (s_instance->imgui_key_callback_) {
        s_instance->imgui_key_callback_(window, key, scancode, action, mods);
    }
    
    // 2. Process our hotkeys on press or repeat
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (s_instance->hotkey_controller_) {
            bool in_editor = s_instance->is_editor_state_ ? s_instance->is_editor_state_() : false;
            s_instance->hotkey_controller_->processKey(key, mods, in_editor);
        }
    }
}

} // namespace Platform
} // namespace MapEditor
