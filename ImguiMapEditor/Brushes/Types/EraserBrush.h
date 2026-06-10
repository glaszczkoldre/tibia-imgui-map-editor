#pragma once
#include "Brushes/Core/IBrush.h"

namespace MapEditor::Brushes {

/**
 * Brush for erasing items/entities from tiles.
 *
 * Can be configured to erase specific types:
 * - Ground items
 * - Stacked items
 * - Creatures
 * - Spawns
 * - Flags
 */
class EraserBrush : public IBrush {
public:
  EraserBrush() noexcept;
  ~EraserBrush() override = default;

  // IBrush interface
  void draw(Domain::ChunkedMap &map, Domain::Tile *tile,
            const DrawContext &ctx) override;
  void undraw(Domain::ChunkedMap &map, Domain::Tile *tile) override;

  const std::string &getName() const override { return name_; }
  BrushType getType() const override { return BrushType::Eraser; }
  uint32_t getLookId() const override { return 0; }

private:
  std::string name_ = "Eraser";

  // What to erase (all true by default)
  bool eraseGround_ = true;
  bool eraseItems_ = true;
  bool eraseCreatures_ = true;
  bool eraseSpawns_ = true;
};

} // namespace MapEditor::Brushes
