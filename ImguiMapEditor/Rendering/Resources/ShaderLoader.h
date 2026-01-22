#pragma once
#include "Rendering/Core/Shader.h"
#include <filesystem>
#include <memory>
#include <string>

namespace MapEditor {
namespace Rendering {

/**
 * Utility for loading GLSL shaders from external files.
 * 
 * Benefits:
 * - Shaders can be edited without recompiling
 * - Cleaner C++ code without embedded string literals
 * - Easier shader debugging and profiling
 * - Hot-reloading support possible in future
 */
class ShaderLoader {
public:
    /**
     * Load a shader from vertex and fragment files.
     * @param vertex_path Path to .vert file
     * @param fragment_path Path to .frag file
     * @return Shader if successful, nullptr on error
     */
    static std::unique_ptr<Shader> loadFromFiles(
        const std::filesystem::path& vertex_path,
        const std::filesystem::path& fragment_path);
    
    /**
     * Load a shader with automatic path resolution.
     * Looks for shaders in the data/shaders directory.
     * @param shader_name Base name without extension (e.g., "sprite_batch")
     * @return Shader if successful, nullptr on error
     */
    static std::unique_ptr<Shader> load(const std::string& shader_name);
    
    /**
     * Read entire file contents as string.
     * @param path Path to file
     * @return File contents, empty string on error
     */
    static std::string readFile(const std::filesystem::path& path);
    
    /**
     * Set the base directory for shader files.
     * @param path Directory containing shader files
     */
    static void setShaderDirectory(const std::filesystem::path& path);
    
    /**
     * Get the current shader directory.
     */
    static const std::filesystem::path& getShaderDirectory();

private:
    static std::filesystem::path shader_directory_;
};

} // namespace Rendering
} // namespace MapEditor
