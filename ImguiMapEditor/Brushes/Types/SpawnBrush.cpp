/**
 * @file SpawnBrush.cpp
 * @brief Implementation of SpawnBrush for placing spawn points.
 */

#include "SpawnBrush.h"

#include "Domain/ChunkedMap.h"
#include "Domain/Spawn.h"
#include "Domain/Tile.h"
#include "Services/BrushSettingsService.h"
#include <memory>

namespace MapEditor::Brushes {

SpawnBrush::SpawnBrush() = default;

void SpawnBrush::draw(Domain::ChunkedMap &map, Domain::Tile *tile,
                      const DrawContext &ctx) {
  if (!tile)
    return;

  // Don't overwrite existing spawn
  if (tile->hasSpawn())
    return;

  // Get radius from DrawContext brush settings, fallback to member, or use
  // default
  int radius = 3;
  if (ctx.brushSettings) {
    radius = ctx.brushSettings->getDefaultSpawnRadius();
  } else if (settingsService_) {
    radius = settingsService_->getDefaultSpawnRadius();
  }

  // Create spawn at this tile's position
  auto spawn = std::make_unique<Domain::Spawn>(tile->getPosition(), radius);
  tile->setSpawn(std::move(spawn));
}

void SpawnBrush::undraw(Domain::ChunkedMap &map, Domain::Tile *tile) {
  if (!tile)
    return;

  if (tile->hasSpawn()) {
    tile->removeSpawn();
  }
}

} // namespace MapEditor::Brushes
