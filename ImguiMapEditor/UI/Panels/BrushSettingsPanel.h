#pragma once

#include <string>

namespace MapEditor {
namespace Brushes {
class BrushController;
class BrushRegistry;
class IBrush;
}
namespace Services {
class BrushSettingsService;
}
namespace UI {
class IconTextureCache;
}
} // namespace MapEditor

namespace MapEditor::UI::Panels {

/**
 * Dockable RME-style Brush Settings / Tool Options panel.
 * Replaces the old BrushSizePanel with a final 1:1 RME layout:
 * - Main tools (icon grids)
 * - Size (X/Y exact and aspect ratio locked sliders)
 * - Other (family-specific checkboxes and sliders)
 */
class BrushSettingsPanel {
public:
  explicit BrushSettingsPanel(Services::BrushSettingsService *brushService,
                              Brushes::BrushController *brushController,
                              Brushes::BrushRegistry *brushRegistry,
                              UI::IconTextureCache *iconCache);
  ~BrushSettingsPanel() = default;

  // Non-copyable
  BrushSettingsPanel(const BrushSettingsPanel &) = delete;
  BrushSettingsPanel &operator=(const BrushSettingsPanel &) = delete;

  // Render the panel interface
  void render(bool *p_visible = nullptr);

private:
  Services::BrushSettingsService *service_;
  Brushes::BrushController *controller_;
  Brushes::BrushRegistry *registry_;
  UI::IconTextureCache *iconCache_;

  const Brushes::IBrush *lastCreatureBrush_ = nullptr;

  void renderMainTools();
  void renderSize();
  void renderOther();

  void selectCreatureTool();
};

} // namespace MapEditor::UI::Panels
