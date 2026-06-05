#pragma once

#include "IPreviewProvider.h"
#include "Services/Autoborder/AutoborderEngine.h"

namespace MapEditor {
namespace Brushes {
class IBrush;
}
namespace Domain {
class ChunkedMap;
}
namespace Services {
class BrushSettingsService;
}
} // namespace MapEditor

namespace MapEditor::Services::Preview {

class AutoborderBrushPreviewProvider final : public IPreviewProvider {
public:
  AutoborderBrushPreviewProvider(const Brushes::IBrush *brush,
                                 BrushSettingsService *brushSettings,
                                 const Domain::ChunkedMap *map);

  bool isActive() const override;
  Domain::Position getAnchorPosition() const override { return anchor_; }
  const std::vector<PreviewTileData> &getTiles() const override;
  PreviewBounds getBounds() const override { return bounds_; }
  void updateCursorPosition(const Domain::Position &cursor) override;
  PreviewStyle getStyle() const override;

  bool needsRegeneration() const override { return needsRegen_; }
  void regenerate() override { needsRegen_ = true; }

private:
  void buildPreview() const;
  std::vector<Domain::Position> getPlacementPositions() const;

  const Brushes::IBrush *brush_ = nullptr;
  BrushSettingsService *brushSettings_ = nullptr;
  const Domain::ChunkedMap *map_ = nullptr;
  Autoborder::AutoborderEngine engine_;

  Domain::Position anchor_{0, 0, 0};
  mutable std::vector<PreviewTileData> tiles_;
  mutable PreviewBounds bounds_;
  mutable bool needsRegen_ = true;
  mutable std::vector<std::pair<int, int>> cachedOffsets_;
};

} // namespace MapEditor::Services::Preview
