#pragma once

#include <vector>
#include <cstdint>

namespace MapEditor {
namespace Rendering {

/**
 * Manages the light map texture.
 * 
 * Computes per-tile light values on CPU and uploads to GPU.
 * The texture is then used for multiply-blend overlay.
 */
class LightTexture {
public:
    LightTexture();
    ~LightTexture();
    
    // Non-copyable
    LightTexture(const LightTexture&) = delete;
    LightTexture& operator=(const LightTexture&) = delete;
    
    /**
     * Initialize OpenGL resources.
     * @return true if initialization succeeded
     */
    bool initialize();
    
    /**
     * Upload pre-computed light data to GPU texture.
     * @param buffer RGBA pixel data
     * @param width Width in pixels
     * @param height Height in pixels
     */
    void upload(const std::vector<uint8_t>& buffer, int width, int height);
    
    /**
     * Get the OpenGL texture ID.
     */
    uint32_t getTextureId() const { return texture_id_; }
    
    /**
     * Get texture dimensions.
     */
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    uint32_t texture_id_ = 0;
    int width_ = 0;
    int height_ = 0;
    std::vector<uint8_t> buffer_;  // RGBA buffer
    bool initialized_ = false;
};

} // namespace Rendering
} // namespace MapEditor
