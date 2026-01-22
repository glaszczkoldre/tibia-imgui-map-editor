#include "UI/Dialogs/ClientConfiguration/ClientTableWidget.h"
#include <imgui.h>

namespace MapEditor {
namespace UI {

void ClientTableWidget::render(float height) {
  ImGui::BeginChild("VersionTable", ImVec2(0, height), true,
                    ImGuiWindowFlags_HorizontalScrollbar);

  if (ImGui::BeginTable(
          "Versions", 11,
          ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
              ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
              ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable)) {

    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 30);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 80);
    ImGui::TableSetupColumn("Ver", ImGuiTableColumnFlags_WidthFixed, 50);
    ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthFixed,
                            120);
    ImGui::TableSetupColumn("OTB ID", ImGuiTableColumnFlags_WidthFixed, 50);
    ImGui::TableSetupColumn("Major", ImGuiTableColumnFlags_WidthFixed, 45);
    ImGui::TableSetupColumn("OTBM", ImGuiTableColumnFlags_WidthFixed, 45);
    ImGui::TableSetupColumn("DAT Sig", ImGuiTableColumnFlags_WidthFixed, 75);
    ImGui::TableSetupColumn("SPR Sig", ImGuiTableColumnFlags_WidthFixed, 75);
    ImGui::TableSetupColumn("Data Dir", ImGuiTableColumnFlags_WidthFixed, 60);
    ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableHeadersRow();

    if (registry_) {
      auto versions = registry_->getAllVersions();

      for (const auto *version : versions) {
        // Apply filter
        if (!filter_.empty()) {
          std::string name = version->getName();
          std::string ver_str = std::to_string(version->getVersion());
          std::string desc = version->getDescription();
          if (name.find(filter_) == std::string::npos &&
              ver_str.find(filter_) == std::string::npos &&
              desc.find(filter_) == std::string::npos) {
            continue;
          }
        }

        uint32_t ver_num = version->getVersion();
        ImGui::TableNextRow();
        ImGui::PushID(static_cast<int>(ver_num));

        // Default checkbox
        ImGui::TableNextColumn();
        bool is_default = version->isDefault();
        if (ImGui::Checkbox("##default", &is_default)) {
          if (is_default && on_default_changed_) {
            on_default_changed_(ver_num);
          }
        }

        // Name - clickable to select
        ImGui::TableNextColumn();
        bool is_selected = (selected_version_ == ver_num);
        if (ImGui::Selectable(version->getName().c_str(), is_selected,
                              ImGuiSelectableFlags_SpanAllColumns)) {
          selected_version_ = ver_num;
          if (on_selection_) {
            on_selection_(ver_num);
          }
        }

        // Version number
        ImGui::TableNextColumn();
        ImGui::Text("%u", ver_num);

        // Description
        ImGui::TableNextColumn();
        const auto &desc = version->getDescription();
        if (desc.empty()) {
          ImGui::TextDisabled("-");
        } else {
          ImGui::TextUnformatted(desc.c_str());
          if (ImGui::IsItemHovered() && desc.length() > 15) {
            ImGui::SetTooltip("%s", desc.c_str());
          }
        }

        // OTB ID
        ImGui::TableNextColumn();
        ImGui::Text("%u", version->getOtbVersion());

        // OTB Major
        ImGui::TableNextColumn();
        ImGui::Text("%u", version->getOtbMajor());

        // OTBM Version
        ImGui::TableNextColumn();
        ImGui::Text("%u", version->getOtbmVersion());

        // DAT signature
        ImGui::TableNextColumn();
        ImGui::Text("%08X", version->getDatSignature());

        // SPR signature
        ImGui::TableNextColumn();
        ImGui::Text("%08X", version->getSprSignature());

        // Data Directory
        ImGui::TableNextColumn();
        const auto &data_dir = version->getDataDirectory();
        if (data_dir.empty()) {
          ImGui::TextDisabled("-");
        } else {
          ImGui::TextUnformatted(data_dir.c_str());
        }

        // Client Path
        ImGui::TableNextColumn();
        auto path = version->getClientPath();
        if (path.empty()) {
          ImGui::TextDisabled("Not set");
        } else {
          ImGui::TextUnformatted(path.filename().string().c_str());
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", path.string().c_str());
          }
        }

        ImGui::PopID();
      }
    }

    ImGui::EndTable();
  }

  ImGui::EndChild();
}

} // namespace UI
} // namespace MapEditor
