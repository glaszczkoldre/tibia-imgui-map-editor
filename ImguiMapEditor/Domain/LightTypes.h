#pragma once

#include <cstdint>

namespace MapEditor {
namespace Domain {

/**
 * A light source extracted from a tile item.
 * Contains position, color (8-bit palette index), and intensity.
 */
struct LightSource {
    int32_t x = 0;              // Tile X position
    int32_t y = 0;              // Tile Y position
    uint8_t color = 215;        // 8-bit palette index (default: white-ish)
    uint8_t intensity = 0;      // 0-255 (affects light radius)
};

/**
 * Configuration for a viewport's lighting system.
 * Each viewport (Map, Ingame Preview) has its own independent config.
 */
struct LightConfig {
    bool enabled = false;           // Master enable for this viewport
    uint8_t ambient_level = 255;    // 0 = complete darkness, 255 = full bright
    uint8_t ambient_color = 215;    // 8-bit palette index for ambient
};

} // namespace Domain
} // namespace MapEditor
