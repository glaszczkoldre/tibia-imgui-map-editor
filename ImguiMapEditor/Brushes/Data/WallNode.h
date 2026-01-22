#pragma once
/**
 * @file WallNode.h
 * @brief Data structures for wall and door item storage.
 */

#include "../Enums/BrushEnums.h"
#include <cstdint>
#include <random>
#include <vector>


namespace MapEditor::Brushes {

/**
 * Stores wall items for a specific wall alignment type.
 */
class WallNode {
public:
  /**
   * Add an item with a chance weight.
   */
  void addItem(uint32_t itemId, uint32_t chance);

  /**
   * Check if this node has any items.
   */
  bool hasItems() const { return !items_.empty(); }

  /**
   * Get a random item using weighted selection.
   */
  uint32_t getRandomItem() const;

  /**
   * Get all items.
   */
  const std::vector<std::pair<uint32_t, uint32_t>> &getItems() const {
    return items_;
  }

private:
  std::vector<std::pair<uint32_t, uint32_t>> items_; // (itemId, chance)
  mutable std::mt19937 rng_{std::random_device{}()};
};

/**
 * Stores door items by door type and wall alignment.
 */
struct DoorNode {
  DoorType type = DoorType::Undefined;
  WallAlign alignment = WallAlign::Pole;
  std::vector<uint32_t> items;
  bool isOpen = false;
  bool isLocked = false;

  /**
   * Get the first door item (doors typically have one item).
   */
  uint32_t getItem() const { return items.empty() ? 0 : items.front(); }
};

} // namespace MapEditor::Brushes
