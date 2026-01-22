#pragma once
#include "Domain/Selection/SelectionEntry.h"
#include <vector>

namespace MapEditor::Services::Selection {

/**
 * Observer interface for selection state changes.
 * Implement this to react to selection updates (UI refresh, etc.).
 *
 * Usage:
 *   class SelectionOverlay : public ISelectionObserver {
 *     void onSelectionChanged(...) override {
 *       // Regenerate overlay geometry
 *     }
 *   };
 *
 * Design notes:
 * - Observers are not owned by SelectionService
 * - Observer lifetime must exceed SelectionService usage
 * - onSelectionChanged provides delta for efficient updates
 */
class ISelectionObserver {
public:
  virtual ~ISelectionObserver() = default;

  /**
   * Called when selection has changed.
   * Provides delta information for efficient UI updates.
   *
   * @param added Entries that were added to selection
   * @param removed Entries that were removed from selection
   */
  virtual void onSelectionChanged(
      const std::vector<Domain::Selection::SelectionEntry> &added,
      const std::vector<Domain::Selection::SelectionEntry> &removed) = 0;

  /**
   * Called when selection is completely cleared.
   * More efficient than receiving all entries as "removed".
   */
  virtual void onSelectionCleared() = 0;
};

} // namespace MapEditor::Services::Selection
