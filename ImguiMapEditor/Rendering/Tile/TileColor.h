#pragma once

namespace MapEditor {
namespace Rendering {

/**
 * RGB color values for tile rendering (0.0 to 1.0).
 */
struct TileColor {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    
    TileColor() = default;
    TileColor(float r_, float g_, float b_) : r(r_), g(g_), b(b_) {}
};

} // namespace Rendering
} // namespace MapEditor
