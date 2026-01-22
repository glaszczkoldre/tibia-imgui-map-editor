#pragma once

#include <cstdint>
#include <string>

namespace MapEditor::Domain {
class ChunkedMap;
class Tile;
class Item;
struct Position;
} // namespace MapEditor::Domain

namespace MapEditor::Services {
class BrushSettingsService;
} // namespace MapEditor::Services

namespace MapEditor::Brushes {

// ============================================================================
// Brush Type Enumeration
// ============================================================================

/**
 * Identifies the type of brush for filtering and specialized handling.
 * Extended to include all RME brush types for migration compatibility.
 */
enum class BrushType {
  Raw,            // Single item by ID
  Doodad,         // Decorations with variations/composites
  Ground,         // Ground tiles with auto-bordering
  Wall,           // Wall tiles with alignment
  WallDecoration, // Wall overlay decorations
  Table,          // Table-like objects with alignment
  Carpet,         // Carpet tiles with alignment
  Door,           // Door items (subset of wall)
  Creature,       // Creature placement
  Spawn,          // Spawn point placement
  House,          // House zone assignment
  HouseExit,      // House exit point
  Waypoint,       // Named waypoints
  Flag,           // Zone flags (PZ, noPVP, etc.)
  OptionalBorder, // Gravel/mountain optional borders
  Eraser,         // Removes items
  Placeholder     // For missing/undefined brushes
};

// ============================================================================
// Draw Context
// ============================================================================

/**
 * Parameters passed to brush draw operations.
 */
struct DrawContext {
  int variation = 0;       // Which size/variant to use (for alternates)
  bool isDragging = false; // Part of a drag stroke
  bool forcePlace = false; // Ignore blocking/duplicate checks
  Services::BrushSettingsService *brushSettings = nullptr; // For spawn settings
};

// ============================================================================
// IBrush Interface
// ============================================================================

/**
 * Abstract interface for all brush types.
 *
 * All brushes implement this interface, allowing BrushController to handle
 * them polymorphically. This replaces the old BrushDefinition struct approach.
 *
 * RME Compatibility:
 * - draw() matches RME's Brush::draw()
 * - undraw() matches RME's Brush::undraw()
 * - ownsItem() supports brush-specific erasing behavior
 */
class IBrush {
public:
  virtual ~IBrush() = default;

  // ─── Identity ─────────────────────────────────────────────────────────

  /**
   * Get the brush name (used for lookup and display).
   */
  virtual const std::string &getName() const = 0;

  /**
   * Get the brush type for filtering.
   */
  virtual BrushType getType() const = 0;

  /**
   * Get the preview sprite ID.
   */
  virtual uint32_t getLookId() const = 0;

  // ─── Capabilities ─────────────────────────────────────────────────────

  /**
   * Check if the brush can draw at the given position.
   * Override to implement blocking checks, etc.
   */
  virtual bool canDraw(const Domain::ChunkedMap &map,
                       const Domain::Position &pos) const {
    return true;
  }

  /**
   * Whether the brush supports drag-painting.
   */
  virtual bool isDraggable() const { return true; }

  /**
   * Whether placing this brush should trigger border recalculation.
   */
  virtual bool needsBorderUpdate() const { return false; }

  // ─── Variations (for brushes with alternates) ─────────────────────────

  /**
   * Get the number of size variations available.
   * Returns 1 for brushes without alternates.
   */
  virtual size_t getMaxVariation() const { return 1; }

  /**
   * Set the current variation index.
   */
  virtual void setVariation(size_t /*index*/) {}

  // ─── Core Operations ──────────────────────────────────────────────────

  /**
   * Draw the brush at the given tile.
   * This is called when the user left-clicks or drags.
   *
   * @param map The map being modified
   * @param tile The target tile
   * @param ctx Draw parameters (variation, dragging state, etc.)
   */
  virtual void draw(Domain::ChunkedMap &map, Domain::Tile *tile,
                    const DrawContext &ctx) = 0;

  /**
   * Undraw (erase) the brush from the given tile.
   * This is called when the user right-clicks.
   * The brush should remove items it "owns" from the tile.
   *
   * @param map The map being modified
   * @param tile The target tile
   */
  virtual void undraw(Domain::ChunkedMap &map, Domain::Tile *tile) = 0;

  // ─── Ownership (for undraw-by-brush-type) ─────────────────────────────

  /**
   * Check if this brush "owns" the given item.
   * Used by undraw() to determine which items to remove.
   *
   * For raw brushes: item ID matches brush's item ID.
   */
  virtual bool ownsItem(const Domain::Item *item) const { return false; }
};

} // namespace MapEditor::Brushes
