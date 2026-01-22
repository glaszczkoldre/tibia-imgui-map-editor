#pragma once
#include "IPreviewProvider.h"
#include <cstdint>

namespace MapEditor {
namespace Services {
class BrushSettingsService;
} // namespace Services
} // namespace MapEditor

namespace MapEditor::Services::Preview {

/**
 * Preview provider for zone brushes (Flag, Eraser, House, Waypoint).
 * Shows colored overlay squares based on brush size/shape.
 */
class ZoneBrushPreviewProvider : public IPreviewProvider {
public:
  /**
   * Create zone brush preview with specified overlay color.
   * @param color ARGB color for overlay (e.g., 0x8000FF00 for semi-transparent
   * green)
   * @param brushSettings Brush settings service for size/shape
   */
  explicit ZoneBrushPreviewProvider(uint32_t color,
                                    BrushSettingsService *brushSettings);

  // IPreviewProvider interface
  bool isActive() const override;
  Domain::Position getAnchorPosition() const override;
  const std::vector<PreviewTileData> &getTiles() const override;
  PreviewBounds getBounds() const override;
  void updateCursorPosition(const Domain::Position &cursor) override;

  bool needsRegeneration() const override { return needsRegen_; }
  void regenerate() override;

private:
  uint32_t color_;
  BrushSettingsService *brushSettings_ = nullptr;
  Domain::Position anchor_{0, 0, 0};
  std::vector<PreviewTileData> tiles_;
  PreviewBounds bounds_;
  mutable bool needsRegen_ = false;

  mutable std::vector<std::pair<int, int>> cachedOffsets_;

  void buildPreview();
  bool checkSettingsChanged() const;
};

} // namespace MapEditor::Services::Preview
