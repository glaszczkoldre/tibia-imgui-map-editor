#include "PalettesPanel.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "../../../Domain/Palette/Palette.h"
#include "../../../Services/AppSettings.h"
#include "UI/Ribbon/Utils/RibbonUtils.h"
#include "UI/Windows/PaletteWindowManager.h"
#include "ext/fontawesome6/IconsFontAwesome6.h"

namespace MapEditor::UI::Ribbon {

using namespace Domain::Palette;

PalettesPanel::PalettesPanel(PaletteWindowManager *windowManager,
                             Domain::Palette::PaletteRegistry &paletteRegistry,
                             Services::AppSettings *appSettings)
    : windowManager_(windowManager), paletteRegistry_(&paletteRegistry),
      appSettings_(appSettings) {}

void PalettesPanel::Render() {
  if (!windowManager_) {
    ImGui::TextDisabled("Window manager not available");
    return;
  }

  // Get all palette names from registry (using injected member)
  if (!paletteRegistry_) {
    ImGui::TextDisabled("Palette registry not available");
    return;
  }
  const auto &paletteNames = paletteRegistry_->getPaletteNames();

  if (paletteNames.empty()) {
    ImGui::TextDisabled("No palettes loaded");
    return;
  }

  // Render a button for each palette
  bool first = true;
  for (const auto &paletteName : paletteNames) {
    if (!first) {
      ImGui::SameLine();
    }
    first = false;
    renderPaletteButton(paletteName);
  }

  // Separator before icon size slider
  ImGui::SameLine();
  ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
  ImGui::SameLine();

  // Icon size slider
  if (appSettings_) {
    ImGui::SetNextItemWidth(100.0f);
    ImGui::SliderFloat("##PaletteIconSize", &appSettings_->paletteIconSize,
                       32.0f, 128.0f, "%.0f px");
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Palette icon size");
    }
  }
}

void PalettesPanel::renderPaletteButton(const std::string &paletteName) {
  bool isActive = windowManager_->isPaletteWindowVisible(paletteName);

  // Use generic palette icon - could be customized per palette if needed
  const char *icon = ICON_FA_PALETTE;

  // Truncate long names for button label
  std::string label = " " + paletteName;
  if (label.length() > 15) {
    label = " " + paletteName.substr(0, 12) + "...";
  }

  std::string tooltip = "Open " + paletteName;

  Utils::RenderToggleButton(
      icon, isActive, tooltip.c_str(),
      [this, &paletteName]() {
        windowManager_->togglePaletteWindow(paletteName);
      },
      label.c_str());
}

} // namespace MapEditor::UI::Ribbon
