/**
 * @file CarpetLookupService.cpp
 * @brief Implementation of carpet lookup table initialization.
 */

#include "CarpetLookupService.h"

namespace MapEditor {
namespace Services {
namespace Brushes {

CarpetLookupService::CarpetLookupService() { initializeTable(); }

uint32_t CarpetLookupService::getCarpetTypes(TileNeighbor neighbors) const {
  return table_[static_cast<uint8_t>(neighbors)];
}

std::vector<EdgeType> CarpetLookupService::unpack(uint32_t packed) {
  std::vector<EdgeType> result;
  result.reserve(4);

  for (int i = 0; i < 4; ++i) {
    auto type = static_cast<EdgeType>((packed >> (i * 8)) & 0xFF);
    if (type != EdgeType::None) {
      result.push_back(type);
    }
  }
  return result;
}

// Include the auto-generated lookup table
#include "CarpetLookupTable.inc"

} // namespace Brushes
} // namespace Services
} // namespace MapEditor
