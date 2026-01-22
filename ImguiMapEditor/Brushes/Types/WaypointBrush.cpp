#include "WaypointBrush.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Tile.h"
#include <spdlog/spdlog.h>

namespace MapEditor::Brushes {

WaypointBrush::WaypointBrush() = default;

void WaypointBrush::draw(Domain::ChunkedMap &map, Domain::Tile *tile,
                         const DrawContext &ctx) {
  if (!tile)
    return;
  if (waypointName_.empty())
    return; // No waypoint name set

  // TODO: Implement waypoint storage on tile
  // tile->setWaypoint(waypointName_);

  spdlog::trace("[WaypointBrush] Set waypoint '{}' at ({},{},{})",
                waypointName_, tile->getPosition().x, tile->getPosition().y,
                tile->getPosition().z);
}

void WaypointBrush::undraw(Domain::ChunkedMap &map, Domain::Tile *tile) {
  if (!tile)
    return;

  // TODO: Implement waypoint removal from tile
  // tile->removeWaypoint();

  spdlog::trace("[WaypointBrush] Removed waypoint from ({},{},{})",
                tile->getPosition().x, tile->getPosition().y,
                tile->getPosition().z);
}

} // namespace MapEditor::Brushes
