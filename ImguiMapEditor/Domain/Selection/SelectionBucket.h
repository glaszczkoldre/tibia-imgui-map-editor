#pragma once
#include "SelectionEntry.h"
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace MapEditor::Domain::Selection {

/**
 * Pure data container for selection state.
 * No business logic - just storage and basic queries.
 *
 * Design principles:
 * - Single source of truth: entries_ map
 * - position_index_ is a secondary index for O(1) position lookup
 * - Invariant: position_index_ is always consistent with entries_
 *
 * Thread safety: NOT thread-safe. Caller must synchronize access.
 */
class SelectionBucket {
public:
  SelectionBucket() = default;
  ~SelectionBucket() = default;

  // Copyable and movable
  SelectionBucket(const SelectionBucket &) = default;
  SelectionBucket &operator=(const SelectionBucket &) = default;
  SelectionBucket(SelectionBucket &&) noexcept = default;
  SelectionBucket &operator=(SelectionBucket &&) noexcept = default;

  // === Modification ===

  /**
   * Add an entry to the selection.
   * If the entry already exists (by EntityId), it is not duplicated.
   */
  void add(const SelectionEntry &entry);

  /**
   * Remove an entry by its EntityId.
   * No-op if the entity is not selected.
   */
  void remove(const EntityId &id);

  /**
   * Remove all entries at a given position.
   */
  void removeAllAt(const Position &pos);

  /**
   * Clear all entries.
   */
  void clear();

  // === Query ===

  /**
   * Check if a specific entity is selected.
   */
  bool contains(const EntityId &id) const;

  /**
   * Check if there are any entries at a given position.
   */
  bool hasEntriesAt(const Position &pos) const;

  /**
   * Get the total number of selected entities.
   */
  size_t size() const { return entries_.size(); }

  /**
   * Check if the selection is empty.
   */
  bool empty() const { return entries_.empty(); }

  // === Iteration ===

  /**
   * Get all entries at a specific position.
   * Returns empty vector if no entries at that position.
   */
  std::vector<SelectionEntry> getEntriesAt(const Position &pos) const;

  /**
   * Get all entries in the selection.
   */
  std::vector<SelectionEntry> getAllEntries() const;

  /**
   * Get all unique positions that have selected entities.
   */
  std::vector<Position> getPositions() const;

  // === Bounds (for rendering optimization) ===

  /**
   * Get the minimum bound (top-left-highest corner) of the selection.
   * Returns (0,0,0) if selection is empty.
   */
  Position getMinBound() const;

  /**
   * Get the maximum bound (bottom-right-lowest corner) of the selection.
   * Returns (0,0,0) if selection is empty.
   */
  Position getMaxBound() const;

  // === Floor filtering ===

  /**
   * Get all entries on a specific floor.
   */
  std::vector<SelectionEntry> getEntriesOnFloor(int16_t floor) const;

  /**
   * Get all positions on a specific floor.
   */
  std::vector<Position> getPositionsOnFloor(int16_t floor) const;

private:
  // Primary storage: entity hash → entry
  // Using hash of EntityId as key for O(1) lookup
  std::unordered_map<uint64_t, SelectionEntry> entries_;

  // Secondary index: position pack → set of entity hashes at that position
  // Enables O(1) lookup of "what's selected at this position?"
  std::unordered_map<uint64_t, std::unordered_set<uint64_t>> position_index_;

  // Helpers
  static uint64_t positionKey(const Position &pos) { return pos.pack(); }
  static uint64_t entityKey(const EntityId &id) { return id.hash(); }
};

} // namespace MapEditor::Domain::Selection
