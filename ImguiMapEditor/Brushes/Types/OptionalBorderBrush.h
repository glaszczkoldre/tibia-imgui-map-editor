#pragma once

#include "Brushes/Core/BrushBase.h"

namespace MapEditor::Brushes {

class OptionalBorderBrush : public BrushBase {
public:
  OptionalBorderBrush();

  BrushType getType() const override { return BrushType::OptionalBorder; }

  void draw(Domain::ChunkedMap &map, Domain::Tile *tile,
            const DrawContext &ctx) override;
  void undraw(Domain::ChunkedMap &map, Domain::Tile *tile) override;

  void setBrushRegistry(const BrushRegistry *registry) { registry_ = registry; }

private:
  const BrushRegistry *registry_ = nullptr;
};

} // namespace MapEditor::Brushes
