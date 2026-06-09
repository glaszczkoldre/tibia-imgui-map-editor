#pragma once
/**
 * @file WallLookupService.h
 * @brief Provides wall alignment lookups based on 4-neighbor configuration.
 */

#include "../../Brushes/Enums/BrushEnums.h"
#include <array>

namespace MapEditor {
namespace Services {
namespace Brushes {

using ::MapEditor::Brushes::WallAlign;
using ::MapEditor::Brushes::WallNeighbor;

/**
 * Lookup service for wall auto-alignment.
 *
 * Supports both "full" walls (complete connections) and "half" walls
 * (partial connections for wall decorations).
 */
class WallLookupService {
public:
  WallLookupService();

  /**
   * Get wall alignment for full wall connections.
   * Used for standard walls that connect in all directions.
   */
  [[nodiscard]] WallAlign getFullType(WallNeighbor neighbors) const noexcept;

  /**
   * Get wall alignment for half wall (decoration) connections.
   * Used for wall decorations that only consider certain neighbors.
   */
  [[nodiscard]] WallAlign getHalfType(WallNeighbor neighbors) const noexcept;

private:
  void initializeTable();
  std::array<WallAlign, 16> fullTable_;
  std::array<WallAlign, 16> halfTable_;
};

} // namespace Brushes
} // namespace Services
} // namespace MapEditor
