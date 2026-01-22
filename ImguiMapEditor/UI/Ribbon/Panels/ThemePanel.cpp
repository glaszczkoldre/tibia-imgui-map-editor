#include "ThemePanel.h"
#include "UI/Core/Theme.h"
#include <imgui.h>
#include "IconsFontAwesome6.h"
namespace MapEditor {
namespace UI {
namespace Ribbon {

void ThemePanel::Render() {
    ImGui::Text(ICON_FA_PALETTE);
    ImGui::SameLine();
    
    // Determine current theme name
    const char* current_name = "Select Theme";
    if (current_theme_) {
        current_name = GetThemeName(*current_theme_);
    }
    
    // Theme dropdown
    ImGui::SetNextItemWidth(130.0f);
    if (ImGui::BeginCombo("##ThemeCombo", current_name)) {
        for (const auto& theme : AVAILABLE_THEMES) {
            bool is_selected = current_theme_ && (*current_theme_ == theme.type);

            // Add icon to theme names in dropdown
            const char* theme_icon = ICON_FA_CIRCLE;
            ImVec4 color_preview = theme.preview_color;

            switch (theme.type) {
            case ThemeType::ModernDark:
            case ThemeType::MidnightPurple:
            case ThemeType::ForestGreen:
                theme_icon = ICON_FA_MOON;
                break;
            case ThemeType::ClassicLight:
            case ThemeType::SunsetOrange:
            case ThemeType::TibiaRPG:
                theme_icon = ICON_FA_SUN;
                break;
            case ThemeType::OtclientTheme:
                theme_icon = ICON_FA_GAMEPAD;
                break;
            }

            // Draw colored rectangle for preview
            ImGui::PushStyleColor(ImGuiCol_Text, color_preview);
            ImGui::Text("%s", ICON_FA_SQUARE);
            ImGui::PopStyleColor();
            ImGui::SameLine();

            std::string label = std::string(theme_icon) + " " + theme.name;

            if (ImGui::Selectable(label.c_str(), is_selected)) {
                ApplyTheme(theme.type);
                if (current_theme_) {
                    *current_theme_ = theme.type;
                }
            }
            if (ImGui::IsItemHovered() && theme.description) {
                ImGui::SetTooltip("%s", theme.description);
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Choose the editor visual theme");
    }
}

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor
