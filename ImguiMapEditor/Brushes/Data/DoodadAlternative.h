#pragma once
/**
 * @file DoodadAlternative.h
 * @brief Data structures for doodad brush variations and composites.
 */

#include <cstdint>
#include <random>
#include <vector>


namespace MapEditor::Brushes {

/**
 * Single item placement with chance weight.
 */
struct SingleItem {
  uint32_t itemId = 0;
  uint32_t chance = 1;
  uint32_t subtype = 0; // For stackables or items with subtypes
};

/**
 * Composite placement (multi-tile doodad).
 */
struct CompositeItem {
  uint32_t chance = 1;

  /**
   * Tile offset within the composite.
   */
  struct TileOffset {
    int dx = 0;
    int dy = 0;
    int dz = 0;
    std::vector<SingleItem> items;
  };

  std::vector<TileOffset> tiles;
};

/**
 * One variation of a doodad brush.
 * Can contain single items and/or composite (multi-tile) items.
 */
class DoodadAlternative {
public:
  /**
   * Add a single item to this alternative.
   */
  void addSingleItem(SingleItem item);

  /**
   * Add a composite to this alternative.
   */
  void addComposite(CompositeItem composite);

  /**
   * Check if this alternative has any content.
   */
  bool hasContent() const;

  /**
   * Check if this alternative uses composites.
   */
  bool isComposite() const { return !composites_.empty(); }

  /**
   * Get total chance weight for selection.
   */
  uint32_t getTotalChance() const;

  /**
   * Select a random single item using weighted selection.
   * @return The selected item, or a default SingleItem if empty
   */
  SingleItem selectRandomSingle() const;

  /**
   * Select a random composite using weighted selection.
   * @return Pointer to the selected composite, or nullptr if empty
   */
  const CompositeItem *selectRandomComposite() const;

  const std::vector<SingleItem> &getSingleItems() const { return singles_; }
  const std::vector<CompositeItem> &getComposites() const {
    return composites_;
  }

private:
  std::vector<SingleItem> singles_;
  std::vector<CompositeItem> composites_;
  mutable std::mt19937 rng_{std::random_device{}()};
};

} // namespace MapEditor::Brushes
