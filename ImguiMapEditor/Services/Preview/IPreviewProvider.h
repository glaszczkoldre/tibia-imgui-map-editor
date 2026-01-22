#pragma once
#include "PreviewTypes.h"
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
     * Get the preview style for rendering.
     * Default is Ghost (semi-transparent blue tint).
     */
    virtual PreviewStyle getStyle() const { return PreviewStyle::Ghost; }
    
    /**
     * Check if preview needs regeneration after parameter change.
     * Default returns false (static preview data).
     */
    virtual bool needsRegeneration() const { return false; }
    
    /**
     * Regenerate preview tiles if content depends on parameters.
     * Called when brush size/shape changes for example.
     */
    virtual void regenerate() {}
};

} // namespace MapEditor::Services::Preview
