#pragma once

#include <cstdint>
#include <vector>

namespace MapEditor {
namespace Rendering {

/**
 * RAII wrapper for OpenGL texture used by minimap.
 * Manages a 2D RGBA texture where each pixel represents one tile.
 */
class MinimapTexture {
public:
    MinimapTexture() = default;
    ~MinimapTexture();
    
    // No copy
    MinimapTexture(const MinimapTexture&) = delete;
    MinimapTexture& operator=(const MinimapTexture&) = delete;
    
    // Move
    MinimapTexture(MinimapTexture&& other) noexcept;
    MinimapTexture& operator=(MinimapTexture&& other) noexcept;
    
    /**
     * Create texture with given dimensions
     */
    void create(int width, int height);
    
    /**
     * Update a region of the texture
     * @param x X offset in texture
     * @param y Y offset in texture
     * @param w Width of region
     * @param h Height of region
     * @param data RGBA pixel data (w * h * 4 bytes)
     */
    void updateRegion(int x, int y, int w, int h, const uint32_t* data);
    
    /**
     * Update entire texture from pixel buffer
     */
    void updateFull(const uint32_t* data);
    
    /**
     * Clear texture to transparent
     */
    void clear();
    
    /**
     * Get OpenGL texture ID for rendering
     */
    uint32_t getTextureId() const { return texture_id_; }
    
    /**
     * Check if texture is valid
     */
    bool isValid() const { return texture_id_ != 0; }
    
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    
    /**
     * Destroy the texture
     */
    void destroy();

private:
    uint32_t texture_id_ = 0;
    int width_ = 0;
    int height_ = 0;
};

} // namespace Rendering
} // namespace MapEditor
