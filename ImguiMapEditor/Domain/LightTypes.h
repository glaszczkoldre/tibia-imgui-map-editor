#pragma once

#include "Core/Config.h"

#include <cstdint>

namespace MapEditor {
namespace Domain {

/**
 * Global world light (mimics server WorldLight protocol message).
 * Defines the ambient light level for above-ground floors.
 */
struct GlobalLight {
    uint8_t intensity = 0;   // Server ambient level (0-255)
    uint8_t color = Config::Lighting::DEFAULT_SERVER_LIGHT_COLOR;
};

/**
 * A light source extracted from a tile item.
 * Contains position, color (8-bit palette index), and intensity.
 */
struct LightSource {
    int32_t x = 0;              // Tile X position
    int32_t y = 0;              // Tile Y position
    uint8_t color = Config::Lighting::DEFAULT_SERVER_LIGHT_COLOR;
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
    uint8_t client_slider = Config::Lighting::DEFAULT_MINIMUM_AMBIENT;
    int16_t camera_floor = Config::Map::GROUND_LAYER;
    // Deprecated compatibility fields. Rendering computes final ambient from
    // global_light, client_slider, and camera_floor elsewhere.
    uint8_t ambient_color = Config::Lighting::DEFAULT_SERVER_LIGHT_COLOR;
    uint8_t ambient_level = 255;

    /**
     * Hash fields that affect generated lighting.
     */
    [[nodiscard]] uint32_t computeHash() const {
        return static_cast<uint32_t>(global_light.intensity) |
               (static_cast<uint32_t>(global_light.color) << 8) |
               (static_cast<uint32_t>(client_slider) << 16) |
               (static_cast<uint32_t>(camera_floor & 0xFF) << 24);
    }
};

} // namespace Domain
} // namespace MapEditor
