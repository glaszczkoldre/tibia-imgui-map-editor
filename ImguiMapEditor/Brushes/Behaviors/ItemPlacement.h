#pragma once

#include <cstdint>
#include <memory>

namespace MapEditor::Domain {
    class ChunkedMap;
    class Tile;
    class Item;
    struct Position;
    struct ItemType;
}

namespace MapEditor::Brushes {

class IBrush;  // Forward declaration for ownership

/**
 * Utility class for placing and removing items on tiles.
 * 
 * Separates tile modification logic from brush logic to:
 * - Enable code reuse across brush types
 * - Centralize item placement rules
 * - Simplify testing
 */
class ItemPlacement {
public:
    /**
     * Place a single item on a tile.
     * Creates the tile if it doesn't exist.
     * 
     * @param map The map to modify
     * @param pos Target position
     * @param itemId Server item ID
     * @param subtype Optional subtype (count/fluid type, 0 = use default)
     * @return The placed item, or nullptr on failure
     */
    static Domain::Item* placeItem(
        Domain::ChunkedMap& map,
        const Domain::Position& pos,
        uint32_t itemId,
        uint16_t subtype = 0
    );
    
    /**
     * Remove all items with a specific server ID from a tile.
     * 
     * @param tile The tile to modify
     * @param itemId Server item ID to remove
     * @return Number of items removed
     */
    static size_t removeItemsById(Domain::Tile* tile, uint32_t itemId);
    
    /**
     * Remove all items owned by a specific brush.
     * Calls brush->ownsItem() for each item.
     * 
     * @param tile The tile to modify
     * @param brush The brush to check ownership
     * @return Number of items removed
     */
    static size_t removeItemsByBrush(Domain::Tile* tile, const IBrush* brush);
};

} // namespace MapEditor::Brushes
