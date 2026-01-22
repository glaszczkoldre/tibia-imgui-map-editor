#pragma once
#include "SelectionBucket.h"
#include <vector>

namespace MapEditor::Domain::Selection {

/**
 * Memento pattern: captures complete selection state for undo/redo.
 * Immutable once created.
 *
 * Usage:
 *   // Capture current state
 *   SelectionSnapshot snapshot = SelectionSnapshot::capture(bucket);
 *
 *   // ... modify selection ...
 *
 *   // Restore to previous state
 *   bucket = snapshot.restore();
 *
 * Design notes:
 * - Stores a copy of all entries (value semantics)
 * - No pointers to avoid dangling after map changes
 * - Lightweight: only stores SelectionEntry structs
 */
class SelectionSnapshot {
public:
  /**
   * Create an empty snapshot.
   */
  SelectionSnapshot() = default;

  /**
   * Capture the current state of a SelectionBucket.
   */
  static SelectionSnapshot capture(const SelectionBucket &bucket) {
    SelectionSnapshot snapshot;
    snapshot.entries_ = bucket.getAllEntries();
    return snapshot;
  }

  /**
   * Restore a SelectionBucket from this snapshot.
   * Creates a new bucket with the captured state.
   */
  SelectionBucket restore() const {
    SelectionBucket bucket;
    for (const auto &entry : entries_) {
      bucket.add(entry);
    }
    return bucket;
  }

  /**
   * Get the number of entries in this snapshot.
   */
  size_t size() const { return entries_.size(); }

  /**
   * Check if this snapshot is empty.
   */
  bool empty() const { return entries_.empty(); }

  /**
   * Get all entries in this snapshot (for debugging/inspection).
   */
  const std::vector<SelectionEntry> &getEntries() const { return entries_; }

private:
  std::vector<SelectionEntry> entries_;
};

} // namespace MapEditor::Domain::Selection
