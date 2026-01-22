#include "Rendering/Resources/ShaderLoader.h"
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace Rendering {

// Default to data/shaders directory (will be set by Application)
std::filesystem::path ShaderLoader::shader_directory_ = "data/shaders";

std::unique_ptr<Shader> ShaderLoader::loadFromFiles(
    const std::filesystem::path& vertex_path,
    const std::filesystem::path& fragment_path)
{
    std::string vertex_source = readFile(vertex_path);
    if (vertex_source.empty()) {
        spdlog::error("Failed to read vertex shader: {}", vertex_path.string());
        return nullptr;
    }
    
    std::string fragment_source = readFile(fragment_path);
    if (fragment_source.empty()) {
        spdlog::error("Failed to read fragment shader: {}", fragment_path.string());
        return nullptr;
    }
    
    auto shader = std::make_unique<Shader>(vertex_source, fragment_source);
    if (!shader->isValid()) {
        spdlog::error("Failed to compile shader from {} and {}: {}", 
                      vertex_path.string(), fragment_path.string(),
                      shader->getError());
        return nullptr;
    }
    
    spdlog::info("Loaded shader: {} + {}", 
                 vertex_path.filename().string(), 
                 fragment_path.filename().string());
    return shader;
}

std::unique_ptr<Shader> ShaderLoader::load(const std::string& shader_name)
{
    std::filesystem::path vertex_path = shader_directory_ / (shader_name + ".vert");
    std::filesystem::path fragment_path = shader_directory_ / (shader_name + ".frag");
    
    return loadFromFiles(vertex_path, fragment_path);
}

std::string ShaderLoader::readFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        spdlog::error("Cannot open file: {}", path.string());
        return "";
    }
    
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

void ShaderLoader::setShaderDirectory(const std::filesystem::path& path)
{
    shader_directory_ = path;
    spdlog::info("Shader directory set to: {}", path.string());
}

const std::filesystem::path& ShaderLoader::getShaderDirectory()
{
    return shader_directory_;
}

} // namespace Rendering
} // namespace MapEditor
