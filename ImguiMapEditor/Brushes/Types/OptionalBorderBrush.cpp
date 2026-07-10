#include "OptionalBorderBrush.h"

#include "Brushes/BrushRegistry.h"
#include "GroundBrush.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Item.h"
#include "Domain/Tile.h"

namespace MapEditor::Brushes {

OptionalBorderBrush::OptionalBorderBrush()
    : BrushBase("Optional Border Tool", 0, true) {}

void OptionalBorderBrush::draw(Domain::ChunkedMap &map, Domain::Tile *tile,
                               const DrawContext &ctx) {
  if (!tile || !tile->hasGround() || !ctx.brushRegistry) {
    return;
  }

  tile->setOptionalBorder(true);
  tile->setOptionalBorderBrushId(ctx.ownerBrushId);

  if (const auto *brush = GroundBrush::resolveGroundBrush(*ctx.brushRegistry, *tile);
      brush && brush->hasOptionalBorderRule()) {
    const_cast<GroundBrush *>(brush)->rebuildAround(map, tile->getPosition());
  }

  map.markChanged();
}

void OptionalBorderBrush::undraw(Domain::ChunkedMap &map, Domain::Tile *tile) {
  if (!tile || !tile->hasGround()) {
    return;
  }
  tile->setOptionalBorder(false);
  tile->setOptionalBorderBrushId(InvalidBrushId);

  if (registry_) {
    if (const auto *brush = GroundBrush::resolveGroundBrush(*registry_, *tile);
        brush && brush->hasOptionalBorderRule()) {
      const_cast<GroundBrush *>(brush)->rebuildAround(map, tile->getPosition());
    }
  }

  map.markChanged();
}

} // namespace MapEditor::Brushes
