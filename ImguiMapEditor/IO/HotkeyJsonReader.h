#pragma once

#include <filesystem>
#include <string>

namespace MapEditor {
namespace Services {
class HotkeyRegistry;
}
}

namespace MapEditor {
namespace IO {

/**
 * Reads and writes hotkey configuration from/to JSON files.
 * 
 * Follows IO pattern like SpawnXmlReader, OtbmWriter.
 */
class HotkeyJsonReader {
public:
    /**
     * Load hotkey bindings from a JSON file into the registry.
     * @return true on success, false on error (registry unchanged on error)
     */
    static bool load(const std::filesystem::path& path, Services::HotkeyRegistry& registry);
    
    /**
     * Save hotkey bindings from the registry to a JSON file.
     * @return true on success
     */
    static bool save(const std::filesystem::path& path, const Services::HotkeyRegistry& registry);
    
    /**
     * Convert key name string to GLFW key code.
     * @param name Key name like "A", "Ctrl", "PageUp", "F1"
     * @return GLFW key code, or -1 if unknown
     */
    static int parseKeyName(const std::string& name);
    
    /**
     * Convert modifier name to GLFW modifier bit.
     */
    static int parseModifier(const std::string& mod);
};

} // namespace IO
} // namespace MapEditor
