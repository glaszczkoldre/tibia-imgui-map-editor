#pragma once

#include <cstdint>

namespace MapEditor {
namespace Domain {

/**
 * Global world light (mimics server WorldLight protocol message).
 * Defines the ambient light level for above-ground floors.
 */
struct GlobalLight {
    uint8_t intensity = 0;   // Server ambient level (0-255)
    uint8_t color = 215;     // Server ambient color (8-bit palette index)
};

/**
 * A light source extracted from a tile item.
 * Contains position, color (8-bit palette index), and intensity.
 */
struct LightSource {
    int32_t x = 0;              // Tile X position
    int32_t y = 0;              // Tile Y position
    uint8_t color = 215;        // 8-bit palette index (default: white-ish)
    uint8_t intensity = 0;      // 0-255 (affects light radius)
    int16_t source_floor = 0;   // Floor this light originates from (for blocking)
};

/**
 * Configuration for a viewport's lighting system.
 * Each viewport (Map, Ingame Preview) has its own independent config.
 */
struct LightConfig {
    bool enabled = false;           // Master enable for this viewport
    GlobalLight global_light;       // Server global light (above-ground ambient)
    uint8_t client_slider = 0;     // Client minimum ambient slider (0-255), acts as floor
    int16_t camera_floor = 7;      // Current camera floor (for above/underground check)
    uint8_t ambient_color = 215;   // DEPRECATED: kept for backward compat, use global_light.color
    uint8_t ambient_level = 255;   // DEPRECATED: kept for backward compat, computed from global_light + slider
};

} // namespace Domain
} // namespace MapEditor
