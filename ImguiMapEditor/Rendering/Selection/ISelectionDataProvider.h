#pragma once
#include "Domain/Position.h"
#include "Domain/Selection/SelectionEntry.h"
#include <cstdint>
#include <functional>
#include <vector>

namespace MapEditor {
namespace Domain {
class Item;
}

namespace Rendering {

/**
 * Interface for providing selection data to renderers.
 * Decouples rendering layer from SelectionService implementation.
 * (Option B: Callback/Interface pattern for clean separation)
 *
 * Extends to support overlay rendering needs:
 * - Position-based queries for tile highlighting
 * - Floor-filtered iteration for overlay rendering
 * - Entity type queries for different highlight styles
 */
class ISelectionDataProvider {
public:
  virtual ~ISelectionDataProvider() = default;

  // === Basic Queries ===

  /**
   * Check if selection is empty.
   */
  virtual bool isEmpty() const = 0;

  /**
   * Get total number of selected entries.
   */
  virtual size_t getSelectionCount() const = 0;

  /**
   * Check if a position has any selected entities.
   */
  virtual bool hasSelectionAt(const Domain::Position &pos) const = 0;

  /**
   * Check if a specific item is selected at a position.
   * Returns true if:
   * - The entire tile at pos is selected, OR
   * - The specific item is selected
   */
  virtual bool isItemSelected(const Domain::Position &pos,
                              const Domain::Item *item) const = 0;

  /**
   * Get selection bounding box for early-out optimization.
   * Returns false if selection is empty.
   */
  virtual bool getSelectionBounds(int32_t &min_x, int32_t &min_y,
                                  int16_t &min_z, int32_t &max_x,
                                  int32_t &max_y, int16_t &max_z) const = 0;

  // === Floor-Filtered Iteration (for overlay rendering) ===

  /**
   * Get all selected positions on a specific floor.
   * Used for viewport-based overlay rendering.
   */
  virtual std::vector<Domain::Position>
  getPositionsOnFloor(int16_t floor) const = 0;

  /**
   * Callback type for iterating entries on a floor.
   * Receives position and entry type for rendering decisions.
   */
  using EntryCallback = std::function<void(const Domain::Position &pos,
                                           Domain::Selection::EntityType type)>;

  /**
   * Iterate all entries on a specific floor.
   * Calls callback for each entry with position and type.
   */
  virtual void forEachEntryOnFloor(int16_t floor,
                                   const EntryCallback &callback) const = 0;

  /**
   * Check if any spawn-type entities are selected at a position.
   */
  virtual bool hasSpawnSelectionAt(const Domain::Position &pos) const = 0;

  /**
   * Check if any creature-type entities are selected at a position.
   */
  virtual bool hasCreatureSelectionAt(const Domain::Position &pos) const = 0;
};

} // namespace Rendering
} // namespace MapEditor
