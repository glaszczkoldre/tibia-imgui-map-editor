#include "FilePanel.h"
#include "Core/Config.h"
#include "IconsFontAwesome6.h"
#include "UI/Ribbon/Utils/RibbonUtils.h"
#include <imgui.h>


namespace MapEditor {
namespace UI {
namespace Ribbon {

void FilePanel::Render() {
  // If loading, show spinner and disable interactions
  bool is_loading = on_check_loading_ && on_check_loading_();

  if (is_loading) {
      ImGui::BeginDisabled();
  }

  // Horizontal layout for buttons
  Utils::RenderButton(ICON_FA_FILE, " New", !is_loading, "Create a new map (Ctrl+N)",
                      nullptr, [this]() {
                        if (on_new_map_)
                          on_new_map_();
                      });

  ImGui::SameLine();

  Utils::RenderButton(ICON_FA_FOLDER_OPEN, " Open", !is_loading,
                      "Open an existing map (Ctrl+O)", nullptr, [this]() {
                        if (on_open_map_)
                          on_open_map_();
                      });

  ImGui::SameLine();

  // Determine if we have unsaved changes
  bool is_modified = has_active_session_ && on_check_modified_ && on_check_modified_();

  // Use generic warning icon since specific floppy one might not be in this font version
  const char* save_icon = is_modified ? ICON_FA_TRIANGLE_EXCLAMATION : ICON_FA_FLOPPY_DISK;
  const char* save_tooltip = is_modified ? "Save changes (Ctrl+S)\n" ICON_FA_TRIANGLE_EXCLAMATION " You have unsaved changes" : "Save the current map (Ctrl+S)";

  // Highlight save button if modified
  if (is_modified) {
      const auto& c = Config::UI::MODIFIED_INDICATOR_COLOR;
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(c.r, c.g, c.b, c.a));
  }

  Utils::RenderButton(save_icon, " Save", has_active_session_ && !is_loading,
                      save_tooltip, "No active map to save",
                      [this]() {
                        if (on_save_map_)
                          on_save_map_();
                      });

  if (is_modified) {
      ImGui::PopStyleColor();
  }

  ImGui::SameLine();

  // Save As Button
  Utils::RenderButton(ICON_FA_FILE_EXPORT, " Save As", has_active_session_ && !is_loading,
                      "Save the current map with a new name (Ctrl+Shift+S)", "No active map to save",
                      [this]() {
                        if (on_save_as_map_)
                          on_save_as_map_();
                      });

  ImGui::SameLine();
  Utils::RenderSeparator();
  ImGui::SameLine();

  // Close Button
  Utils::RenderButton(ICON_FA_XMARK, " Close", has_active_session_ && !is_loading,
                      "Close current map (Ctrl+W)", "No active map",
                      [this]() {
                        if (on_close_map_)
                          on_close_map_();
                      });

  if (is_loading) {
      ImGui::EndDisabled();
      ImGui::SameLine();
      ImGui::Text(ICON_FA_SPINNER " Loading map... (please wait)");
      // Simple rotation effect
      // Note: FontAwesome spinners don't auto-rotate in ImGui, usually requires rotated rendering
      // For now, static text is better than nothing.
  }
}

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor
