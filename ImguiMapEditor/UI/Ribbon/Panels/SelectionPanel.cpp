#include "SelectionPanel.h"
#include "Application/MapTabManager.h"
#include "Domain/SelectionMode.h"
#include "IconsFontAwesome6.h"
#include "UI/Ribbon/Utils/RibbonUtils.h"
#include <imgui.h>

namespace MapEditor {
namespace UI {
namespace Ribbon {

SelectionPanel::SelectionPanel(Domain::SelectionSettings &selection_settings,
                               AppLogic::MapTabManager *tab_manager)
    : selection_settings_(selection_settings), tab_manager_(tab_manager) {}

void SelectionPanel::Render() {
  bool has_session = tab_manager_ && tab_manager_->getActiveSession();
  bool has_selection =
      has_session && !tab_manager_->getActiveSession()->getSelectionService().isEmpty();

  // Selection Type: Smart vs Pixel Perfect (Radio behavior)
  bool is_smart = !selection_settings_.use_pixel_perfect;
  Utils::RenderRadioButton(
      ICON_FA_WAND_MAGIC_SPARKLES, is_smart,
      "Smart Selection\nContext-sensitive logical selection\nPriority: "
      "Creature > Top Item > Ground",
      [this]() { selection_settings_.use_pixel_perfect = false; },
      "##SmartSelection");

  ImGui::SameLine();

  bool is_pixel_perfect = selection_settings_.use_pixel_perfect;
  Utils::RenderRadioButton(
      ICON_FA_CROSSHAIRS, is_pixel_perfect,
      "Pixel Perfect Selection\nUse sprite hit testing to select\nthe exact "
      "item under cursor",
      [this]() { selection_settings_.use_pixel_perfect = true; },
      "##PixelPerfect");

  ImGui::SameLine();
  Utils::RenderSeparator();
  ImGui::SameLine();

  // Floor scope toggle (Radio behavior)
  bool current_floor = selection_settings_.floor_scope ==
                       Domain::SelectionFloorScope::CurrentFloor;
  Utils::RenderRadioButton(
      ICON_FA_LAYER_GROUP, current_floor,
      "Select Current Floor Only\nLimit selection to the active Z-level",
      [this]() {
        selection_settings_.floor_scope =
            Domain::SelectionFloorScope::CurrentFloor;
      },
      "##CurrentFloor");

  ImGui::SameLine();

  bool all_floors = selection_settings_.floor_scope ==
                    Domain::SelectionFloorScope::VisibleFloors;
  Utils::RenderRadioButton(
      ICON_FA_CUBES, all_floors,
      "Select All Visible Floors\nSelect items across all visible Z-levels",
      [this]() {
        selection_settings_.floor_scope =
            Domain::SelectionFloorScope::VisibleFloors;
      },
      "##AllFloors");

  ImGui::SameLine();
  Utils::RenderSeparator();
  ImGui::SameLine();

  // Note: Select All button removed - not needed per user feedback

  // Clear Selection with count
  std::string clear_label = " Clear";
  if (has_selection) {
    size_t count = tab_manager_->getActiveSession()->getSelectionService().size();
    clear_label = std::format(" Clear ({})", count);
  }

  Utils::RenderButton(ICON_FA_XMARK, clear_label.c_str(), has_selection,
                      "Clear Selection (Esc)", nullptr, [this]() {
                        if (tab_manager_)
                          tab_manager_->getActiveSession()->clearSelection();
                      });
}

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor
