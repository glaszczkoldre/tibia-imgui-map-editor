#pragma once

#include <cstdint>

namespace MapEditor {
namespace Domain {

struct Outfit {
    uint16_t lookType = 0;
    uint16_t lookItem = 0;      // Item ID for item-based appearances
    uint16_t lookHead = 0;
    uint16_t lookBody = 0;
    uint16_t lookLegs = 0;
    uint16_t lookFeet = 0;
    uint16_t lookAddons = 0;
    uint16_t lookMount = 0;
    // Mount colors (for RME XML compatibility)
    uint16_t lookMountHead = 0;
    uint16_t lookMountBody = 0;
    uint16_t lookMountLegs = 0;
    uint16_t lookMountFeet = 0;
};

} // namespace Domain
} // namespace MapEditor
