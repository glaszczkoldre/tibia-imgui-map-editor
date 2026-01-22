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
      BrushSettingsService *brushSettings = nullptr);

  /**
   * Set brush settings service for size/shape.
   * Called if settings weren't available at construction.
   */
  void setBrushSettingsService(BrushSettingsService *service) {
    brushSettings_ = service;
    needsRegen_ = true;
  }

  // IPreviewProvider interface
  bool isActive() const override;
  Domain::Position getAnchorPosition() const override;
  const std::vector<PreviewTileData> &getTiles() const override;
  PreviewBounds getBounds() const override;
  void updateCursorPosition(const Domain::Position &cursor) override;

  // Regeneration support for brush size changes
  bool needsRegeneration() const override { return needsRegen_; }
  void regenerate() override;

  /**
   * Get the creature name being previewed.
   */
  const std::string &getCreatureName() const { return creatureName_; }

  /**
   * Mark preview as needing regeneration.
   * Called by BrushSettingsService when settings change.
   */
  void markNeedsRegeneration() { needsRegen_ = true; }

private:
  std::string creatureName_;
  BrushSettingsService *brushSettings_ = nullptr;
  Domain::Position anchor_{0, 0, 0};
  std::vector<PreviewTileData> tiles_;
  PreviewBounds bounds_;
  mutable bool needsRegen_ = false;

  // Cached settings for change detection
  mutable std::vector<std::pair<int, int>> cachedOffsets_;

  void buildPreview();
  bool checkSettingsChanged() const;
};

} // namespace Preview
} // namespace Services
} // namespace MapEditor
