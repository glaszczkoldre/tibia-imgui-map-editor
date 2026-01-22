#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include "Domain/Outfit.h"
namespace MapEditor {
namespace Rendering {

/**
 * RME's pre-computed outfit color lookup table (133 colors).
 * Format: 0xRRGGBB
 */
static constexpr uint32_t TemplateOutfitLookupTable[] = {
    0xFFFFFF, 0xFFD4BF, 0xFFE9BF, 0xFFFFBF, 0xE9FFBF, 0xD4FFBF, 0xBFFFBF,
    0xBFFFD4, 0xBFFFE9, 0xBFFFFF, 0xBFE9FF, 0xBFD4FF, 0xBFBFFF, 0xD4BFFF,
    0xE9BFFF, 0xFFBFFF, 0xFFBFE9, 0xFFBFD4, 0xFFBFBF, 0xDADADA, 0xBF9F8F,
    0xBFAF8F, 0xBFBF8F, 0xAFBF8F, 0x9FBF8F, 0x8FBF8F, 0x8FBF9F, 0x8FBFAF,
    0x8FBFBF, 0x8FAFBF, 0x8F9FBF, 0x8F8FBF, 0x9F8FBF, 0xAF8FBF, 0xBF8FBF,
    0xBF8FAF, 0xBF8F9F, 0xBF8F8F, 0xB6B6B6, 0xBF7F5F, 0xBFAF8F, 0xBFBF5F,
    0x9FBF5F, 0x7FBF5F, 0x5FBF5F, 0x5FBF7F, 0x5FBF9F, 0x5FBFBF, 0x5F9FBF,
    0x5F7FBF, 0x5F5FBF, 0x7F5FBF, 0x9F5FBF, 0xBF5FBF, 0xBF5F9F, 0xBF5F7F,
    0xBF5F5F, 0x919191, 0xBF6A3F, 0xBF943F, 0xBFBF3F, 0x94BF3F, 0x6ABF3F,
    0x3FBF3F, 0x3FBF6A, 0x3FBF94, 0x3FBFBF, 0x3F94BF, 0x3F6ABF, 0x3F3FBF,
    0x6A3FBF, 0x943FBF, 0xBF3FBF, 0xBF3F94, 0xBF3F6A, 0xBF3F3F, 0x6D6D6D,
    0xFF5500, 0xFFAA00, 0xFFFF00, 0xAAFF00, 0x54FF00, 0x00FF00, 0x00FF54,
    0x00FFAA, 0x00FFFF, 0x00A9FF, 0x0055FF, 0x0000FF, 0x5500FF, 0xA900FF,
    0xFE00FF, 0xFF00AA, 0xFF0055, 0xFF0000, 0x484848, 0xBF3F00, 0xBF7F00,
    0xBFBF00, 0x7FBF00, 0x3FBF00, 0x00BF00, 0x00BF3F, 0x00BF7F, 0x00BFBF,
    0x007FBF, 0x003FBF, 0x0000BF, 0x3F00BF, 0x7F00BF, 0xBF00BF, 0xBF007F,
    0xBF003F, 0xBF0000, 0x242424, 0x7F2A00, 0x7F5500, 0x7F7F00, 0x557F00,
    0x2A7F00, 0x007F00, 0x007F2A, 0x007F55, 0x007F7F, 0x00547F, 0x002A7F,
    0x00007F, 0x2A007F, 0x54007F, 0x7F007F, 0x7F0055, 0x7F002A, 0x7F0000
};

constexpr size_t OUTFIT_COLOR_COUNT = sizeof(TemplateOutfitLookupTable) / sizeof(TemplateOutfitLookupTable[0]);

/**
 * Get outfit color RGB from index (0-132).
 * Returns packed 0xRRGGBB value.
 */
inline uint32_t getOutfitColorRGB(uint8_t colorIndex) {
    if (colorIndex >= OUTFIT_COLOR_COUNT) return 0xFFFFFF;
    return TemplateOutfitLookupTable[colorIndex];
}

/**
 * Colorizes a pixel based on outfit color index.
 * Uses RME's exact algorithm: multiply pixel RGB by outfit color.
 */
inline void colorizePixel(uint8_t colorIndex, uint8_t& r, uint8_t& g, uint8_t& b) {
    if (colorIndex >= OUTFIT_COLOR_COUNT) colorIndex = 0;
    uint32_t color = TemplateOutfitLookupTable[colorIndex];
    uint8_t ro = (color >> 16) & 0xFF;
    uint8_t go = (color >> 8) & 0xFF;
    uint8_t bo = color & 0xFF;
    r = static_cast<uint8_t>(r * (ro / 255.0f));
    g = static_cast<uint8_t>(g * (go / 255.0f));
    b = static_cast<uint8_t>(b * (bo / 255.0f));
}

/**
 * Checks template pixel color and returns which outfit part it represents.
 * Template colors: yellow=head(0), red=body(1), green=legs(2), blue=feet(3), -1=none
 */
inline int getTemplatePartFromColor(uint8_t r, uint8_t g, uint8_t b) {
    // Template mask uses pure colors:
    // Yellow (R+G, no B) = head
    // Red (R only) = body
    // Green (G only) = legs  
    // Blue (B only) = feet
    if (r && g && !b) return 0;      // Yellow => head
    if (r && !g && !b) return 1;     // Red => body
    if (!r && g && !b) return 2;     // Green => legs
    if (!r && !g && b) return 3;     // Blue => feet
    return -1;  // Not a template pixel
}

/**
 * CPU-based outfit colorizer.
 * Takes base sprite and template mask, applies head/body/legs/feet colors.
 */
class OutfitColorizer {
public:
    /**
     * Apply outfit colors to RGBA pixel data.
     * @param basePixels Base sprite RGBA data (will be modified in-place)
     * @param templatePixels Template mask RGBA data (template layer)
     * @param pixelCount Number of pixels (width * height)
     * @param outfit Outfit with head/body/legs/feet color indices
     */
    static void colorize(uint8_t* basePixels, const uint8_t* templatePixels, 
                         size_t pixelCount, const Domain::Outfit& outfit) {
        if (!basePixels || !templatePixels) return;
        
        uint8_t lookHead = static_cast<uint8_t>(outfit.lookHead);
        uint8_t lookBody = static_cast<uint8_t>(outfit.lookBody);
        uint8_t lookLegs = static_cast<uint8_t>(outfit.lookLegs);
        uint8_t lookFeet = static_cast<uint8_t>(outfit.lookFeet);
        
        for (size_t i = 0; i < pixelCount; ++i) {
            size_t idx = i * 4;  // RGBA
            uint8_t& r = basePixels[idx + 0];
            uint8_t& g = basePixels[idx + 1];
            uint8_t& b = basePixels[idx + 2];
            // alpha at idx + 3
            
            // Template is also RGBA
            uint8_t tr = templatePixels[idx + 0];
            uint8_t tg = templatePixels[idx + 1];
            uint8_t tb = templatePixels[idx + 2];
            
            int part = getTemplatePartFromColor(tr, tg, tb);
            switch (part) {
                case 0: colorizePixel(lookHead, r, g, b); break;
                case 1: colorizePixel(lookBody, r, g, b); break;
                case 2: colorizePixel(lookLegs, r, g, b); break;
                case 3: colorizePixel(lookFeet, r, g, b); break;
                default: break;  // Not a template pixel, keep original
            }
        }
    }
};

} // namespace Rendering
} // namespace MapEditor
