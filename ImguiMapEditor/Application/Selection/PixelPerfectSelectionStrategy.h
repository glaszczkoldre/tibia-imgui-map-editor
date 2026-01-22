#pragma once
#include "ISelectionStrategy.h"

namespace MapEditor {
namespace Services {
class ClientDataService;
}
} // namespace MapEditor

namespace MapEditor::AppLogic {

/**
 * Pixel-Perfect Selection Strategy.
 * Advanced visual precision using sprite hit testing.
 * Selects the exact sprite visible under the mouse cursor.
 */
class PixelPerfectSelectionStrategy : public ISelectionStrategy {
public:
  explicit PixelPerfectSelectionStrategy(
      Services::ClientDataService *client_data);

  Domain::Selection::SelectionEntry
  selectAt(const Domain::ChunkedMap *map, const Domain::Position &pos,
           const glm::vec2 &pixel_offset = {0, 0}) const override;

private:
  /**
   * Helper to find first hit on a specific tile.
   */
  Domain::Selection::SelectionEntry
  findHitOnTile(const Domain::Tile *tile, const Domain::Position &pos,
                const glm::vec2 &pixel_offset) const;

  /**
   * Check if pixel offset hits an item's sprite bounds.
   */
  bool hitTestItem(const Domain::Item *item, const glm::vec2 &pixel_offset,
                   const Domain::Position &tile_pos) const;

  /**
   * Check if pixel offset hits a creature's sprite bounds.
   */
  bool hitTestCreature(const Domain::Creature *creature,
                       const glm::vec2 &pixel_offset,
                       const Domain::Position &tile_pos) const;

  Services::ClientDataService *client_data_;
};

} // namespace MapEditor::AppLogic
