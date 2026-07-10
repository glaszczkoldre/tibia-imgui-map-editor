/**
 * @file WallLookupService.cpp
 * @brief Implementation of wall lookup table initialization.
 */

#include "WallLookupService.h"

namespace MapEditor {
namespace Services {
namespace Brushes {

#include "WallLookupTable.inc"

WallLookupService::WallLookupService() = default;

WallAlign WallLookupService::getFullType(WallNeighbor neighbors) const noexcept {
  return kFullWallLookupTable[static_cast<uint8_t>(neighbors) & 0x0F];
}

WallAlign WallLookupService::getHalfType(WallNeighbor neighbors) const noexcept {
  return kHalfWallLookupTable[static_cast<uint8_t>(neighbors) & 0x0F];
}

} // namespace Brushes
} // namespace Services
} // namespace MapEditor
