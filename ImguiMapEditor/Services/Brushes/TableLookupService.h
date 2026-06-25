#pragma once
/**
 * @file TableLookupService.h
 * @brief Provides table alignment lookups based on 8-neighbor configuration.
 */

#include "Brushes/Enums/BrushEnums.h"
#include <array>

namespace MapEditor {
namespace Services {
namespace Brushes {

using ::MapEditor::Brushes::TableAlign;
using ::MapEditor::Brushes::TileNeighbor;

/**
 * Lookup service for table brush auto-alignment.
 */
class TableLookupService {
public:
  TableLookupService();

  /**
   * Get table alignment for neighbor configuration.
   * @param neighbors 8-bit TileNeighbor bitmask
   * @return TableAlign value
   */
  [[nodiscard]] TableAlign getTableType(TileNeighbor neighbors) const noexcept;

};

} // namespace Brushes
} // namespace Services
} // namespace MapEditor
