#pragma once

#include "ext/fontawesome6/IconsFontAwesome6.h"
#include <format>
#include <imgui.h>
#include <string_view>


namespace MapEditor::UI::Utils {

/**
 * Helper to set a tooltip if the previous item is hovered.
 * Reduces code duplication for simple tooltips.
 */
inline void
SetTooltipOnHover(std::string_view text,
                  ImGuiHoveredFlags flags = ImGuiHoveredFlags_None) {
  if (ImGui::IsItemHovered(flags)) {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(text.data(), text.data() + text.size());
    ImGui::EndTooltip();
  }
}

/**
 * @brief Renders a paste button if the clipboard contains text.
 *
 * If clicked, it pastes the clipboard content into the provided buffer.
 * This is useful for search/filter input fields.
 *
 * @param buffer The destination buffer for the pasted text.
 * @param buffer_size The size of the destination buffer.
 * @param button_id A unique ID for the button (e.g., "##PasteSearch").
 * @param tooltip The tooltip text to show on hover.
 * @param button_size The size of the button.
 * @return True if the paste button was clicked, false otherwise.
 */
inline bool RenderPasteButton(char *buffer, size_t buffer_size,
                              const char *button_id, const char *tooltip,
                              const ImVec2 &button_size = ImVec2(0, 0)) {
  const char *clipboard = ImGui::GetClipboardText();
  if (clipboard && clipboard[0] != '\0') {
    ImGui::SameLine();
    // Use {}{} to concatenate icon and ID without extra space, allowing caller
    // to handle spacing if needed
    if (ImGui::Button(std::format("{}{}", ICON_FA_PASTE, button_id).c_str(),
                      button_size)) {
      // Safely copy clipboard content into the buffer
      if (buffer_size > 0) {
        *std::format_to_n(buffer, buffer_size - 1, "{}", clipboard).out = '\0';
      }
      return true;
    }
    SetTooltipOnHover(tooltip);
  }
  return false;
}

/**
 * @brief Renders a flat image grid item with selection/hover highlights.
 *
 * Standardized rendering for palette/tileset grid items. Uses:
 * - Gold border for selection (professional editor standard)
 * - Semi-transparent white overlay for hover
 *
 * @param textureId OpenGL texture ID (cast to void*)
 * @param size Size of the grid item in pixels
 * @param isSelected Whether this item is currently selected
 * @return True if the item was clicked, false otherwise
 */
inline bool RenderGridItem(void *textureId, float size, bool isSelected) {
  ImVec2 cursorPos = ImGui::GetCursorScreenPos();

  // Draw flat image (no button frame)
  ImGui::Image(textureId, ImVec2(size, size));

  // Track interaction manually
  bool hovered = ImGui::IsItemHovered();
  bool clicked = ImGui::IsItemClicked();

  ImDrawList *drawList = ImGui::GetWindowDrawList();
  ImVec2 rectMax = ImVec2(cursorPos.x + size, cursorPos.y + size);

  // Selection highlight: gold border (professional editor standard)
  if (isSelected) {
    ImU32 selectColor = IM_COL32(255, 200, 0, 255); // Gold
    drawList->AddRect(cursorPos, rectMax, selectColor, 0.0f, 0, 2.0f);
  }

  // Hover highlight: semi-transparent overlay
  if (hovered && !isSelected) {
    ImU32 hoverColor = IM_COL32(255, 255, 255, 60); // Subtle white
    drawList->AddRectFilled(cursorPos, rectMax, hoverColor);
  }

  return clicked;
}

} // namespace MapEditor::UI::Utils
