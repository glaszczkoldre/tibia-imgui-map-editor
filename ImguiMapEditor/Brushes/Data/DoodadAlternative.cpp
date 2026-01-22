/**
 * @file DoodadAlternative.cpp
 * @brief Implementation of DoodadAlternative weighted selection.
 */

#include "DoodadAlternative.h"

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

  uint32_t totalWeight = 0;
  for (const auto &item : singles_) {
    totalWeight += item.chance;
  }

  if (totalWeight == 0) {
    return singles_.front();
  }

  std::uniform_int_distribution<uint32_t> dist(1, totalWeight);
  uint32_t roll = dist(rng_);

  uint32_t cumulative = 0;
  for (const auto &item : singles_) {
    cumulative += item.chance;
    if (roll <= cumulative) {
      return item;
    }
  }

  return singles_.back();
}

const CompositeItem *DoodadAlternative::selectRandomComposite() const {
  if (composites_.empty()) {
    return nullptr;
  }

  uint32_t totalWeight = 0;
  for (const auto &comp : composites_) {
    totalWeight += comp.chance;
  }

  if (totalWeight == 0) {
    return &composites_.front();
  }

  std::uniform_int_distribution<uint32_t> dist(1, totalWeight);
  uint32_t roll = dist(rng_);

  uint32_t cumulative = 0;
  for (const auto &comp : composites_) {
    cumulative += comp.chance;
    if (roll <= cumulative) {
      return &comp;
    }
  }

  return &composites_.back();
}

} // namespace MapEditor::Brushes
