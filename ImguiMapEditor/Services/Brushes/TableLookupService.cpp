/**
 * @file TableLookupService.cpp
 * @brief Implementation of table lookup table initialization.
 */

#include "TableLookupService.h"

namespace MapEditor {
namespace Services {
namespace Brushes {

TableLookupService::TableLookupService() { initializeTable(); }

TableAlign TableLookupService::getTableType(TileNeighbor neighbors) const {
  return table_[static_cast<uint8_t>(neighbors)];
}

// Include the auto-generated lookup table
#include "TableLookupTable.inc"

} // namespace Brushes
} // namespace Services
} // namespace MapEditor
