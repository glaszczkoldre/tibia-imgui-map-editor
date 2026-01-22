#include "AvailableClientsPanel.h"
#include <IconsFontAwesome6.h>
#include <algorithm>
#include <imgui.h>

namespace MapEditor {
namespace UI {

void AvailableClientsPanel::render() {
  // Panel header
  ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.92f, 1.0f), "Available Clients");
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::BeginChild("##ClientsList", ImVec2(0, 0), false);

  int total_count = 0;
  if (registry_) {
    auto all_versions = registry_->getAllVersions();

    // Sort: configured clients first (by version ascending),
    // then unconfigured clients (by version ascending)
    std::sort(
        all_versions.begin(), all_versions.end(),
        [](const Domain::ClientVersion *a, const Domain::ClientVersion *b) {
          bool a_has_path = !a->getClientPath().empty();
          bool b_has_path = !b->getClientPath().empty();

          // Configured clients come first
          if (a_has_path != b_has_path) {
            return a_has_path > b_has_path;
          }
          // Within same group, sort by version ascending (lowest first)
          return a->getVersion() < b->getVersion();
        });

    for (const auto *client : all_versions) {
      if (!client)
        continue;

      total_count++;
      uint32_t version = client->getVersion();
      bool is_selected = (selected_version_ == version);
      bool has_path = !client->getClientPath().empty();

      // Color scheme based on path availability
      const ImVec4 configured_color(0.3f, 0.85f, 0.5f, 1.0f);    // Green
      const ImVec4 not_configured_color(0.9f, 0.4f, 0.4f, 1.0f); // Red

      const float item_height = 48.0f;

      ImGui::PushID(static_cast<int>(version));

      // Style for selected/hover
      if (is_selected) {
        ImGui::PushStyleColor(ImGuiCol_Header,
                              ImVec4(0.25f, 0.45f, 0.70f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                              ImVec4(0.30f, 0.50f, 0.75f, 1.0f));
      } else {
        ImGui::PushStyleColor(ImGuiCol_Header,
                              ImVec4(0.18f, 0.20f, 0.24f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                              ImVec4(0.22f, 0.25f, 0.30f, 0.8f));
      }

      ImVec2 item_size = ImVec2(ImGui::GetContentRegionAvail().x, item_height);

      if (ImGui::Selectable("##ClientEntry", is_selected, 0, item_size)) {
        selected_version_ = version;
        if (on_selection_) {
          on_selection_(version);
        }
      }

      // Draw content over the selectable
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() - item_height);
      ImGui::Indent(8.0f);

      // Computer icon - colored based on path status
      ImGui::BeginGroup();
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12.0f);
      ImGui::PushStyleColor(ImGuiCol_Text,
                            has_path ? configured_color : not_configured_color);
      ImGui::Text(ICON_FA_COMPUTER);
      ImGui::PopStyleColor();
      ImGui::EndGroup();

      ImGui::SameLine();

      // Client name and description
      ImGui::BeginGroup();
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
      // Name colored based on path status
      ImGui::TextColored(has_path ? configured_color : not_configured_color,
                         "Tibia Client %s", client->getName().c_str());
      const auto &desc = client->getDescription();
      if (has_path) {
        ImGui::TextColored(ImVec4(0.55f, 0.58f, 0.62f, 1.0f), "%s",
                           desc.empty() ? "-" : desc.c_str());
      } else {
        ImGui::TextColored(ImVec4(0.6f, 0.35f, 0.35f, 1.0f),
                           "(Not configured)");
      }
      ImGui::EndGroup();

      ImGui::Unindent(8.0f);
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + item_height - 44);

      ImGui::PopStyleColor(2);
      ImGui::PopID();
      ImGui::Spacing();
    }
  }

  if (total_count == 0) {
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.5f, 0.52f, 0.55f, 1.0f),
                       "No clients in database.");
    ImGui::TextColored(ImVec4(0.4f, 0.42f, 0.45f, 1.0f),
                       "Use 'Client Config' to add clients.");
  }

  ImGui::EndChild();
}

} // namespace UI
} // namespace MapEditor
