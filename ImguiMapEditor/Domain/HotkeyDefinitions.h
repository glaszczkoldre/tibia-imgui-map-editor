#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace MapEditor {
namespace Domain {

/**
 * Pure-data representation of a single hotkey binding.
 * Owned by Services::HotkeyRegistry at runtime.
 * Serialized/deserialized by IO::HotkeyJsonReader.
 */
struct HotkeyBinding {
    std::string action_id;
    int32_t key = 0;        // GLFW key code or mouse button
    int32_t mods = 0;       // GLFW modifier bits
    std::string category;
    bool is_mouse = false;

    bool matches(int32_t k, int32_t m) const {
        return key == k && (m & mods) == mods;
    }
};

using HotkeyBindingMap = std::unordered_map<std::string, HotkeyBinding>;

} // namespace Domain
} // namespace MapEditor
