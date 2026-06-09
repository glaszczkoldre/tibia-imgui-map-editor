/**
 * @file CarpetLookupService.cpp
 * @brief Implementation of carpet lookup table initialization.
 */

#include "CarpetLookupService.h"

namespace MapEditor {
namespace Services {
namespace Brushes {

CarpetLookupService::CarpetLookupService() { initializeTable(); }

uint32_t CarpetLookupService::getCarpetTypes(TileNeighbor neighbors) const noexcept {
  return table_[static_cast<uint8_t>(neighbors)];
}

std::vector<EdgeType> CarpetLookupService::unpack(uint32_t packed) noexcept {
  return ::MapEditor::Brushes::unpackEdgeTypes(packed);
}

// Include the auto-generated lookup table
#include "CarpetLookupTable.inc"

} // namespace Brushes
} // namespace Services
} // namespace MapEditor
