#pragma once

#include "Brushes/Core/BrushBase.h"
#include "Brushes/Enums/BrushEnums.h"
#include <array>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace MapEditor::Brushes {

class BrushRegistry;

/**
 * Carpet brush.
 *
 * RME behaviour (see `docs/carpet-parity-report.md`):
 *   * `draw`  — place a single Center item, then re-align the 3x3 area.
 *   * `undraw` — remove all owned items, then re-align the 3x3 area.
 *   * `rebuildTile` — given the 8-neighbour bitmask of owned tiles,
 *     look up the alignment and update the items in this tile to match.
 *
 * All alignment computation is delegated to `CarpetLookupService`. Item
 * selection and ownership is shared with `TableBrush` via
 * `Brushes::Helpers::AlignedBrushHelpers`. The `planTile` method is the
 * single source of truth for what items *should* be on a tile — both the
 * live commit path (`rebuildTile`) and the preview path
 * (`AutoborderBrushPreviewProvider`) call it.
 */
class CarpetBrush : public BrushBase {
public:
  CarpetBrush(std::string name, uint32_t lookId, BrushRegistry &registry);

  BrushType getType() const override { return BrushType::Carpet; }
  bool needsBorderUpdate() const override { return true; }
  bool needBorders() const override { return true; }

  void draw(Domain::ChunkedMap &map, Domain::Tile *tile,
            const DrawContext &ctx) override;
  void undraw(Domain::ChunkedMap &map, Domain::Tile *tile) override;
  bool ownsItem(const Domain::Item *item) const override;

  void addAlignedItem(EdgeType align, uint16_t itemId, uint32_t chance);
  uint16_t getPreviewItemId() const;

  void placeCenterTile(Domain::Tile &tile, const DrawContext &ctx) const;
  void eraseFromTile(Domain::Tile &tile) const;
  void rebuildAround(Domain::ChunkedMap &map,
                     const Domain::Position &center) const;
  void rebuildTile(Domain::ChunkedMap &map,
                   const Domain::Position &pos) const;

  /**
   * Plan the items that *should* be on a single tile after alignment. The
   * returned vector contains one entry per owned item, in the order they
   * should appear in the tile (first entry → lowest slot, etc.). An empty
   * result means "no items" — the caller should leave the tile empty.
   *
   * This is the single source of truth for preview AND commit: both
   * `AutoborderBrushPreviewProvider` and `rebuildTile` go through it.
   */
  struct PlannedItem {
    uint16_t itemId = 0;
    EdgeType alignment = EdgeType::Center;
  };
  [[nodiscard]] std::vector<PlannedItem>
  planTile(const Domain::ChunkedMap &map,
           const Domain::Position &pos) const;

  /**
   * Apply a plan to a tile — write the planned items, removing any excess
   * owned items that aren't in the plan. Used by `rebuildTile` (commit)
   * and by the preview provider (cloned scratch map).
   */
  void applyTilePlan(Domain::Tile &tile,
                     const std::vector<PlannedItem> &plan) const;

private:
  static constexpr size_t kEdgeTypeCount = 14;
  uint16_t selectItem(EdgeType align) const;
  bool tileHasBrush(const Domain::Tile *tile) const;

  BrushRegistry &registry_;
  std::array<std::vector<std::pair<uint16_t, uint32_t>>, kEdgeTypeCount> itemsByEdge_{};
  std::unordered_set<uint16_t> ownedItemIds_;
};

} // namespace MapEditor::Brushes
