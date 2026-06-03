#pragma once
#include "Outfit.h"
#include <string>

namespace MapEditor {
namespace Domain {

struct CreatureType {
    std::string name;
    bool is_npc = false;
    Outfit outfit;

    // Light properties (from DAT or creatures.xml)
    uint8_t light_level = 0;    // 0 = no light, >0 = light radius
    uint8_t light_color = 0;    // 8-bit Tibia palette index
    
    // Default constructor
    CreatureType() = default;
};

} // namespace Domain
} // namespace MapEditor
