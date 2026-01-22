#include "ClientDetailsCard.h"
#include <IconsFontAwesome6.h>
#include <imgui.h>

namespace MapEditor {
namespace UI {

void ClientDetailsCard::render() {
  // Card container with darker background
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.14f, 0.17f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
  ImGui::BeginChild("DetailsCard", ImVec2(0, 120), true);

  // Get version from registry
  const Domain::ClientVersion *version = nullptr;
  if (registry_ && selected_version_ != 0) {
    version = registry_->getVersion(selected_version_);
  }

  if (!version) {
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 40);
    float text_width =
        ImGui::CalcTextSize("Select a client from the list above").x;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - text_width) * 0.5f);
    ImGui::TextDisabled("Select a client from the list above");
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    return;
  }

  // Card header with icon
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
  ImGui::Text(ICON_FA_CIRCLE_INFO);
  ImGui::PopStyleColor();
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0.9f, 0.92f, 0.95f, 1.0f), "Client %s (version %u)",
                     version->getName().c_str(), version->getVersion());

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Two-column layout for details
  ImGui::Columns(2, "details_cols", false);

  ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.7f, 1.0f), "Description:");
  ImGui::SameLine();
  ImGui::Text("%s", version->getDescription().empty()
                        ? "-"
                        : version->getDescription().c_str());

  ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.7f, 1.0f), "OTB ID:");
  ImGui::SameLine();
  ImGui::Text("%u", version->getOtbVersion());

  ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.7f, 1.0f), "DAT:");
  ImGui::SameLine();
  ImGui::Text("%08X", version->getDatSignature());

  ImGui::NextColumn();

  auto path = version->getClientPath();
  ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.7f, 1.0f), "Path:");
  ImGui::SameLine();
  if (path.empty()) {
    ImGui::TextDisabled("(not configured)");
  } else {
    ImGui::Text("%s", path.string().c_str());
  }

  ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.7f, 1.0f), "OTB Major:");
  ImGui::SameLine();
  ImGui::Text("%u", version->getOtbMajor());

  ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.7f, 1.0f), "SPR:");
  ImGui::SameLine();
  ImGui::Text("%08X", version->getSprSignature());

  ImGui::Columns(1);

  ImGui::EndChild();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor();
}

} // namespace UI
} // namespace MapEditor
