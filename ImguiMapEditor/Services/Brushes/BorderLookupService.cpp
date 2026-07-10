/**
 * @file BorderLookupService.cpp
 * @brief Implementation of border lookup table initialization and utilities.
 */

#include "BorderLookupService.h"

namespace MapEditor {
namespace Services {
namespace Brushes {

#include "BorderLookupTable.inc"

BorderLookupService::BorderLookupService() = default;

uint32_t BorderLookupService::getBorderTypes(TileNeighbor neighbors) const noexcept {
  return kBorderLookupTable[static_cast<uint8_t>(neighbors)];
}

std::vector<EdgeType> BorderLookupService::unpack(uint32_t packed) noexcept {
  return ::MapEditor::Brushes::unpackEdgeTypes(packed);
}

} // namespace Brushes
} // namespace Services
} // namespace MapEditor
