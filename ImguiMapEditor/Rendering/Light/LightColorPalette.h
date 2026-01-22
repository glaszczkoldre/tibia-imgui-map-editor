#pragma once

#include <cstdint>
#include <array>

namespace MapEditor {
namespace Rendering {

/**
 * Tibia's 216-color palette conversion.
 * Converts 8-bit color indices (0-215) to RGB values.
 * 
 * The palette is a 6×6×6 color cube where each channel
 * has 6 possible values: 0, 51, 102, 153, 204, 255.
 */
class LightColorPalette {
public:
    /**
     * Convert an 8-bit palette index to RGB values.
     * @param color_index 0-215 palette index
     * @param out_r Output red component (0-255)
     * @param out_g Output green component (0-255)
     * @param out_b Output blue component (0-255)
     */
    static void from8bit(uint8_t color_index, uint8_t& out_r, uint8_t& out_g, uint8_t& out_b) {
        if (color_index >= 216) {
            out_r = out_g = out_b = 0;
            return;
        }
        
        // 6×6×6 color cube
        // r = (color / 36) % 6 * 51
        // g = (color / 6) % 6 * 51
        // b = color % 6 * 51
        out_r = static_cast<uint8_t>((color_index / 36) % 6 * 51);
        out_g = static_cast<uint8_t>((color_index / 6) % 6 * 51);
        out_b = static_cast<uint8_t>(color_index % 6 * 51);
    }
    
    /**
     * Convert an 8-bit palette index to RGB float values (0.0-1.0).
     * @param color_index 0-215 palette index
     * @param out_r Output red component (0.0-1.0)
     * @param out_g Output green component (0.0-1.0)
     * @param out_b Output blue component (0.0-1.0)
     */
    static void from8bitFloat(uint8_t color_index, float& out_r, float& out_g, float& out_b) {
        uint8_t r, g, b;
        from8bit(color_index, r, g, b);
        out_r = r / 255.0f;
        out_g = g / 255.0f;
        out_b = b / 255.0f;
    }
};

} // namespace Rendering
} // namespace MapEditor
