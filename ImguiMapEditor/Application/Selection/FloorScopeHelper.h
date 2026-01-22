#pragma once
#include "Domain/SelectionMode.h"
#include <algorithm>
#include <cstdint>

namespace MapEditor::Domain {
class ChunkedMap;
struct Position;
} // namespace MapEditor::Domain

namespace MapEditor::Services::Selection {
class SelectionService;
}

namespace MapEditor::AppLogic {

/**
 * Floor range for selection operations.
 * Used by FloorScopeHelper to determine which floors to iterate.
 */
struct FloorRange {
  int16_t start_z; ///< Starting floor (higher number = underground)
  int16_t end_z;   ///< Ending floor (inclusive)
};

/**
 * Compute floor range based on scope setting (matches RME logic).
 *
 * @param scope Current FloorScope setting from SelectionSettings
 * @param current_floor Currently active editing floor
 * @return FloorRange with start_z and end_z for iteration (descending: z =
 * start to end)
 *
 * RME Logic:
 * - CurrentFloor: only z=current
 * - AllFloors: z=15 to z=current (all floors from bottom to current)
 * - VisibleFloors:
 *   - Above ground (z<=7): z=7 to z=current
 *   - Underground (z>7): z=min(15, current+2) to z=current
 */
FloorRange getFloorRange(Domain::SelectionFloorScope scope,
                         int16_t current_floor);

/**
 * Add all items from a tile stack at the given X/Y across multiple floors
 * based on the FloorScope setting. This is a shared helper used by
 * Shift+Click and Ctrl+Shift+Click selection handlers.
 *
 * @param map The chunked map to get tiles from
 * @param selection_service The selection service to add entries to
 * @param pos The base position (uses X/Y, Z is starting point for scope calc)
 * @param scope Current FloorScope setting
 */
void selectTileStackAcrossFloors(
    Domain::ChunkedMap *map,
    Services::Selection::SelectionService &selection_service,
    const Domain::Position &pos, Domain::SelectionFloorScope scope);

} // namespace MapEditor::AppLogic
