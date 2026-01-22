#include "PastePreviewProvider.h"
namespace MapEditor::Services::Preview {

PastePreviewProvider::PastePreviewProvider(
    const std::vector<Domain::CopyBuffer::CopiedTile>& copiedTiles)
    : copiedTiles_(&copiedTiles) {
    buildPreview();
}

bool PastePreviewProvider::isActive() const {
    return copiedTiles_ != nullptr && !copiedTiles_->empty();
}

Domain::Position PastePreviewProvider::getAnchorPosition() const {
    return anchor_;
}

const std::vector<PreviewTileData>& PastePreviewProvider::getTiles() const {
    return tiles_;
}

PreviewBounds PastePreviewProvider::getBounds() const {
    return bounds_;
}

void PastePreviewProvider::updateCursorPosition(const Domain::Position& cursor) {
    anchor_ = cursor;
}

void PastePreviewProvider::buildPreview() {
    tiles_.clear();
    bounds_ = PreviewBounds{};
    
    if (!copiedTiles_ || copiedTiles_->empty()) {
        return;
    }
    
    bool first = true;
    
    for (const auto& ct : *copiedTiles_) {
        if (!ct.tile) continue;
        
        PreviewTileData previewTile;
        previewTile.relativePosition = ct.relative_pos;
        
        // Add ground if present
        if (ct.tile->hasGround()) {
            const auto* ground = ct.tile->getGround();
            if (ground) {
                previewTile.addItem(ground->getServerId(), 
                                   static_cast<uint16_t>(ground->getSubtype()));
            }
        }
        
        // Add all items
        for (const auto& itemPtr : ct.tile->getItems()) {
            if (itemPtr) {
                previewTile.addItem(itemPtr->getServerId(),
                                   static_cast<uint16_t>(itemPtr->getSubtype()));
            }
        }
        
        // Add creature if present (store name for lookup at render time)
        if (ct.tile->hasCreature()) {
            const auto* creature = ct.tile->getCreature();
            if (creature) {
                previewTile.creature_name = creature->name;
            }
        }
        
        // Add spawn if present
        if (ct.tile->hasSpawn()) {
            previewTile.has_spawn = true;
        }
        
        if (!previewTile.empty()) {
            // Update bounds
            if (first) {
                bounds_.minX = bounds_.maxX = ct.relative_pos.x;
                bounds_.minY = bounds_.maxY = ct.relative_pos.y;
                bounds_.minZ = bounds_.maxZ = ct.relative_pos.z;
                first = false;
            } else {
                bounds_.expand(ct.relative_pos);
            }
            
            tiles_.push_back(std::move(previewTile));
        }
    }
}

} // namespace MapEditor::Services::Preview
