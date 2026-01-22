#include "SmartSelectionStrategy.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Creature.h"
#include "Domain/Item.h"
#include "Domain/Spawn.h"
#include <spdlog/spdlog.h>

namespace MapEditor::AppLogic {

using namespace Domain::Selection;

SelectionEntry
SmartSelectionStrategy::selectAt(const Domain::ChunkedMap *map,
                                 const Domain::Position &pos,
                                 const glm::vec2 & /*pixel_offset*/) const {
  const Domain::Tile *tile = map ? map->getTile(pos) : nullptr;

  // Helper to create EntityId
  auto makeEntityId = [&pos](EntityType type, uint64_t local_id) {
    EntityId id;
    id.position = pos;
    id.type = type;
    id.local_id = local_id;
    return id;
  };

  // If no tile, return empty entry (ground type with null pointer)
  if (!tile) {
    return SelectionEntry{makeEntityId(EntityType::Ground, 0), nullptr, 0};
  }

  // Priority 1: Spawn center (direct click on spawn tile)
  if (tile->hasSpawn()) {
    const Domain::Spawn *spawn = tile->getSpawn();
    return SelectionEntry{
        makeEntityId(EntityType::Spawn, reinterpret_cast<uint64_t>(spawn)),
        spawn, 0};
  }

  // Priority 2: Creature on this tile (per-tile storage like RME)
  if (tile->hasCreature()) {
    const Domain::Creature *creature = tile->getCreature();
    spdlog::info("[SELECT] Selected creature '{}' at ({},{},{})",
                 creature->name, pos.x, pos.y, pos.z);
    return SelectionEntry{makeEntityId(EntityType::Creature,
                                       reinterpret_cast<uint64_t>(creature)),
                          creature, 0};
  }

  // Priority 3: Top item (excluding ground)
  const auto &items = tile->getItems();
  if (!items.empty()) {
    // Get topmost item
    const auto &top_item = items.back();
    if (top_item) {
      return SelectionEntry{
          makeEntityId(EntityType::Item,
                       reinterpret_cast<uint64_t>(top_item.get())),
          top_item.get(), top_item->getServerId()};
    }
  }

  // Priority 4: Ground item
  if (tile->hasGround()) {
    const Domain::Item *ground = tile->getGround();
    if (ground) {
      return SelectionEntry{makeEntityId(EntityType::Ground, 0), ground,
                            ground->getServerId()};
    }
  }

  // Fallback: empty ground entry
  return SelectionEntry{makeEntityId(EntityType::Ground, 0), nullptr, 0};
}

} // namespace MapEditor::AppLogic
