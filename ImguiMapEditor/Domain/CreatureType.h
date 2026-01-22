#pragma once
#include "Outfit.h"
#include <string>

namespace MapEditor {
namespace Domain {

struct CreatureType {
    std::string name;
    bool is_npc = false;
    Outfit outfit;
    
    // Default constructor
    CreatureType() = default;
};

} // namespace Domain
} // namespace MapEditor
