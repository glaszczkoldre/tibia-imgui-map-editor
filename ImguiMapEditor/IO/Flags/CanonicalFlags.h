#pragma once

#include <cstdint>

namespace MapEditor {
namespace IO {

/**
 * Canonical flag values (8.60+ format as baseline)
 * 
 * All version-specific readers transform their raw flag bytes to these
 * canonical values using runtime transformation functions. This matches
 * the approach used in RME's loadSpriteMetadataFlags().
 */
namespace CanonicalFlags {
    // Basic properties
    constexpr uint8_t GROUND = 0;
    constexpr uint8_t GROUND_BORDER = 1;
    constexpr uint8_t ON_BOTTOM = 2;
    constexpr uint8_t ON_TOP = 3;
    constexpr uint8_t CONTAINER = 4;
    constexpr uint8_t STACKABLE = 5;
    constexpr uint8_t FORCE_USE = 6;
    constexpr uint8_t MULTI_USE = 7;
    constexpr uint8_t WRITABLE = 8;
    constexpr uint8_t WRITABLE_ONCE = 9;
    constexpr uint8_t FLUID_CONTAINER = 10;
    constexpr uint8_t FLUID = 11;
    constexpr uint8_t UNPASSABLE = 12;
    constexpr uint8_t UNMOVEABLE = 13;
    constexpr uint8_t BLOCK_MISSILE = 14;
    constexpr uint8_t BLOCK_PATHFINDER = 15;
    constexpr uint8_t PICKUPABLE = 16;
    constexpr uint8_t HANGABLE = 17;
    constexpr uint8_t HOOK_SOUTH = 18;
    constexpr uint8_t HOOK_EAST = 19;
    constexpr uint8_t ROTATABLE = 20;
    constexpr uint8_t HAS_LIGHT = 21;
    constexpr uint8_t DONT_HIDE = 22;
    constexpr uint8_t TRANSLUCENT = 23;
    constexpr uint8_t HAS_OFFSET = 24;
    constexpr uint8_t HAS_ELEVATION = 25;
    constexpr uint8_t LYING_OBJECT = 26;
    constexpr uint8_t ANIMATE_ALWAYS = 27;
    constexpr uint8_t MINI_MAP = 28;
    constexpr uint8_t LENS_HELP = 29;
    constexpr uint8_t FULL_GROUND = 30;
    constexpr uint8_t IGNORE_LOOK = 31;
    
    // Extended flags (8.60+)
    constexpr uint8_t CLOTH = 32;
    constexpr uint8_t MARKET_ITEM = 33;
    constexpr uint8_t DEFAULT_ACTION = 34;
    constexpr uint8_t WRAPPABLE = 35;
    constexpr uint8_t UNWRAPPABLE = 36;
    constexpr uint8_t TOP_EFFECT = 37;
    
    // 10.50+ additions
    constexpr uint8_t NPC_SALE_DATA = 38;
    constexpr uint8_t CHANGER = 39;
    constexpr uint8_t PODIUM = 40;
    constexpr uint8_t USABLE = 41;
    
    // Special flags (high values to avoid collision)
    constexpr uint8_t FLOOR_CHANGE = 252;
    constexpr uint8_t NO_MOVE_ANIMATION = 253;
    constexpr uint8_t CHARGEABLE = 254;
    constexpr uint8_t LAST = 255;
}

} // namespace IO
} // namespace MapEditor
