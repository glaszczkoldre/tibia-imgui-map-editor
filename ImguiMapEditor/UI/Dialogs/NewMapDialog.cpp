#include "NewMapDialog.h"
#include "Core/Config.h"
#include "UI/Core/Theme.h"
#include "ext/fontawesome6/IconsFontAwesome6.h"
#include <imgui.h>

namespace MapEditor {
namespace UI {

namespace SC = SemanticColors;

void NewMapDialog::initialize(Services::ClientVersionRegistry *registry) {
  panel_.initialize(registry);
}

void NewMapDialog::show() {
  visible_ = true;
  state_ = {};
  state_.description = "Made with Tibia Imgui Map Editor!";
  panel_.reset();
}

void NewMapDialog::render() {
  if (!visible_)
    return;

  ImGui::OpenPopup("New Map##EditorModal");

  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(
      ImVec2(Config::UI::NEW_MAP_DIALOG_W, Config::UI::NEW_MAP_DIALOG_H),
      ImGuiCond_Appearing);
  ImGui::SetNextWindowSizeConstraints(
      ImVec2(Config::UI::NEW_MAP_DIALOG_W, Config::UI::NEW_MAP_DIALOG_H),
      ImVec2(FLT_MAX, FLT_MAX));

  ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 8.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

  if (ImGui::BeginPopupModal("New Map##EditorModal", nullptr,
                              ImGuiWindowFlags_None)) {

    // Content area: fill all space minus footer
    float footer_h = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y * 2;
    float content_h = ImGui::GetContentRegionAvail().y - footer_h;
    if (content_h < 100.0f) content_h = 100.0f;

    // === TABS + CONTENT ===
    ImGui::BeginChild("##content", ImVec2(0, content_h), ImGuiChildFlags_None);

    if (ImGui::BeginTabBar("##NewMapTabs")) {
      if (ImGui::BeginTabItem("OTBM")) {
        ImGui::Spacing();
        panel_.render(state_);
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("SEC")) {
        ImGui::Spacing();
        ImGui::TextColored(SC::TextDim(),
                           ICON_FA_CLOCK " Coming soon");
        ImGui::Spacing();
        ImGui::TextColored(SC::TextDim(),
                           "SEC format support is not yet implemented.");
        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }

    ImGui::EndChild();

    // === FOOTER: always visible at the bottom ===
    ImGui::Separator();

    float button_width = Config::UI::MODAL_BUTTON_W;
    float total_width = button_width * 2 + 10.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - total_width) / 2.0f);

    if (ImGui::Button("Cancel", ImVec2(button_width, 0))) {
      visible_ = false;
      state_ = {};
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine(0, 10.0f);

    bool can_create =
        !state_.map_name.empty() && state_.selected_template_index >= 0;

    if (!can_create) {
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, SC::DISABLED_ALPHA);
    }

    ImGui::PushStyleColor(ImGuiCol_Button, SC::INFO);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, SC::Lighten(SC::INFO));

    if (ImGui::Button(ICON_FA_CHECK " Create Map", ImVec2(button_width, 0)) &&
        can_create) {
      visible_ = false;
      if (on_confirm_) {
        on_confirm_(state_);
      }
      state_ = {};
      ImGui::CloseCurrentPopup();
    }

    ImGui::PopStyleColor(2);

    if (!can_create) {
      ImGui::PopStyleVar();
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (state_.map_name.empty()) {
          ImGui::SetTooltip("Enter a map name");
        } else {
          ImGui::SetTooltip("Select a client version first");
        }
      }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      visible_ = false;
      state_ = {};
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }

  ImGui::PopStyleVar(3);
}

} // namespace UI
} // namespace MapEditor
