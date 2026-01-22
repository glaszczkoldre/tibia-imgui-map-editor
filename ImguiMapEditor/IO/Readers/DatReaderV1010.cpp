#include "DatReaderV1010.h"
#include "../Flags/CanonicalFlags.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace MapEditor {
namespace IO {

using namespace CanonicalFlags;

/**
 * Transform V1010 (10.10+) raw flag to canonical flag.
 * Based on RME's loadSpriteMetadataFlags() for DAT_FORMAT_1010.
 * 
 * In 10.10+:
 * - Flag 16 = No Movement Animation (inserted)
 * - Flags > 16 need -1 adjustment
 */
uint8_t DatReaderV1010::transformFlag(uint8_t raw) {
    if (raw == 16) {
        return NO_MOVE_ANIMATION;
    } else if (raw > 16) {
        return raw - 1;
    }
    return raw;
}

} // namespace IO
} // namespace MapEditor
