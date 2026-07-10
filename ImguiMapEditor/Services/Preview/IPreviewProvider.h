#pragma once
#include "PreviewTypes.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace MapEditor::Services::Preview {

/**
 * Abstract interface for preview data providers.
 * 
 * Each provider generates preview tiles for a specific use case:
 * - RawBrushPreviewProvider: Single item RAW brush
 * - PastePreviewProvider: Copied tiles for paste operation  
 * - DragPreviewProvider: Selected items being dragged
 * 
 * Providers are owned by PreviewService and swapped as context changes.
 */
class IPreviewProvider {
public:
    virtual ~IPreviewProvider() = default;
    
    /**
     * Check if this provider has valid preview data.
     * @return true if getTiles() will return meaningful data
     */
    virtual bool isActive() const = 0;
    
    /**
     * Get the world position this preview is anchored to.
     * Usually the cursor/mouse tile position.
     * All tile positions in getTiles() are relative to this anchor.
     */
    virtual Domain::Position getAnchorPosition() const = 0;
    
    /**
     * Get all preview tiles.
     * Positions are relative to anchor (0,0,0 = anchor tile).
     * @return Const reference to vector of preview tiles
     */
    virtual const std::vector<PreviewTileData>& getTiles() const = 0;
    
    /**
     * Get bounding box of all preview tiles (for viewport culling).
     * Bounds are relative to anchor position.
     */
    virtual PreviewBounds getBounds() const = 0;
    
    /**
     * Update the cursor/anchor position.
     * Called every frame when mouse moves over viewport.
     * @param cursor New world position of cursor
     */
    virtual void updateCursorPosition(const Domain::Position& cursor) = 0;

    /**
     * Update the fractional position within the tile (0..1 range).
     * Used by wall brush preview for sub-tile alignment detection.
     * Default is no-op. Override to use.
     * @param fracX Horizontal fraction (0=left edge, 1=right edge)
     * @param fracY Vertical fraction (0=top edge, 1=bottom edge)
     */
    virtual void updateFractionalPosition(float fracX, float fracY) {}

    /**
     * Get the preview style for rendering.
     * Default is Ghost (semi-transparent blue tint).
     */
    virtual PreviewStyle getStyle() const { return PreviewStyle::Ghost; }
    

    
    /**
     * Regenerate preview tiles if content depends on parameters.
     * Called when brush size/shape changes for example.
     */
    virtual void regenerate() {}

    /**
     * Get the current deterministic seed used by the preview.
     * Providers that use seeded randomness should return the seed
     * so that the placement operation produces the exact same result
     * as the preview.
     * @return Seed value, or std::nullopt if not applicable
     */
    virtual std::optional<uint32_t> getCurrentSeed() const { return std::nullopt; }
};

} // namespace MapEditor::Services::Preview
