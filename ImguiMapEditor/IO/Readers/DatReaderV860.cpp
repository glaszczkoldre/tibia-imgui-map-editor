#include "DatReaderV860.h"
#include "../Flags/CanonicalFlags.h"
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace IO {

using namespace CanonicalFlags;

/**
 * V860 (8.60-9.86) is the canonical format - no transformation needed.
 * This is the baseline that all other versions transform to.
 */

// No overrides needed as default implementation handles canonical flags

} // namespace IO
} // namespace MapEditor
