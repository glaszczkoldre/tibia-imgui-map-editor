#include "LightTexture.h"
#include <glad/glad.h>
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace Rendering {

LightTexture::LightTexture() = default;

LightTexture::~LightTexture() {
    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }
}

bool LightTexture::initialize() {
    if (initialized_) return true;
    
    glGenTextures(1, &texture_id_);
    if (texture_id_ == 0) return false;
    
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    initialized_ = true;
    return true;
}

void LightTexture::upload(const std::vector<uint8_t>& buffer, int width, int height) {
    if (!initialized_ || texture_id_ == 0) return;
    if (buffer.empty() || width <= 0 || height <= 0) return;
    
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    if (width == width_ && height == height_) {
        // Optimized path: No reallocation, just update content
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                        GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());
    } else {
        // Resize path: Reallocate storage
        width_ = width;
        height_ = height;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());
        spdlog::debug("LightTexture: Reallocated to {}x{}", width, height);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace Rendering
} // namespace MapEditor
