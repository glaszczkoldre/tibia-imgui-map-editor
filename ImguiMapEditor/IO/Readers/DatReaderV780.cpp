#include "DatReaderV780.h"
#include "../Flags/CanonicalFlags.h"
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace IO {

using namespace CanonicalFlags;

/**
 * Transform V780 (7.80-8.54) raw flag to canonical flag.
 * Based on RME's loadSpriteMetadataFlags() for DAT_FORMAT_78.
 * 
 * In 7.80-8.54:
 * - Flag 8 = Chargeable (inserted)
 * - Flags > 8 need -1 adjustment
 */
uint8_t DatReaderV780::transformFlag(uint8_t raw) {
    if (raw == 8) {
        return CHARGEABLE;
    } else if (raw > 8) {
        return raw - 1;
    }
    return raw;
}

} // namespace IO
} // namespace MapEditor
