#include "DatReaderV740.h"
#include "../Flags/CanonicalFlags.h"
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace IO {

using namespace CanonicalFlags;

/**
 * Transform V740 (7.40-7.54) raw flag to canonical flag.
 * Based on RME's loadSpriteMetadataFlags() for DAT_FORMAT_74.
 * 
 * Same transformation as V710:
 * - "Ground Border" did not exist, attributes 1-15 need +1 adjustment
 * - Flags 16-28 map to different canonical positions
 * - MultiUse and ForceUse are swapped
 */
uint8_t DatReaderV740::transformFlag(uint8_t raw) {
    uint8_t flag = raw;
    
    // Shift flags 1-15 up by 1 (no GROUND_BORDER in 7.40)
    if (flag > 0 && flag <= 15) {
        flag += 1;
    } else if (flag == 16) {
        flag = HAS_LIGHT;
    } else if (flag == 17) {
        flag = FLOOR_CHANGE;
    } else if (flag == 18) {
        flag = FULL_GROUND;
    } else if (flag == 19) {
        flag = HAS_ELEVATION;
    } else if (flag == 20) {
        flag = HAS_OFFSET;
    } else if (flag == 22) {
        flag = MINI_MAP;
    } else if (flag == 23) {
        flag = ROTATABLE;
    } else if (flag == 24) {
        flag = LYING_OBJECT;
    } else if (flag == 25) {
        flag = HANGABLE;
    } else if (flag == 26) {
        flag = HOOK_SOUTH;
    } else if (flag == 27) {
        flag = HOOK_EAST;
    } else if (flag == 28) {
        flag = ANIMATE_ALWAYS;
    }
    
    // MultiUse and ForceUse are SWAPPED in 7.10-7.54
    if (flag == MULTI_USE) {
        flag = FORCE_USE;
    } else if (flag == FORCE_USE) {
        flag = MULTI_USE;
    }
    
    return flag;
}

bool DatReaderV740::handleSpecificFlag(uint8_t flag, ClientItem& item, BinaryReader& reader) {
    if (flag == HAS_OFFSET) {
        item.has_offset = true;
        // V7.40: Fixed offset, NOT read from file
        item.offset_x = 8;
        item.offset_y = 8;
        return true;
    }
    return false;
}

} // namespace IO
} // namespace MapEditor
