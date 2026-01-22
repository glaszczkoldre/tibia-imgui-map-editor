#pragma once
/**
 * @file BorderBlock.h
 * @brief Data structures for border item storage and specific case handling.
 */

#include "../Enums/BrushEnums.h"
#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <vector>


namespace MapEditor::Brushes {

/**
 * Stores border items for each edge type.
 * Used by GroundBrush for border generation.
 */
class BorderBlock {
public:
  static constexpr size_t kEdgeTypeCount =
      14; // EdgeType::None through EdgeType::Center

  /**
   * Add an item for a specific edge type.
   * @param edge The edge type
   * @param itemId The item ID to place
   * @param chance Weight for random selection (higher = more likely)
   */
  void addItem(EdgeType edge, uint32_t itemId, uint32_t chance);

  /**
   * Check if this block has items for a specific edge type.
   */
  bool hasItemsFor(EdgeType edge) const;

  /**
   * Get a random item for a specific edge type using weighted selection.
   * @return Item ID, or 0 if no items available
   */
  uint32_t getRandomItem(EdgeType edge) const;

  /**
   * Get all items for a specific edge type.
   */
  const std::vector<std::pair<uint32_t, uint32_t>> &
  getItems(EdgeType edge) const;

  /**
   * Set the owner brush name (for z-order comparison).
   */
  void setOwnerBrush(const std::string &name) { ownerBrush_ = name; }
  const std::string &getOwnerBrush() const { return ownerBrush_; }

  /**
   * Set ground equivalent ID for optional borders.
   */
  void setGroundEquivalent(uint32_t id) { groundEquivalent_ = id; }
  uint32_t getGroundEquivalent() const { return groundEquivalent_; }

private:
  // Items for each of 14 edge types: vector of (itemId, chance)
  std::array<std::vector<std::pair<uint32_t, uint32_t>>, kEdgeTypeCount> items_;

  // Reference to owner brush (for z-order comparison)
  std::string ownerBrush_;

  // Ground equivalent for optional borders
  uint32_t groundEquivalent_ = 0;

  // Random number generator for weighted selection
  mutable std::mt19937 rng_{std::random_device{}()};
};

/**
 * Condition for specific border case matching.
 */
struct SpecificCaseCondition {
  EdgeType edge = EdgeType::None; // Edge to check
  std::string matchBrush;         // Brush name to match (empty = any)
  bool matchEmpty = false;        // If true, match when no border present
};

/**
 * Action to perform when specific case matches.
 */
struct SpecificCaseAction {
  EdgeType edge = EdgeType::None; // Edge to modify
  uint32_t itemId = 0;            // Item to place
  bool keepBorder = false;        // If true, keep existing border
};

/**
 * Specific case block for complex border conditions.
 * Used by GroundBrush to handle special border situations.
 */
class SpecificCaseBlock {
public:
  void addCondition(SpecificCaseCondition condition);
  void addAction(SpecificCaseAction action);

  const std::vector<SpecificCaseCondition> &getConditions() const {
    return conditions_;
  }
  const std::vector<SpecificCaseAction> &getActions() const { return actions_; }

  /**
   * Check if this specific case matches the given context.
   * (Implementation depends on map context - declared here for interface)
   */
  // bool matches(const NeighborContext& context) const;

private:
  std::vector<SpecificCaseCondition> conditions_;
  std::vector<SpecificCaseAction> actions_;
};

} // namespace MapEditor::Brushes
