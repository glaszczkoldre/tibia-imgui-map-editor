#pragma once
#include "../Interfaces/IRibbonPanel.h"

namespace MapEditor {

namespace Brushes {
class BrushController;
}

namespace UI {
namespace Ribbon {

/**
 * Brushes panel for the ribbon.
 * Controls active brush selection.
 */
class BrushesPanel : public IRibbonPanel {
public:
  explicit BrushesPanel(Brushes::BrushController *controller);
  ~BrushesPanel() override = default;

  // IRibbonPanel interface
  const char *GetPanelName() const override { return "Brushes"; }
  const char *GetPanelID() const override { return "Brushes###RibbonBrushes"; }
  void Render() override;

private:
  Brushes::BrushController *controller_;
  int selected_brush_ = 0;
};

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor
