#pragma once
/**
 * @file CarpetLookupService.h
 * @brief Provides carpet alignment lookups based on 8-neighbor configuration.
 */

#include "../../Brushes/Enums/BrushEnums.h"
#include <array>
#include <cstdint>
#include <vector>

namespace MapEditor {
namespace Services {
namespace Brushes {

using ::MapEditor::Brushes::EdgeType;
using ::MapEditor::Brushes::TileNeighbor;

/**
 * Lookup service for carpet brush auto-alignment.
 * Uses same structure as border lookup (256 entries, packed edge types).
 */
class CarpetLookupService {
public:
  CarpetLookupService();

  /**
   * Get packed edge types for neighbor configuration.
   * @param neighbors 8-bit TileNeighbor bitmask
   * @return Packed edge types (same format as BorderLookupService)
   */
  uint32_t getCarpetTypes(TileNeighbor neighbors) const;

  /**
   * Unpack edge types from packed value.
   */
  static std::vector<EdgeType> unpack(uint32_t packed);

private:
  void initializeTable();
  std::array<uint32_t, 256> table_;
};

} // namespace Brushes
} // namespace Services
} // namespace MapEditor
