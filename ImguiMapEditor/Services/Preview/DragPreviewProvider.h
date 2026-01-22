#pragma once
#include "Domain/Selection/SelectionEntry.h"
#include "IPreviewProvider.h"
#include "Services/Selection/SelectionService.h"


namespace MapEditor::Domain {
class ChunkedMap;
struct Creature;
}

namespace MapEditor::Services::Preview {

/**
 * Preview provider for drag operations.
 *
 * Extracts selected items/tiles from the map and provides
 * them as preview data following the cursor with an offset.
 */
class DragPreviewProvider : public IPreviewProvider {
public:
  /**
   * Create drag preview from selection.
   * @param selectionService Current selection service (must outlive provider)
   * @param map Map to extract items from (must outlive provider)
   * @param dragStartPos Position where drag started
   */
  DragPreviewProvider(const Selection::SelectionService &selectionService,
                      Domain::ChunkedMap *map,
                      const Domain::Position &dragStartPos);

  // IPreviewProvider interface
  bool isActive() const override;
  Domain::Position getAnchorPosition() const override;
  const std::vector<PreviewTileData> &getTiles() const override;
  PreviewBounds getBounds() const override;
  void updateCursorPosition(const Domain::Position &cursor) override;

private:
  const Selection::SelectionService *selection_service_;
  Domain::ChunkedMap *map_;
  Domain::Position dragStartPos_;
  Domain::Position currentPos_{0, 0, 0};

  std::vector<PreviewTileData> tiles_;
  PreviewBounds bounds_;

  void buildPreview();
  void addTileItems(const Domain::Position &pos);
  void addSingleItem(const Domain::Position &pos, const Domain::Item *item);
  void addCreature(const Domain::Position &pos, const Domain::Creature *creature);
  void addSpawn(const Domain::Position &pos);
};

} // namespace MapEditor::Services::Preview
