/**
 * @file WallNode.cpp
 * @brief Implementation of WallNode weighted selection.
 */

#include "WallNode.h"

namespace MapEditor::Brushes {

void WallNode::addItem(uint32_t itemId, uint32_t chance) {
  items_.emplace_back(itemId, chance);
}

uint32_t WallNode::getRandomItem() const {
  if (items_.empty()) {
    return 0;
  }

  // Calculate total weight
  uint32_t totalWeight = 0;
  for (const auto &[itemId, chance] : items_) {
    totalWeight += chance;
  }

  if (totalWeight == 0) {
    return items_.front().first;
  }

  // Weighted random selection
  std::uniform_int_distribution<uint32_t> dist(1, totalWeight);
  uint32_t roll = dist(rng_);

  uint32_t cumulative = 0;
  for (const auto &[itemId, chance] : items_) {
    cumulative += chance;
    if (roll <= cumulative) {
      return itemId;
    }
  }

  return items_.back().first;
}

} // namespace MapEditor::Brushes
