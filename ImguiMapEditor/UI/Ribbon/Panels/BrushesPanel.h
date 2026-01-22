#pragma once
#include "../Interfaces/IRibbonPanel.h"

namespace MapEditor {

namespace Brushes {
class BrushController;
}

namespace Services {
class BrushSettingsService;
}

namespace UI {
namespace Ribbon {

/**
 * Brushes panel for the ribbon.
 * Controls active brush and brush settings (type, size, shape).
 */
class BrushesPanel : public IRibbonPanel {
public:
  explicit BrushesPanel(
      Brushes::BrushController *controller,
      Services::BrushSettingsService *settingsService = nullptr);
  ~BrushesPanel() override = default;

  // IRibbonPanel interface
  const char *GetPanelName() const override { return "Brushes"; }
  const char *GetPanelID() const override { return "Brushes###RibbonBrushes"; }
  void Render() override;

private:
  Brushes::BrushController *controller_;
  Services::BrushSettingsService *settingsService_;
  int selected_brush_ = 0;

  void renderShapeControls();
  void renderSizeControls();
};

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor
