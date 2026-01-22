#include "EraserBrush.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Tile.h"
#include <spdlog/spdlog.h>

namespace MapEditor::Brushes {

EraserBrush::EraserBrush() = default;

void EraserBrush::draw(Domain::ChunkedMap &map, Domain::Tile *tile,
                       const DrawContext &ctx) {
  if (!tile)
    return;

  // Erase ground if enabled
  if (eraseGround_ && tile->hasGround()) {
    tile->removeGround();
  }

  // Erase stacked items if enabled
  if (eraseItems_) {
    tile->clearItems();
  }

  // Erase creature if enabled
  if (eraseCreatures_ && tile->hasCreature()) {
    tile->removeCreature();
  }

  // Erase spawn if enabled
  if (eraseSpawns_ && tile->hasSpawn()) {
    tile->removeSpawn();
  }

  spdlog::trace("[EraserBrush] Erased at ({},{},{})", tile->getPosition().x,
                tile->getPosition().y, tile->getPosition().z);
}

void EraserBrush::undraw(Domain::ChunkedMap &map, Domain::Tile *tile) {
  // Eraser doesn't have undraw - history system handles undo
  // This is intentionally empty
}

} // namespace MapEditor::Brushes
