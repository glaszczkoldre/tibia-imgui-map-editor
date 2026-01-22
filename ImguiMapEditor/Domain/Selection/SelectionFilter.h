#pragma once
#include "SelectionEntry.h"
#include <optional>

namespace MapEditor::Domain::Selection {

/**
 * Filter options for selection operations.
 * Controls which entity types are affected by select/deselect operations.
 *
 * Usage:
 * - SelectionFilter::all() - affects all entity types
 * - SelectionFilter::singleEntity(id) - affects only specific entity
 * - SelectionFilter::itemsOnly() - affects only items (no ground/creatures)
 */
struct SelectionFilter {
  bool include_ground = true;
  bool include_items = true;
  bool include_creatures = true;
  bool include_spawns = true;

  // For single-entity operations (Ctrl+Click on specific item)
  std::optional<EntityId> specific_entity;

  /**
   * Check if a given entity type should be included.
   */
  bool includes(EntityType type) const {
    switch (type) {
    case EntityType::Ground:
      return include_ground;
    case EntityType::Item:
      return include_items;
    case EntityType::Creature:
      return include_creatures;
    case EntityType::Spawn:
      return include_spawns;
    default:
      return false;
    }
  }

  /**
   * Check if a specific entity matches this filter.
   */
  bool matches(const EntityId &id) const {
    // If specific_entity is set, only match that exact entity
    if (specific_entity.has_value()) {
      return id == specific_entity.value();
    }
    // Otherwise, match based on entity type
    return includes(id.type);
  }

  /**
   * Create a filter that includes all entity types.
   */
  static SelectionFilter all() {
    return SelectionFilter{true, true, true, true, std::nullopt};
  }

  /**
   * Create a filter for a single specific entity.
   */
  static SelectionFilter singleEntity(const EntityId &id) {
    SelectionFilter filter;
    filter.include_ground = false;
    filter.include_items = false;
    filter.include_creatures = false;
    filter.include_spawns = false;
    filter.specific_entity = id;
    return filter;
  }

  /**
   * Create a filter that includes only items (no ground, creatures, spawns).
   */
  static SelectionFilter itemsOnly() {
    return SelectionFilter{false, true, false, false, std::nullopt};
  }

  /**
   * Create a filter that includes items and ground (common for copy).
   */
  static SelectionFilter itemsAndGround() {
    return SelectionFilter{true, true, false, false, std::nullopt};
  }

  /**
   * Create an empty filter that matches nothing.
   */
  static SelectionFilter none() {
    return SelectionFilter{false, false, false, false, std::nullopt};
  }
};

} // namespace MapEditor::Domain::Selection
