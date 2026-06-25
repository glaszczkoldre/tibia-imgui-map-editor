/**
 * @file CarpetLookupService.cpp
 * @brief Implementation of carpet lookup table initialization.
 */

#include "CarpetLookupService.h"

namespace MapEditor {
namespace Services {
namespace Brushes {

#include "CarpetLookupTable.inc"

CarpetLookupService::CarpetLookupService() = default;

uint32_t CarpetLookupService::getCarpetTypes(TileNeighbor neighbors) const noexcept {
  return kCarpetLookupTable[static_cast<uint8_t>(neighbors)];
}

std::vector<EdgeType> CarpetLookupService::unpack(uint32_t packed) noexcept {
  return ::MapEditor::Brushes::unpackEdgeTypes(packed);
}

} // namespace Brushes
} // namespace Services
} // namespace MapEditor
