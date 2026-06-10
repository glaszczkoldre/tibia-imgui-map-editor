#pragma once
#include "IPreviewProvider.h"
#include <memory>
#include <optional>

namespace MapEditor::Services::Preview {

/**
 * Central service managing the active preview.
 * 
 * Only one preview can be active at a time (brush OR paste OR drag).
 * Owned by EditorSession, so each map tab has its own preview state.
 * 
 * Usage:
 *   PreviewService service;
 *   service.setProvider(std::make_unique<RawBrushPreviewProvider>(...));
 *   
 *   // Each frame:
 *   if (service.hasPreview()) {
 *       service.updateCursor(mousePos);
 *       renderer.render(service.getPreviewTiles(), ...);
 *   }
 */
class PreviewService {
public:
    PreviewService() = default;
    ~PreviewService() = default;
    
    // Non-copyable, movable
    PreviewService(const PreviewService&) = delete;
    PreviewService& operator=(const PreviewService&) = delete;
    PreviewService(PreviewService&&) = default;
    PreviewService& operator=(PreviewService&&) = default;
    
    /**
     * Set the active preview provider.
     * Replaces any existing provider. Takes ownership.
     * @param provider New preview provider (may be nullptr to clear)
     */
    void setProvider(std::unique_ptr<IPreviewProvider> provider);
    
    /**
     * Clear the current preview.
     * Equivalent to setProvider(nullptr).
     */
    void clearPreview();
    
    /**
     * Check if a preview is currently active.
     * @return true if provider exists and is active
     */
    bool hasPreview() const;
    
    /**
     * Get preview tiles from active provider.
     * @return Reference to tiles, or empty vector if no provider
     */
    const std::vector<PreviewTileData>& getPreviewTiles() const;
    
    /**
     * Get anchor position from active provider.
     * @return Anchor position, or (0,0,0) if no provider
     */
    Domain::Position getAnchorPosition() const;
    
    /**
     * Get bounds from active provider.
     * @return Bounds, or single-tile bounds if no provider
     */
    PreviewBounds getBounds() const;
    
    /**
     * Get style from active provider.
     * @return Style, or Ghost if no provider
     */
    PreviewStyle getStyle() const;
    
    /**
     * Update cursor position on active provider.
     * No-op if no provider.
     * @param cursor World position of cursor
     */
    void updateCursor(const Domain::Position& cursor);

    /**
     * Update fractional position within tile for sub-tile precision.
     * @param fracX Horizontal fraction (0..1)
     * @param fracY Vertical fraction (0..1)
     */
    void updateFractional(float fracX, float fracY);
    
    /**
     * Trigger regeneration on active provider.
     * Used when brush parameters change (size, shape).
     */
    void regenerate();

    /**
     * Get the current deterministic seed from the active provider.
     * Providers that produce preview results deterministically from a seed
     * expose it here so placement code can reproduce the exact preview result.
     * @return Seed value, or std::nullopt if provider doesn't support it
     */
    std::optional<uint32_t> getCurrentSeed() const;

private:
    std::unique_ptr<IPreviewProvider> provider_;
    
    // Empty tiles returned when no provider
    static const std::vector<PreviewTileData> EMPTY_TILES;
};

} // namespace MapEditor::Services::Preview
