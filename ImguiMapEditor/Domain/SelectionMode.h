#pragma once

namespace MapEditor::Domain {

/**
 * Floor scope for selection operations.
 * Determines which floors are affected by selection actions.
 */
enum class SelectionFloorScope {
  /// Only current floor (default)
  CurrentFloor,

  /// All visible floors based on view settings
  VisibleFloors,

  /// All floors 0-15
  AllFloors
};

} // namespace MapEditor::Domain
