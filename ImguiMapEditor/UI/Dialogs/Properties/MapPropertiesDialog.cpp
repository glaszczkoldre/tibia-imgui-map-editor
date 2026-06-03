#include "MapPropertiesDialog.h"
#include "Core/Config.h"
#include "Presentation/NotificationHelper.h"
#include "UI/Core/Theme.h"
#include "ext/fontawesome6/IconsFontAwesome6.h"
#include <filesystem>
#include <imgui.h>

namespace MapEditor {
namespace UI {

namespace SC = SemanticColors;

void MapPropertiesDialog::initialize(Services::ClientVersionRegistry *registry) {
  panel_.initialize(registry);
}

void MapPropertiesDialog::show(Domain::ChunkedMap *map) {
  if (!map)
    return;

  map_ = map;
  state_ = panel_.loadFromMap(map);
  should_open_ = true;
}

MapPropertiesDialog::Result MapPropertiesDialog::render() {
  Result result = Result::None;

  if (should_open_) {
    ImGui::OpenPopup("Map Properties###MapPropertiesDialog");
    should_open_ = false;
    is_open_ = true;
  }

  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  constexpr float PROPERTIES_DIALOG_H = Config::UI::NEW_MAP_DIALOG_H + 50.0f;
  ImGui::SetNextWindowSize(ImVec2(Config::UI::NEW_MAP_DIALOG_W, PROPERTIES_DIALOG_H), ImGuiCond_Appearing);
  ImGui::SetNextWindowSizeConstraints(ImVec2(Config::UI::NEW_MAP_DIALOG_W, PROPERTIES_DIALOG_H),
                                      ImVec2(FLT_MAX, FLT_MAX));

  ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 8.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

  if (ImGui::BeginPopupModal("Map Properties###MapPropertiesDialog", nullptr,
                              ImGuiWindowFlags_None)) {

    bool is_otbm = map_ &&
                   std::filesystem::path(map_->getFilename()).extension() == ".otbm";

    if (!is_otbm) {
      ImGui::Spacing();
      ImGui::TextColored(SC::TextDim(),
                         ICON_FA_TRIANGLE_EXCLAMATION
                         " Map properties are not supported for SEC maps.");
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      float button_width = Config::UI::MODAL_BUTTON_W;
      ImGui::SetCursorPosX((ImGui::GetWindowWidth() - button_width) / 2.0f);
      if (ImGui::Button("Close", ImVec2(button_width, 0))) {
        result = Result::Cancelled;
        ImGui::CloseCurrentPopup();
        is_open_ = false;
      }
    } else {
      // Content area: fill all space minus footer
      float footer_h = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y * 2;
      float content_h = ImGui::GetContentRegionAvail().y - footer_h;
      if (content_h < 100.0f) content_h = 100.0f;

      ImGui::BeginChild("##content", ImVec2(0, content_h), ImGuiChildFlags_None);
      panel_.render(state_);
      ImGui::EndChild();

      // Footer: always visible at the bottom
      ImGui::Separator();
      float button_width = Config::UI::MODAL_BUTTON_W;
      float total_width = button_width * 2 + 10.0f;
      ImGui::SetCursorPosX((ImGui::GetWindowWidth() - total_width) / 2.0f);

      if (ImGui::Button("Cancel", ImVec2(button_width, 0))) {
        result = Result::Cancelled;
        ImGui::CloseCurrentPopup();
        is_open_ = false;
      }

      ImGui::SameLine(0, 10.0f);

      ImGui::PushStyleColor(ImGuiCol_Button, SC::INFO);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, SC::Lighten(SC::INFO));

      if (ImGui::Button(ICON_FA_CHECK " OK", ImVec2(button_width, 0))) {
        applyToMap();
        result = Result::Applied;
        Presentation::showSuccess("Map properties updated!");
        ImGui::CloseCurrentPopup();
        is_open_ = false;
      }

      ImGui::PopStyleColor(2);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      result = Result::Cancelled;
      ImGui::CloseCurrentPopup();
      is_open_ = false;
    }

    ImGui::EndPopup();
  } else if (is_open_) {
    is_open_ = false;
    result = Result::Cancelled;
  }

  ImGui::PopStyleVar(3);

  return result;
}

void MapPropertiesDialog::applyToMap() {
  if (!map_)
    return;

  map_->setDescription(state_.description);
  map_->setHouseFile(state_.house_file);
  map_->setSpawnFile(state_.spawn_file);
}

} // namespace UI
} // namespace MapEditor
