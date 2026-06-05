#pragma once
#include "IPreviewProvider.h"

namespace MapEditor {
namespace Brushes {
class WallBrush;
}
namespace Domain {
class ChunkedMap;
}
namespace Services {
class BrushSettingsService;
}
} // namespace MapEditor

namespace MapEditor::Services::Preview {

/**
 * Preview provider for wall brushes.
 * Shows the brush's default preview item (horizontal wall).
 * Actual alignment is handled by the placement system (rebuildAround).
 */
class WallBrushPreviewProvider : public IPreviewProvider {
public:
  WallBrushPreviewProvider(const Brushes::WallBrush *wallBrush,
                           BrushSettingsService *brushSettings,
                           const Domain::ChunkedMap *map = nullptr);

  bool isActive() const override { return wallBrush_ != nullptr; }
  Domain::Position getAnchorPosition() const override { return anchor_; }
  const std::vector<PreviewTileData> &getTiles() const override;
  PreviewBounds getBounds() const override { return bounds_; }
  void updateCursorPosition(const Domain::Position &cursor) override;
  PreviewStyle getStyle() const override;

  bool needsRegeneration() const override { return needsRegen_; }
  void regenerate() override { needsRegen_ = true; }

private:
  std::vector<std::pair<int, int>> getPerimeterOffsets() const;
  void buildPreview() const;

  const Brushes::WallBrush *wallBrush_ = nullptr;
  BrushSettingsService *brushSettings_ = nullptr;
  const Domain::ChunkedMap *map_ = nullptr;

  Domain::Position anchor_{0, 0, 0};
  mutable std::vector<PreviewTileData> tiles_;
  mutable PreviewBounds bounds_;
  mutable bool needsRegen_ = true;
  mutable std::vector<std::pair<int, int>> cachedOffsets_;
};

} // namespace MapEditor::Services::Preview
