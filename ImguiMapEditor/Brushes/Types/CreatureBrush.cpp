#include "CreatureBrush.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Creature.h"
#include "Domain/Spawn.h"
#include "Domain/Tile.h"
#include "Services/BrushSettingsService.h"
#include <memory>
#include <spdlog/spdlog.h>

namespace MapEditor::Brushes {

CreatureBrush::CreatureBrush(const std::string &name,
                             const Domain::Outfit &outfit)
    : name_(name), outfit_(outfit) {}

void CreatureBrush::draw(Domain::ChunkedMap &map, Domain::Tile *tile,
                         const DrawContext &ctx) {
  if (!tile)
    return;

  // Create new creature instance
  auto creature = std::make_unique<Domain::Creature>();
  creature->setName(name_);
  creature->setOutfit(outfit_);
  creature->setPosition(tile->getPosition());
  if (ctx.brushSettings) {
    creature->spawn_time = ctx.brushSettings->getDefaultSpawnTime();
  }

  // Add to tile (Tile takes ownership)
  tile->setCreature(std::move(creature));
  tile->setCreatureBrushId(ctx.ownerBrushId);

  // Auto-create spawn if enabled in settings
  if (ctx.brushSettings && ctx.brushSettings->getAutoCreateSpawn()) {
    // Check if this tile is within ANY existing spawn's radius
    bool withinExistingSpawn = false;

    // Search nearby tiles for spawns that cover this position
    Domain::Position pos = tile->getPosition();
    constexpr int kMaxSpawnSearchRadius = 10;

    for (int dy = -kMaxSpawnSearchRadius; dy <= kMaxSpawnSearchRadius && !withinExistingSpawn; ++dy) {
      for (int dx = -kMaxSpawnSearchRadius; dx <= kMaxSpawnSearchRadius && !withinExistingSpawn; ++dx) {
        Domain::Tile *nearbyTile = map.getTile(pos.x + dx, pos.y + dy, pos.z);
        if (nearbyTile && nearbyTile->hasSpawn()) {
          const Domain::Spawn *spawn = nearbyTile->getSpawn();
          if (spawn) {
            // Check if current tile is within this spawn's radius
            int spawnRadius = spawn->radius;
            if (std::abs(dx) <= spawnRadius && std::abs(dy) <= spawnRadius) {
              withinExistingSpawn = true;
            }
          }
        }
      }
    }

    // Only create spawn if not within any existing spawn
    if (!withinExistingSpawn) {
      int radius = ctx.brushSettings->getDefaultSpawnRadius();
      auto spawn = std::make_unique<Domain::Spawn>(tile->getPosition(), radius);
      tile->setSpawn(std::move(spawn));
      tile->setSpawnBrushId(ctx.ownerBrushId);
      spdlog::debug(
          "[CreatureBrush] Auto-created spawn at ({},{},{}) with radius {}",
          tile->getPosition().x, tile->getPosition().y, tile->getPosition().z,
          radius);
    }
  }

  map.markChanged();
}

void CreatureBrush::undraw(Domain::ChunkedMap& map, Domain::Tile* tile) {
  if (!tile)
    return;
  // Remove creature from tile
  tile->removeCreature();
  tile->setCreatureBrushId(InvalidBrushId);
  map.markChanged();
}

} // namespace MapEditor::Brushes
