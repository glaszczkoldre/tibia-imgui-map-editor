#pragma once
#include "IPreviewProvider.h"
#include "Domain/CopyBuffer.h"
namespace MapEditor::Services::Preview {

/**
 * Preview provider for paste operations.
 * 
 * Converts CopiedTile data to PreviewTileData format.
 * Preserves relative positions and Z-offsets from source tiles.
 */
class PastePreviewProvider : public IPreviewProvider {
public:
    /**
     * Create paste preview from copied tiles.
     * @param copiedTiles Reference to copied tiles (must outlive provider)
     */
    explicit PastePreviewProvider(const std::vector<Domain::CopyBuffer::CopiedTile>& copiedTiles);
    
    // IPreviewProvider interface
    bool isActive() const override;
    Domain::Position getAnchorPosition() const override;
    const std::vector<PreviewTileData>& getTiles() const override;
    PreviewBounds getBounds() const override;
    void updateCursorPosition(const Domain::Position& cursor) override;

private:
    const std::vector<Domain::CopyBuffer::CopiedTile>* copiedTiles_;
    Domain::Position anchor_{0, 0, 0};
    std::vector<PreviewTileData> tiles_;
    PreviewBounds bounds_;
    
    void buildPreview();
};

} // namespace MapEditor::Services::Preview
