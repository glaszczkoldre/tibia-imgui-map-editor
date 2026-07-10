#include "DoodadBrushPreviewProvider.h"

#include "Brushes/Types/DoodadBrush.h"
#include "Brushes/Types/DoodadPlacementPlanner.h"
#include "Services/BrushSettingsService.h"
#include "Utils/HashUtils.h"
#include <random>

namespace MapEditor::Services::Preview {

namespace {
uint32_t randomUint32() {
  std::random_device rd;
  return rd();
}
} // namespace

DoodadBrushPreviewProvider::DoodadBrushPreviewProvider(
    const Brushes::DoodadBrush &brush, const BrushSettingsService *brushSettings,
    const Domain::ChunkedMap *map)
    : brush_(brush), brushSettings_(brushSettings), map_(map),
      previewNonce_(randomUint32()) {
  buildPreview();
}

bool DoodadBrushPreviewProvider::isActive() const { return true; }

Domain::Position DoodadBrushPreviewProvider::getAnchorPosition() const {
  return anchor_;
}

bool DoodadBrushPreviewProvider::checkSettingsChanged() const {
  if (brush_.getVariation() != lastVariation_) {
    return true;
  }
  if (!brushSettings_) {
    return false;
  }

  const auto offsets = brushSettings_->getBrushOffsets();
  return offsets != cachedOffsets_;
}

const std::vector<PreviewTileData> &DoodadBrushPreviewProvider::getTiles() const {
  if (checkSettingsChanged()) {
    needsRegen_ = true;
  }

  if (needsRegen_) {
    buildPreview();
  }

  return tiles_;
}

PreviewBounds DoodadBrushPreviewProvider::getBounds() const { return bounds_; }

PreviewStyle DoodadBrushPreviewProvider::getStyle() const {
  return brushSettings_ && brushSettings_->getPreviewBorder()
             ? PreviewStyle::Outline
             : PreviewStyle::Ghost;
}

void DoodadBrushPreviewProvider::updateCursorPosition(const Domain::Position &cursor) {
  if (anchor_ == cursor) {
    return;
  }
  anchor_ = cursor;
  needsRegen_ = true;
}

uint32_t DoodadBrushPreviewProvider::buildStableSeed() const {
  uint32_t seed = Utils::kFnvOffsetBasis;
  Utils::mixSeed(seed, brush_.getName());
  Utils::mixSeed(seed, previewNonce_);
  Utils::mixSeed(seed, static_cast<uint32_t>(brush_.getVariation()));
  return seed;
}

void DoodadBrushPreviewProvider::regenerate() {
  previewNonce_ = randomUint32();
  needsRegen_ = true;
}

void DoodadBrushPreviewProvider::buildPreview() const {
  currentSeed_ = buildStableSeed();
  lastVariation_ = brush_.getVariation();
  tiles_ = brush_.buildPreviewTiles(anchor_, brushSettings_, map_, currentSeed_);
  bounds_ = PreviewBounds();
  needsRegen_ = false;
  cachedOffsets_ = brushSettings_ ? brushSettings_->getBrushOffsets()
                                  : std::vector<std::pair<int, int>>{{0, 0}};

  for (const auto &tile : tiles_) {
    bounds_.expand(tile.relativePosition);
  }
}

std::optional<uint32_t> DoodadBrushPreviewProvider::getCurrentSeed() const {
  return currentSeed_;
}

} // namespace MapEditor::Services::Preview
