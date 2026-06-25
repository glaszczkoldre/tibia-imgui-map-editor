/**
 * @file TableLookupService.cpp
 * @brief Implementation of table lookup table initialization.
 */

#include "TableLookupService.h"

namespace MapEditor {
namespace Services {
namespace Brushes {

#include "TableLookupTable.inc"

TableLookupService::TableLookupService() = default;

TableAlign TableLookupService::getTableType(TileNeighbor neighbors) const noexcept {
  return kTableLookupTable[static_cast<uint8_t>(neighbors)];
}

} // namespace Brushes
} // namespace Services
} // namespace MapEditor
