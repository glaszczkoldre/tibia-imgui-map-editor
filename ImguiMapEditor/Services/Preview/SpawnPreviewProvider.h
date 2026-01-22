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
 * Preview provider for Spawn brush placement.
 *
 * Generates preview tiles showing spawn radius circle:
 * - Uses BrushSettingsService to get spawn radius
 * - Generates tiles in a circular pattern around cursor
 * - Each tile in radius is marked with has_spawn = true
 */
class SpawnPreviewProvider : public IPreviewProvider {
public:
  /**
   * Create spawn brush preview.
   * @param brushSettings Brush settings service for spawn radius
   */
  explicit SpawnPreviewProvider(BrushSettingsService *brushSettings = nullptr);

  /**
   * Set brush settings service.
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
  PreviewStyle getStyle() const override { return PreviewStyle::Outline; }

  // Regeneration support
  bool needsRegeneration() const override { return needsRegen_; }
  void regenerate() override;

  void markNeedsRegeneration() { needsRegen_ = true; }

private:
  BrushSettingsService *brushSettings_ = nullptr;
  Domain::Position anchor_{0, 0, 0};
  std::vector<PreviewTileData> tiles_;
  PreviewBounds bounds_;
  mutable bool needsRegen_ = true;

  // Cached radius for change detection
  mutable int cachedRadius_ = 3;

  void buildSquarePreview();
};

} // namespace Preview
} // namespace Services
} // namespace MapEditor
