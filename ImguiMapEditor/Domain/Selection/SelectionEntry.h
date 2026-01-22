#pragma once
#include "Domain/Position.h"
#include <cstdint>
#include <functional>

namespace MapEditor::Domain::Selection {

/**
 * Type of entity that can be selected.
 */
enum class EntityType : uint8_t {
  Ground = 0,
  Item = 1,
  Creature = 2,
  Spawn = 3
};

/**
 * Unique identifier for a selectable entity.
 * Combines position + type + entity-specific ID for uniqueness.
 *
 * The local_id is typically:
 * - For Ground: 0 (only one ground per tile)
 * - For Item: reinterpret_cast<uint64_t>(item_ptr) or item stack index
 * - For Creature: hash of creature name or pointer
 * - For Spawn: reinterpret_cast<uint64_t>(spawn_ptr)
 */
struct EntityId {
  Position position;
  EntityType type = EntityType::Item;
  uint64_t local_id = 0;

  bool operator==(const EntityId &other) const {
    return position == other.position && type == other.type &&
           local_id == other.local_id;
  }

  bool operator!=(const EntityId &other) const { return !(*this == other); }

  /**
   * Compute a hash for use in containers.
   * Combines position pack with type and local_id.
   */
  uint64_t hash() const {
    // Position pack is 64 bits, combine with type and local_id
    uint64_t pos_hash = position.pack();
    uint64_t type_hash = static_cast<uint64_t>(type) << 56;
    // XOR with local_id for uniqueness
    return pos_hash ^ type_hash ^ (local_id * 0x9e3779b97f4a7c15ULL);
  }
};

/**
 * A single selection entry - references one entity on the map.
 * Pure value type, no business logic.
 *
 * Design notes:
 * - entity_ptr is non-owning and for validation only
 * - item_id is cached for copy operations (avoids pointer dereference)
 * - Equality is based on EntityId only
 */
struct SelectionEntry {
  EntityId id;
  const void *entity_ptr = nullptr; // Non-owning, for validation only
  uint16_t item_id = 0;             // For items: server ID for copy operations

  SelectionEntry() = default;

  SelectionEntry(const EntityId &entity_id, const void *ptr = nullptr,
                 uint16_t server_id = 0)
      : id(entity_id), entity_ptr(ptr), item_id(server_id) {}

  bool operator==(const SelectionEntry &other) const { return id == other.id; }

  bool operator!=(const SelectionEntry &other) const {
    return !(*this == other);
  }

  /**
   * Get the position of this entry.
   */
  const Position &getPosition() const { return id.position; }

  /**
   * Get the entity type.
   */
  EntityType getType() const { return id.type; }
};

/**
 * Hash functor for EntityId in unordered containers.
 */
struct EntityIdHash {
  size_t operator()(const EntityId &id) const {
    return std::hash<uint64_t>{}(id.hash());
  }
};

/**
 * Hash functor for SelectionEntry in unordered containers.
 */
struct SelectionEntryHash {
  size_t operator()(const SelectionEntry &entry) const {
    return std::hash<uint64_t>{}(entry.id.hash());
  }
};

/**
 * Convert EntityType to string for debugging.
 */
inline const char *entityTypeToString(EntityType type) {
  switch (type) {
  case EntityType::Ground:
    return "Ground";
  case EntityType::Item:
    return "Item";
  case EntityType::Creature:
    return "Creature";
  case EntityType::Spawn:
    return "Spawn";
  default:
    return "Unknown";
  }
}

} // namespace MapEditor::Domain::Selection
