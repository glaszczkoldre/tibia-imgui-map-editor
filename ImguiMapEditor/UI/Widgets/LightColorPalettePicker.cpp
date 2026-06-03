#include "LightColorPalettePicker.h"

#include <cstdio>

#include <imgui.h>

#include "Core/Config.h"
#include "Rendering/Light/LightColorPalette.h"

namespace MapEditor::UI {
namespace {

constexpr int kPaletteColumns = 18;
constexpr float kCellSize = 14.0f;
constexpr float kCellPadding = 2.0f;
constexpr float kButtonSwatchWidth = 24.0f;
constexpr float kButtonSwatchHeight = 16.0f;

ImU32 paletteColor(uint8_t color_index) {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  Rendering::LightColorPalette::from8bit(color_index, r, g, b);
  return IM_COL32(r, g, b, 255);
}

ImU32 borderColorFor(ImU32 color) {
  const int r = static_cast<int>(color & 0xFF);
  const int g = static_cast<int>((color >> 8) & 0xFF);
  const int b = static_cast<int>((color >> 16) & 0xFF);
  const int luma = (r * 299 + g * 587 + b * 114) / 1000;
  return luma < 96 ? IM_COL32(200, 200, 200, 180) : IM_COL32(48, 48, 48, 180);
}

void drawSwatch(ImDrawList& draw_list,
                ImVec2 min,
                ImVec2 max,
                uint8_t color_index,
                bool selected,
                bool hovered) {
  const ImU32 color = paletteColor(color_index);
  draw_list.AddRectFilled(min, max, color);
  draw_list.AddRect(min, max, borderColorFor(color));

  if (selected) {
    draw_list.AddRect(ImVec2(min.x - 2.0f, min.y - 2.0f),
                      ImVec2(max.x + 2.0f, max.y + 2.0f),
                      ImGui::GetColorU32(ImGuiCol_NavHighlight), 0.0f, 0, 2.0f);
  } else if (hovered) {
    draw_list.AddRect(ImVec2(min.x - 1.0f, min.y - 1.0f),
                      ImVec2(max.x + 1.0f, max.y + 1.0f),
                      IM_COL32(255, 255, 255, 128));
  }
}

void drawCurrentColorButton(const char* id, uint8_t color, bool show_index) {
  const float height = ImGui::GetFrameHeight();
  const float width = show_index ? 64.0f : 40.0f;
  const ImVec2 start = ImGui::GetCursorScreenPos();

  ImGui::InvisibleButton(id, ImVec2(width, height));

  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  const ImVec2 min = ImGui::GetItemRectMin();
  const ImVec2 max = ImGui::GetItemRectMax();
  const bool active = ImGui::IsItemActive();
  const bool hovered = ImGui::IsItemHovered();
  const ImU32 button_color = ImGui::GetColorU32(
      active ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);

  draw_list->AddRectFilled(min, max, button_color, ImGui::GetStyle().FrameRounding);
  draw_list->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_Border), ImGui::GetStyle().FrameRounding);

  const ImVec2 swatch_min(start.x + 6.0f, start.y + (height - kButtonSwatchHeight) * 0.5f);
  const ImVec2 swatch_max(swatch_min.x + kButtonSwatchWidth, swatch_min.y + kButtonSwatchHeight);
  drawSwatch(*draw_list, swatch_min, swatch_max, color, false, false);

  if (show_index) {
    char label[8] = {};
    std::snprintf(label, sizeof(label), "%u", static_cast<unsigned>(color));
    const ImVec2 text_size = ImGui::CalcTextSize(label);
    const ImVec2 text_pos(swatch_max.x + 6.0f, start.y + (height - text_size.y) * 0.5f);
    draw_list->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), label);
  }
}

} // namespace

bool LightColorPalettePicker(const char* id, uint8_t& color, bool show_index, const char* tooltip) {
  bool changed = false;
  if (color > Config::Lighting::MAX_SERVER_LIGHT_COLOR) {
    color = Config::Lighting::MAX_SERVER_LIGHT_COLOR;
    changed = true;
  }

  ImGui::PushID(id);
  drawCurrentColorButton("button", color, show_index);
  if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
    ImGui::OpenPopup("light_color_palette");
  }
  if (ImGui::IsItemHovered() && tooltip) {
    ImGui::SetTooltip("%s: %u", tooltip, static_cast<unsigned>(color));
  }

  if (ImGui::BeginPopup("light_color_palette")) {
    if (tooltip) {
      ImGui::TextUnformatted(tooltip);
      ImGui::Separator();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(kCellPadding, kCellPadding));
    for (int index = 0; index < Config::Lighting::LIGHT_COLOR_COUNT; ++index) {
      ImGui::PushID(index);
      const ImVec2 swatch_min = ImGui::GetCursorScreenPos();
      ImGui::InvisibleButton("swatch", ImVec2(kCellSize, kCellSize));
      const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
      const bool hovered = ImGui::IsItemHovered();
      const auto palette_index = static_cast<uint8_t>(index);
      drawSwatch(*ImGui::GetWindowDrawList(),
                 swatch_min,
                 ImVec2(swatch_min.x + kCellSize, swatch_min.y + kCellSize),
                 palette_index,
                 color == palette_index,
                 hovered);

      if (hovered) {
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        Rendering::LightColorPalette::from8bit(palette_index, r, g, b);
        ImGui::SetTooltip("Index %d | RGB(%u, %u, %u)",
                          index,
                          static_cast<unsigned>(r),
                          static_cast<unsigned>(g),
                          static_cast<unsigned>(b));
      }

      if (clicked) {
        color = palette_index;
        changed = true;
        ImGui::CloseCurrentPopup();
      }

      ImGui::PopID();
      if ((index + 1) % kPaletteColumns != 0) {
        ImGui::SameLine();
      }
    }
    ImGui::PopStyleVar();

    ImGui::EndPopup();
  }

  ImGui::PopID();
  return changed;
}

} // namespace MapEditor::UI
