#pragma once
#include "Domain/Selection/SelectionEntry.h"
#include "Domain/Tile.h"
#include <glm/glm.hpp>

namespace MapEditor::Domain {
class ChunkedMap;
}

namespace MapEditor::AppLogic {

/**
 * Interface for selection strategies.
 * Determines which entity to select when clicking on a tile.
 */
class ISelectionStrategy {
public:
  virtual ~ISelectionStrategy() = default;

  /**
   * Determine what to select at the given tile.
   * @param map The map containing the tile
   * @param pos World position
   * @param pixel_offset Sub-tile pixel offset (for pixel-perfect mode)
   * @return Selection entry for the picked entity
   */
  virtual Domain::Selection::SelectionEntry
  selectAt(const Domain::ChunkedMap *map, const Domain::Position &pos,
           const glm::vec2 &pixel_offset = {0, 0}) const = 0;
};

} // namespace MapEditor::AppLogic
