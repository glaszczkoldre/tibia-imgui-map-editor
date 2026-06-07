/**
 * @file DoodadAlternative.cpp
 * @brief Implementation of DoodadAlternative weighted selection.
 */

#include "DoodadAlternative.h"

#include "Brushes/Behaviors/WeightedSelection.h"
#include <utility>

namespace MapEditor::Brushes {

void DoodadAlternative::addSingleItem(SingleItem item) {
  singles_.push_back(item);
}

void DoodadAlternative::addComposite(CompositeItem composite) {
  composites_.push_back(std::move(composite));
}

bool DoodadAlternative::hasContent() const {
  return !singles_.empty() || !composites_.empty();
}

uint32_t DoodadAlternative::getTotalChance() const {
  uint32_t total = 0;
  for (const auto &item : singles_) {
    total += item.chance;
  }
  for (const auto &comp : composites_) {
    total += comp.chance;
  }
  return total;
}

SingleItem DoodadAlternative::selectRandomSingle() const {
  if (singles_.empty()) {
    return SingleItem{};
  }

  std::vector<uint32_t> weights;
  weights.reserve(singles_.size());
  for (const auto &item : singles_) {
    weights.push_back(item.chance);
  }

  const auto selected = WeightedSelection::select(weights);
  return selected ? singles_[*selected] : singles_.front();
}

const CompositeItem *DoodadAlternative::selectRandomComposite() const {
  if (composites_.empty()) {
    return nullptr;
  }

  std::vector<uint32_t> weights;
  weights.reserve(composites_.size());
  for (const auto &comp : composites_) {
    weights.push_back(comp.chance);
  }

  const auto selected = WeightedSelection::select(weights);
  return selected ? &composites_[*selected] : &composites_.front();
}

} // namespace MapEditor::Brushes
