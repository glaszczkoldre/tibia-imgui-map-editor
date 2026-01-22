#include "OpenSecDialog.h"
#include "Core/Config.h"
#include "Services/ClientVersionRegistry.h"
#include "Utils/FormatUtils.h"
#include <imgui.h>
#include <nfd.hpp>

namespace MapEditor {
namespace UI {

void OpenSecDialog::initialize(Services::ClientVersionRegistry* registry) {
  registry_ = registry;
}

void OpenSecDialog::show() {
  visible_ = true;
  sec_folder_.clear();
  sec_version_ = 772;
  folder_valid_ = false;
  
  // Pre-calculate SEC version list once when dialog opens
  sec_versions_.clear();
  if (registry_) {
    for (auto* v : registry_->getAllVersions()) {
      if (v && v->getVersion() < 800) {
        sec_versions_.push_back(v->getVersion());
      }
    }
  }
}

void OpenSecDialog::render() {
  if (!visible_) return;

  ImGui::OpenPopup("Open SEC Map##EditorModal");

  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(Config::UI::OPEN_SEC_DIALOG_W, Config::UI::OPEN_SEC_DIALOG_H), ImGuiCond_Appearing);

  if (ImGui::BeginPopupModal("Open SEC Map##EditorModal", nullptr,
                              ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextColored(ImVec4(0.7f, 0.8f, 0.9f, 1.0f),
                       "Select SEC map folder and client:");
    ImGui::Separator();
    ImGui::Spacing();

    // Folder selection
    ImGui::Text("SEC Map Folder:");
    ImGui::SameLine();
    std::string folder_str = sec_folder_.empty() ? "<none selected>" : sec_folder_.string();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    ImGui::TextWrapped("%s", folder_str.c_str());
    ImGui::PopStyleColor();
    ImGui::SameLine();
    if (ImGui::Button("Browse...##SecFolder")) {
      NFD::UniquePath outPath;
      if (NFD::PickFolder(outPath) == NFD_OKAY) {
        sec_folder_ = outPath.get();
        folder_valid_ = std::filesystem::exists(sec_folder_);
      }
    }

    ImGui::Spacing();

    // Client version selection (using cached list from show())
    ImGui::Text("Client Version:");
    if (!sec_versions_.empty()) {
      std::string preview = Utils::formatVersion(sec_version_);
      if (ImGui::BeginCombo("##SecVersion", preview.c_str())) {
        for (auto v : sec_versions_) {
          std::string label = Utils::formatVersion(v);
          bool selected = (v == sec_version_);
          if (ImGui::Selectable(label.c_str(), selected)) {
            sec_version_ = v;
          }
        }
        ImGui::EndCombo();
      }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Action buttons
    float button_width = Config::UI::MODAL_BUTTON_W;
    float total_width = button_width * 2 + 10.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - total_width) / 2.0f);

    if (ImGui::Button("Cancel##Sec", ImVec2(button_width, 0))) {
      visible_ = false;
      sec_folder_.clear();
      folder_valid_ = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine(0, 10.0f);

    bool can_open = folder_valid_ && sec_version_ > 0;
    if (!can_open) {
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
    }

    if (ImGui::Button("Open SEC Map", ImVec2(button_width, 0)) && can_open) {
      visible_ = false;
      if (on_confirm_) {
        on_confirm_(sec_folder_, sec_version_);
      }
      sec_folder_.clear();
      folder_valid_ = false;
      ImGui::CloseCurrentPopup();
    }

    if (!can_open) {
      ImGui::PopStyleVar();
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Select a valid SEC folder first");
      }
    }

    ImGui::EndPopup();
  }
}

} // namespace UI
} // namespace MapEditor
