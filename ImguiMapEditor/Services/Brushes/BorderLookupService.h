#pragma once
/**
 * @file BorderLookupService.h
 * @brief Provides border type lookups for ground brush auto-bordering.
 *
 * The 256-entry lookup table maps 8-neighbor bitmasks to packed border types.
 * Up to 4 border types can be packed into a single uint32_t value.
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
 * Lookup service for auto-border calculation.
 *
 * Uses a 256-entry table (indexed by TileNeighbor bitmask) to determine
 * which border types to place based on neighboring tiles.
 */
class BorderLookupService {
public:
  BorderLookupService();

  /**
   * Get packed border types for neighbor configuration.
   * @param neighbors 8-bit TileNeighbor bitmask
   * @return Packed value: (type1) | (type2 << 8) | (type3 << 16) | (type4 <<
   * 24)
   */
  uint32_t getBorderTypes(TileNeighbor neighbors) const;

  /**
   * Unpack border types from packed value.
   * @param packed The packed border value
   * @return Vector of 1-4 EdgeType values (excludes None)
   */
  static std::vector<EdgeType> unpack(uint32_t packed);

  /**
   * Pack border types into single value.
   * @param types Vector of EdgeType values (max 4)
   * @return Packed uint32_t value
   */
  static uint32_t pack(const std::vector<EdgeType> &types);

private:
  void initializeTable();
  std::array<uint32_t, 256> table_;
};

} // namespace Brushes
} // namespace Services
} // namespace MapEditor
