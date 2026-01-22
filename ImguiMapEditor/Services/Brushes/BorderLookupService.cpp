/**
 * @file BorderLookupService.cpp
 * @brief Implementation of border lookup table initialization and utilities.
 */

#include "BorderLookupService.h"

namespace MapEditor {
namespace Services {
namespace Brushes {

BorderLookupService::BorderLookupService() { initializeTable(); }

uint32_t BorderLookupService::getBorderTypes(TileNeighbor neighbors) const {
  return table_[static_cast<uint8_t>(neighbors)];
}

std::vector<EdgeType> BorderLookupService::unpack(uint32_t packed) {
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

uint32_t BorderLookupService::pack(const std::vector<EdgeType> &types) {
  uint32_t packed = 0;
  for (size_t i = 0; i < types.size() && i < 4; ++i) {
    packed |= static_cast<uint32_t>(types[i]) << (i * 8);
  }
  return packed;
}

// Include the auto-generated lookup table
#include "BorderLookupTable.inc"

} // namespace Brushes
} // namespace Services
} // namespace MapEditor
