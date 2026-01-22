#pragma once

#include "../../../Domain/Palette/Palette.h"
#include "../Interfaces/IRibbonPanel.h"
#include <string>
#include <vector>


namespace MapEditor {

namespace UI {
class PaletteWindowManager;
} // namespace UI

namespace Services {
struct AppSettings;
} // namespace Services

namespace UI {
namespace Ribbon {

/**
 * Ribbon panel with buttons for each palette.
 *
 * Palette buttons are generated dynamically from PaletteRegistry.
 * Clicking a button opens/toggles the corresponding PaletteWindow.
 */
class PalettesPanel : public IRibbonPanel {
public:
  explicit PalettesPanel(PaletteWindowManager *windowManager,
                         Domain::Palette::PaletteRegistry &paletteRegistry,
                         Services::AppSettings *appSettings = nullptr);
  ~PalettesPanel() override = default;

  const char *GetPanelName() const override { return "Palettes"; }
  const char *GetPanelID() const override { return "Palettes"; }
  void Render() override;

private:
  void renderPaletteButton(const std::string &paletteName);

  PaletteWindowManager *windowManager_ = nullptr;
  Domain::Palette::PaletteRegistry *paletteRegistry_ = nullptr;
  Services::AppSettings *appSettings_ = nullptr;
};

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor
