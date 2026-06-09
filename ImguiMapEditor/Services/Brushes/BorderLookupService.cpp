/**
 * @file BorderLookupService.cpp
 * @brief Implementation of border lookup table initialization and utilities.
 */

#include "BorderLookupService.h"

namespace MapEditor {
namespace Services {
namespace Brushes {

BorderLookupService::BorderLookupService() { initializeTable(); }

uint32_t BorderLookupService::getBorderTypes(TileNeighbor neighbors) const noexcept {
  return table_[static_cast<uint8_t>(neighbors)];
}

std::vector<EdgeType> BorderLookupService::unpack(uint32_t packed) noexcept {
  return ::MapEditor::Brushes::unpackEdgeTypes(packed);
}

// Include the auto-generated lookup table
#include "BorderLookupTable.inc"

} // namespace Brushes
} // namespace Services
} // namespace MapEditor
