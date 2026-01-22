/**
 * @file WallLookupService.cpp
 * @brief Implementation of wall lookup table initialization.
 */

#include "WallLookupService.h"

namespace MapEditor {
namespace Services {
namespace Brushes {

WallLookupService::WallLookupService() { initializeTable(); }

WallAlign WallLookupService::getFullType(WallNeighbor neighbors) const {
  return fullTable_[static_cast<uint8_t>(neighbors) & 0x0F];
}

WallAlign WallLookupService::getHalfType(WallNeighbor neighbors) const {
  return halfTable_[static_cast<uint8_t>(neighbors) & 0x0F];
}

// Include the auto-generated lookup table
#include "WallLookupTable.inc"

} // namespace Brushes
} // namespace Services
} // namespace MapEditor
