#pragma once

#include <string>
#include <cstdint>
#include "Position.h"
namespace MapEditor {
namespace Domain {

struct Town {
    uint32_t id;
    std::string name;
    Position temple_position;

    Town(uint32_t id = 0, std::string name = "", Position pos = Position())
        : id(id), name(std::move(name)), temple_position(pos) {}
};

} // namespace Domain
} // namespace MapEditor
