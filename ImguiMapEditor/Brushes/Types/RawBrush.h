#pragma once
#include "Brushes/Core/BrushBase.h"
namespace MapEditor::Domain {
    class ItemType;
}

namespace MapEditor::Brushes {

/**
 * RAW Brush - Places a single item by ID.
 * 
 * The simplest brush type, used for direct item placement
 * from the palette. Matches RME's RAWBrush behavior.
 * 
 * Features:
 * - draw(): Adds item to tile
 * - undraw(): Removes items with matching ID
 * - ownsItem(): Checks if item has matching ID
 */
class RawBrush : public BrushBase {
public:
    /**
     * Construct a raw brush for the given item ID.
     * 
     * @param itemId Server item ID to place
     * @param type Optional cached ItemType for efficiency
     */
    explicit RawBrush(uint32_t itemId, const Domain::ItemType* type = nullptr);
    
    // ─── IBrush Implementation ────────────────────────────────────────────
    
    BrushType getType() const override { return BrushType::Raw; }
    
    void draw(Domain::ChunkedMap& map, 
              Domain::Tile* tile,
              const DrawContext& ctx) override;
    
    void undraw(Domain::ChunkedMap& map, 
                Domain::Tile* tile) override;
    
    bool ownsItem(const Domain::Item* item) const override;
    
    // ─── RawBrush Specific ────────────────────────────────────────────────
    
    /**
     * Get the item ID this brush places.
     */
    uint32_t getItemId() const { return itemId_; }
    
    /**
     * Get the cached ItemType, if available.
     */
    const Domain::ItemType* getCachedType() const { return cachedType_; }
    
    /**
     * Set the cached ItemType for efficiency.
     */
    void setCachedType(const Domain::ItemType* type) { cachedType_ = type; }
    
private:
    uint32_t itemId_;
    const Domain::ItemType* cachedType_;
};

} // namespace MapEditor::Brushes
