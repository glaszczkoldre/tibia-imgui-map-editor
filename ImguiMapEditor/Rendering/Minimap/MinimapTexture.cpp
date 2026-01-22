#include "MinimapTexture.h"
#include <glad/glad.h>
#include <cstring>

namespace MapEditor {
namespace Rendering {

MinimapTexture::~MinimapTexture() {
    destroy();
}

MinimapTexture::MinimapTexture(MinimapTexture&& other) noexcept
    : texture_id_(other.texture_id_)
    , width_(other.width_)
    , height_(other.height_) {
    other.texture_id_ = 0;
    other.width_ = 0;
    other.height_ = 0;
}

MinimapTexture& MinimapTexture::operator=(MinimapTexture&& other) noexcept {
    if (this != &other) {
        destroy();
        texture_id_ = other.texture_id_;
        width_ = other.width_;
        height_ = other.height_;
        other.texture_id_ = 0;
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

void MinimapTexture::create(int width, int height) {
    destroy();
    
    width_ = width;
    height_ = height;
    
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    
    // Set texture parameters for pixel-perfect rendering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Allocate texture storage (initialized to transparent)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void MinimapTexture::updateRegion(int x, int y, int w, int h, const uint32_t* data) {
    if (!isValid() || !data) return;
    
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h,
                    GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void MinimapTexture::updateFull(const uint32_t* data) {
    if (!isValid() || !data) return;
    
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_,
                    GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void MinimapTexture::clear() {
    if (!isValid()) return;
    
    std::vector<uint32_t> clear_data(width_ * height_, 0);
    updateFull(clear_data.data());
}

void MinimapTexture::destroy() {
    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }
    width_ = 0;
    height_ = 0;
}

} // namespace Rendering
} // namespace MapEditor
