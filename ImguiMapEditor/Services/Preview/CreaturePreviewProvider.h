#pragma once
#include "IPreviewProvider.h"

namespace MapEditor::Services {
class BrushSettingsService;
} // namespace MapEditor::Services

namespace MapEditor::Services::Preview {

/**
 * Preview provider for Creature brush placement.
 *
 * Generates preview tiles showing creature names at brush positions:
 * - Uses BrushSettingsService to get brush positions
 * - Each position gets the creature name for the preview overlay
 * - Supports regeneration when brush size/shape changes
 */
class CreaturePreviewProvider : public IPreviewProvider {
public:
  /**
   * Create creature brush preview.
   * @param creatureName Name of creature to preview
   * @param brushSettings Brush settings service for size/shape (optional)
   */
  explicit CreaturePreviewProvider(
      const std::string &creatureName,
      const BrushSettingsService *brushSettings = nullptr);

  // IPreviewProvider interface
  bool isActive() const override;
  Domain::Position getAnchorPosition() const override;
  const std::vector<PreviewTileData> &getTiles() const override;
  PreviewBounds getBounds() const override;
  void updateCursorPosition(const Domain::Position &cursor) override;

  // Regeneration support for brush size changes
  void regenerate() override;

private:
  std::string creatureName_;
  const BrushSettingsService *brushSettings_ = nullptr;
  Domain::Position anchor_{0, 0, 0};
  mutable std::vector<PreviewTileData> tiles_;
  mutable PreviewBounds bounds_;
  mutable bool needsRegen_ = false;

  // Cached settings for change detection
  mutable std::vector<std::pair<int, int>> cachedOffsets_;

  void buildPreview() const;
  bool checkSettingsChanged() const;
};

} // namespace MapEditor::Services::Preview
