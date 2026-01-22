#include "PreviewService.h"
namespace MapEditor::Services::Preview {

// Static empty tiles for null provider case
const std::vector<PreviewTileData> PreviewService::EMPTY_TILES;

void PreviewService::setProvider(std::unique_ptr<IPreviewProvider> provider) {
    provider_ = std::move(provider);
}

void PreviewService::clearPreview() {
    provider_.reset();
}

bool PreviewService::hasPreview() const {
    return provider_ && provider_->isActive();
}

IPreviewProvider* PreviewService::getProvider() const {
    return provider_.get();
}

const std::vector<PreviewTileData>& PreviewService::getPreviewTiles() const {
    if (provider_) {
        return provider_->getTiles();
    }
    return EMPTY_TILES;
}

Domain::Position PreviewService::getAnchorPosition() const {
    if (provider_) {
        return provider_->getAnchorPosition();
    }
    return Domain::Position{0, 0, 0};
}

PreviewBounds PreviewService::getBounds() const {
    if (provider_) {
        return provider_->getBounds();
    }
    return PreviewBounds::fromSingle();
}

PreviewStyle PreviewService::getStyle() const {
    if (provider_) {
        return provider_->getStyle();
    }
    return PreviewStyle::Ghost;
}

void PreviewService::updateCursor(const Domain::Position& cursor) {
    if (provider_) {
        provider_->updateCursorPosition(cursor);
    }
}

void PreviewService::regenerate() {
    if (provider_ && provider_->needsRegeneration()) {
        provider_->regenerate();
    }
}

} // namespace MapEditor::Services::Preview
