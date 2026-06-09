/**
 * @file WallNode.cpp
 * @brief Implementation of WallNode weighted selection.
 */

#include "WallNode.h"
#include "Brushes/Behaviors/WeightedSelection.h"

namespace MapEditor::Brushes {

void WallNode::addItem(uint32_t itemId, uint32_t chance) {
  items_.emplace_back(itemId, chance);
}

uint32_t WallNode::getRandomItem() const {
  if (items_.empty()) {
    return 0;
  }

  std::vector<uint32_t> weights;
  weights.reserve(items_.size());
  for (const auto &[itemId, chance] : items_) {
    weights.push_back(chance);
  }

  const auto selected = WeightedSelection::select(weights);
  return selected ? items_[*selected].first : items_.front().first;
}

} // namespace MapEditor::Brushes
