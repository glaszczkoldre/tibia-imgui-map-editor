#pragma once

#include "IPreviewProvider.h"
#include <optional>

namespace MapEditor::Brushes {
class DoodadBrush;
}

namespace MapEditor::Domain {
class ChunkedMap;
}

namespace MapEditor::Services {
class BrushSettingsService;
}

namespace MapEditor::Services::Preview {

class DoodadBrushPreviewProvider : public IPreviewProvider {
public:
  DoodadBrushPreviewProvider(const Brushes::DoodadBrush &brush,
                             const BrushSettingsService *brushSettings = nullptr,
                             const Domain::ChunkedMap *map = nullptr);

  bool isActive() const override;
  Domain::Position getAnchorPosition() const override;
  const std::vector<PreviewTileData> &getTiles() const override;
  PreviewBounds getBounds() const override;
  PreviewStyle getStyle() const override;
  void updateCursorPosition(const Domain::Position &cursor) override;
  void regenerate() override;
  std::optional<uint32_t> getCurrentSeed() const override;

private:
  bool checkSettingsChanged() const;
  void buildPreview() const;
  uint32_t buildStableSeed() const;

  const Brushes::DoodadBrush &brush_;
  const BrushSettingsService *brushSettings_ = nullptr;
  const Domain::ChunkedMap *map_ = nullptr;
  Domain::Position anchor_{0, 0, 0};
  mutable std::vector<PreviewTileData> tiles_;
  mutable PreviewBounds bounds_;
  mutable bool needsRegen_ = true;
  mutable std::vector<std::pair<int, int>> cachedOffsets_;
  mutable uint32_t previewNonce_ = 0;
  mutable uint32_t currentSeed_ = 0;
};

} // namespace MapEditor::Services::Preview
