#pragma once
#include "ISelectionStrategy.h"

namespace MapEditor::AppLogic {

/**
 * Smart Selection Strategy (Default).
 * Context-sensitive logical selection with priority:
 * Creature > Top Item > Ground > Tile
 */
class SmartSelectionStrategy : public ISelectionStrategy {
public:
  Domain::Selection::SelectionEntry
  selectAt(const Domain::ChunkedMap *map, const Domain::Position &pos,
           const glm::vec2 &pixel_offset = {0, 0}) const override;
};

} // namespace MapEditor::AppLogic
