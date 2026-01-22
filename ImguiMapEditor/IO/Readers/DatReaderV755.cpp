#include "DatReaderV755.h"
#include "../Flags/CanonicalFlags.h"
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace IO {

using namespace CanonicalFlags;

/**
 * Transform V755 (7.55-7.72) raw flag to canonical flag.
 * Based on RME's loadSpriteMetadataFlags() for DAT_FORMAT_755.
 * 
 * In 7.55-7.72:
 * - Attribute 23 is "Floor Change"
 */
uint8_t DatReaderV755::transformFlag(uint8_t raw) {
    if (raw == 23) {
        return FLOOR_CHANGE;
    }
    return raw;
}

} // namespace IO
} // namespace MapEditor
