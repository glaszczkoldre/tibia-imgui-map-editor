#pragma once
#include "IPreviewProvider.h"

namespace MapEditor {
namespace Services {
class BrushSettingsService;
} // namespace Services
} // namespace MapEditor

namespace MapEditor {
namespace Services {
namespace Preview {

/**
 * Preview provider for RAW brush (item placement).
 *
 * Generates preview tiles based on current brush settings:
 * - Uses BrushSettingsService to get brush positions
 * - Each position gets a copy of the item
 * - Supports regeneration when brush size/shape changes
 */
class RawBrushPreviewProvider : public IPreviewProvider {
public:
  /**
   * Create RAW brush preview.
   * @param itemId Server ID of item to preview
   * @param subtype Optional subtype (stack count, fluid type)
   * @param brushSettings Brush settings service for size/shape (optional)
   */
  explicit RawBrushPreviewProvider(
      uint32_t itemId, uint16_t subtype = 0,
      const BrushSettingsService *brushSettings = nullptr);

  // IPreviewProvider interface
  bool isActive() const override;
  Domain::Position getAnchorPosition() const override;
  const std::vector<PreviewTileData> &getTiles() const override;
  PreviewBounds getBounds() const override;
  void updateCursorPosition(const Domain::Position &cursor) override;
  PreviewStyle getStyle() const override;

  // Regeneration support for brush size changes
  void regenerate() override;

private:
  uint32_t itemId_;
  uint16_t subtype_;
  const BrushSettingsService *brushSettings_ = nullptr;
  Domain::Position anchor_{0, 0, 0};
  mutable std::vector<PreviewTileData> tiles_;
  mutable PreviewBounds bounds_;
  mutable bool needsRegen_ = false;

  // Cached settings for change detection (actual offsets, not just count)
  mutable std::vector<std::pair<int, int>> cachedOffsets_;

  void buildPreview() const;
  bool checkSettingsChanged() const;
};

} // namespace Preview
} // namespace Services
} // namespace MapEditor
