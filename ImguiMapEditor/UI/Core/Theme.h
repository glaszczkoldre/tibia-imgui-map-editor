#pragma once

#include <algorithm>
#include <ranges>
#include <imgui.h>
#include <string>
#include <iterator>
#include <functional>

// ============================================================================
// Semantic Colors — universal across all themes
// ============================================================================
namespace SemanticColors {
    // State indicators
    inline constexpr ImVec4 MODIFIED     { 1.00f, 0.80f, 0.00f, 1.00f };
    inline constexpr ImVec4 SAVED        { 0.20f, 0.80f, 0.20f, 1.00f };
    inline constexpr ImVec4 DANGER       { 0.90f, 0.30f, 0.30f, 1.00f };
    inline constexpr ImVec4 WARNING      { 0.90f, 0.70f, 0.20f, 1.00f };
    inline constexpr ImVec4 INFO         { 0.20f, 0.50f, 0.90f, 1.00f };
    inline constexpr ImVec4 GOLD         { 1.00f, 0.80f, 0.00f, 1.00f };

    // Text roles
    inline constexpr ImVec4 LABEL        { 0.55f, 0.58f, 0.62f, 1.00f };
    inline constexpr ImVec4 VALUE        { 0.95f, 0.95f, 0.95f, 1.00f };
    inline constexpr ImVec4 EMPTY        { 0.40f, 0.42f, 0.45f, 1.00f };
    inline constexpr ImVec4 HEADER_TEXT  { 0.85f, 0.88f, 0.92f, 1.00f };
    inline constexpr ImVec4 MUTED        { 0.67f, 0.70f, 0.75f, 1.00f };

    // Pulse/Glow base
    inline constexpr ImVec4 PULSE_BASE   { 0.60f, 0.50f, 0.00f, 1.00f };

    // Misc
    inline constexpr float DISABLED_ALPHA = 0.5f;

    // Theme-adaptive text colors (samples from current ImGui style)
    inline ImVec4 TextPrimary() { return ImGui::GetStyleColorVec4(ImGuiCol_Text); }
    inline ImVec4 TextDim()     { return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled); }

    // Helpers for deriving hover/active variants
    constexpr ImVec4 Lighten(const ImVec4& c, float amount = 0.12f) {
        return ImVec4(std::clamp(c.x + amount, 0.0f, 1.0f),
                      std::clamp(c.y + amount, 0.0f, 1.0f),
                      std::clamp(c.z + amount, 0.0f, 1.0f), c.w);
    }
    constexpr ImVec4 Darken(const ImVec4& c, float amount = 0.12f) {
        return ImVec4(std::clamp(c.x - amount, 0.0f, 1.0f),
                      std::clamp(c.y - amount, 0.0f, 1.0f),
                      std::clamp(c.z - amount, 0.0f, 1.0f), c.w);
    }
} // namespace SemanticColors

// ============================================================================
// Theme selection
// ============================================================================
enum class ThemeType {
    DocumentLight = 0,
    DocumentDark,
    DocumentMidnight,
    MaterialFlat,
    PhotoshopStyle,
    Cherry,
    Darcula,
    DarculaDarker,
    LightRounded,
    SoDarkAccentBlue,
    SoDarkAccentYellow,
    SoDarkAccentRed,
    ShadesOfGray,
    MicrosoftStyle,
    WhiteIsWhite,
    BlackIsBlack
};

struct ThemeInfo {
    ThemeType type;
    const char* name;
    const char* description;
    ImVec4 preview_color;
};

constexpr ThemeInfo AVAILABLE_THEMES[] = {
    { ThemeType::DocumentLight, "Document Light", "Warm parchment and ink", {0.96f, 0.94f, 0.89f, 1.0f} },
    { ThemeType::DocumentDark, "Document Dark", "Inkwell and candlelight", {0.11f, 0.10f, 0.08f, 1.0f} },
    { ThemeType::DocumentMidnight, "Document Midnight", "Warm charcoal and power-tool orange", {0.08f, 0.07f, 0.04f, 1.0f} },
    { ThemeType::MaterialFlat, "Material Flat", "Material Design flat style", {0.17f, 0.19f, 0.24f, 1.0f} },
    { ThemeType::PhotoshopStyle, "Photoshop", "Adobe Photoshop inspired", {0.18f, 0.18f, 0.18f, 1.0f} },
    { ThemeType::Cherry, "Cherry", "Dark cherry blossom", {0.13f, 0.14f, 0.17f, 1.0f} },
    { ThemeType::Darcula, "Darcula", "JetBrains Darcula inspired", {0.24f, 0.25f, 0.25f, 1.0f} },
    { ThemeType::DarculaDarker, "Darcula Darker", "Darker Darcula variant", {0.14f, 0.12f, 0.12f, 1.0f} },
    { ThemeType::LightRounded, "Light Rounded", "Clean light with rounded corners", {0.96f, 0.95f, 0.95f, 1.0f} },
    { ThemeType::SoDarkAccentBlue, "So Dark (Blue)", "Deep dark with blue accents", {0.10f, 0.10f, 0.10f, 1.0f} },
    { ThemeType::SoDarkAccentYellow, "So Dark (Yellow)", "Deep dark with yellow accents", {0.10f, 0.10f, 0.10f, 1.0f} },
    { ThemeType::SoDarkAccentRed, "So Dark (Red)", "Deep dark with red accents", {0.10f, 0.10f, 0.10f, 1.0f} },
    { ThemeType::ShadesOfGray, "Shades of Gray", "Desaturated professional gray", {0.95f, 0.95f, 0.95f, 1.0f} },
    { ThemeType::MicrosoftStyle, "Microsoft", "Microsoft Fluent inspired", {0.95f, 0.95f, 0.95f, 1.0f} },
    { ThemeType::WhiteIsWhite, "White Is White", "High contrast white", {1.0f, 1.0f, 1.0f, 1.0f} },
    { ThemeType::BlackIsBlack, "Black Is Black", "True black with blue accents", {0.0f, 0.0f, 0.0f, 1.0f} },
};

constexpr size_t THEME_COUNT = std::size(AVAILABLE_THEMES);

inline const char* GetThemeName(ThemeType type) {
    const auto it = std::ranges::find(AVAILABLE_THEMES, type, &ThemeInfo::type);
    if (it != std::end(AVAILABLE_THEMES)) {
        return it->name;
    }
    return "Unknown";
}

// ============================================================================
// Theme helper functions (from hello_imgui)
// ============================================================================
namespace ThemeHelpers {
    enum class ColorCategory { Bg, Front, Text, FrameBg };

    inline ColorCategory GetColorCategory(ImGuiCol_ col) {
        if (col == ImGuiCol_FrameBg) return ColorCategory::FrameBg;
        if (col == ImGuiCol_WindowBg || col == ImGuiCol_ChildBg || col == ImGuiCol_PopupBg)
            return ColorCategory::Bg;
        if (col == ImGuiCol_Text || col == ImGuiCol_TextDisabled)
            return ColorCategory::Text;
        return ColorCategory::Front;
    }

    inline void ApplyHue(ImGuiStyle& style, float hue) {
        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            ImVec4& col = style.Colors[i];
            float h, s, v;
            ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, h, s, v);
            h = hue;
            ImGui::ColorConvertHSVtoRGB(h, s, v, col.x, col.y, col.z);
        }
    }

    inline void ApplySaturationMultiplier(ImGuiStyle& style, float sat_mul, const ImGuiStyle& ref) {
        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            ImVec4& col = style.Colors[i];
            const ImVec4& col_ref = ref.Colors[i];
            float h, s, v;
            ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, h, s, v);
            float h_ref, s_ref, v_ref;
            ImGui::ColorConvertRGBtoHSV(col_ref.x, col_ref.y, col_ref.z, h_ref, s_ref, v_ref);
            s = s_ref * sat_mul;
            ImGui::ColorConvertHSVtoRGB(h, s, v, col.x, col.y, col.z);
        }
    }

    inline void ApplyValueMultiplier(ImGuiStyle& style, float val_mul, const ImGuiStyle& ref, ColorCategory cat) {
        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            if (GetColorCategory(static_cast<ImGuiCol_>(i)) == cat) {
                ImVec4& col = style.Colors[i];
                const ImVec4& col_ref = ref.Colors[i];
                float h, s, v;
                ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, h, s, v);
                float h_ref, s_ref, v_ref;
                ImGui::ColorConvertRGBtoHSV(col_ref.x, col_ref.y, col_ref.z, h_ref, s_ref, v_ref);
                v = v_ref * val_mul;
                ImGui::ColorConvertHSVtoRGB(h, s, v, col.x, col.y, col.z);
            }
        }
    }

    inline void ApplyValueMultiplierFront(ImGuiStyle& s, float m, const ImGuiStyle& r) {
        ApplyValueMultiplier(s, m, r, ColorCategory::Front);
    }
    inline void ApplyValueMultiplierBg(ImGuiStyle& s, float m, const ImGuiStyle& r) {
        ApplyValueMultiplier(s, m, r, ColorCategory::Bg);
    }
    inline void ApplyValueMultiplierText(ImGuiStyle& s, float m, const ImGuiStyle& r) {
        ApplyValueMultiplier(s, m, r, ColorCategory::Text);
    }
    inline void ApplyValueMultiplierFrameBg(ImGuiStyle& s, float m, const ImGuiStyle& r) {
        ApplyValueMultiplier(s, m, r, ColorCategory::FrameBg);
    }

    inline ImVec4 ColorValueMultiply(ImVec4 col, float val_mul) {
        float h, s, v;
        ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, h, s, v);
        v *= val_mul;
        ImGui::ColorConvertHSVtoRGB(h, s, v, col.x, col.y, col.z);
        return col;
    }

    inline ImVec4 ColorSetValue(ImVec4 col, float value) {
        float h, s, v;
        ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, h, s, v);
        v = value;
        ImGui::ColorConvertHSVtoRGB(h, s, v, col.x, col.y, col.z);
        return col;
    }
} // namespace ThemeHelpers

// ============================================================================
// SoDark Theme (parameterized by hue)
// ============================================================================
inline void ApplySoDarkTheme(float hue) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
    colors[ImGuiCol_Border]                = ImVec4(0.39f, 0.39f, 0.39f, 0.59f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.25f, 0.25f, 0.25f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_Button]                = ImVec4(0.30f, 0.30f, 0.30f, 0.54f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
    colors[ImGuiCol_Separator]             = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_DockingPreview]        = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_DockingEmptyBg]        = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

    style.WindowPadding     = ImVec2(8.00f, 8.00f);
    style.FramePadding      = ImVec2(5.00f, 2.00f);
    style.CellPadding       = ImVec2(6.00f, 6.00f);
    style.ItemSpacing       = ImVec2(6.00f, 6.00f);
    style.ItemInnerSpacing  = ImVec2(6.00f, 6.00f);
    style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
    style.IndentSpacing     = 25;
    style.ScrollbarSize     = 15;
    style.GrabMinSize       = 10;
    style.WindowBorderSize  = 1;
    style.ChildBorderSize   = 1;
    style.PopupBorderSize   = 1;
    style.FrameBorderSize   = 1;
    style.TabBorderSize     = 1;
    style.WindowRounding    = 7;
    style.ChildRounding     = 4;
    style.FrameRounding     = 3;
    style.PopupRounding     = 4;
    style.ScrollbarRounding = 9;
    style.GrabRounding      = 3;
    style.TabRounding       = 4;

    ThemeHelpers::ApplyHue(style, hue);
}

// ============================================================================
// Material Flat Theme
// ============================================================================
inline void ApplyMaterialFlatTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha         = 1.0f;
    style.DisabledAlpha = 0.5f;
    style.WindowPadding     = ImVec2(8.0f, 8.0f);
    style.WindowRounding    = 0.0f;
    style.WindowBorderSize  = 1.0f;
    style.WindowMinSize     = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign  = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding     = 0.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupRounding     = 0.0f;
    style.PopupBorderSize   = 1.0f;
    style.FramePadding      = ImVec2(4.0f, 3.0f);
    style.FrameRounding     = 0.0f;
    style.FrameBorderSize   = 0.0f;
    style.ItemSpacing       = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);
    style.CellPadding       = ImVec2(4.0f, 2.0f);
    style.IndentSpacing     = 21.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize     = 14.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabMinSize       = 10.0f;
    style.GrabRounding      = 0.0f;
    style.TabRounding       = 0.0f;
    style.TabBorderSize     = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Left;
    style.ButtonTextAlign     = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                  = ImVec4(0.83f, 0.85f, 0.88f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.83f, 0.85f, 0.88f, 0.50f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.17f, 0.19f, 0.24f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.16f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.17f, 0.19f, 0.24f, 1.00f);
    colors[ImGuiCol_Border]                = ImVec4(0.24f, 0.34f, 0.53f, 1.00f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.11f, 0.11f, 0.14f, 0.50f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.31f, 0.62f, 0.93f, 0.25f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.31f, 0.62f, 0.93f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.11f, 0.11f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.11f, 0.11f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.11f, 0.11f, 0.14f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.11f, 0.11f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.02f, 0.02f, 0.02f, 0.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.31f, 0.62f, 0.93f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.35f, 0.41f, 0.44f, 0.50f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.15f, 0.17f, 0.21f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.31f, 0.62f, 0.93f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.23f, 0.26f, 0.32f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.31f, 0.62f, 0.93f, 0.25f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.31f, 0.62f, 0.93f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.11f, 0.11f, 0.14f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.31f, 0.62f, 0.93f, 0.25f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.31f, 0.62f, 0.93f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.15f, 0.17f, 0.21f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.31f, 0.62f, 0.93f, 0.25f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.31f, 0.62f, 0.93f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.15f, 0.17f, 0.21f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.31f, 0.62f, 0.93f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.11f, 0.11f, 0.14f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.20f, 0.23f, 0.28f, 1.00f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.20f, 0.23f, 0.28f, 0.50f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.20f, 0.23f, 0.28f, 1.00f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.20f, 0.23f, 0.28f, 0.75f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.11f, 0.11f, 0.14f, 0.75f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.11f, 0.11f, 0.14f, 0.75f);
}

// ============================================================================
// Photoshop Theme
// ============================================================================
inline void ApplyPhotoshopTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha         = 1.0f;
    style.DisabledAlpha = 0.60f;
    style.WindowPadding     = ImVec2(8.0f, 8.0f);
    style.WindowRounding    = 4.0f;
    style.WindowBorderSize  = 1.0f;
    style.WindowMinSize     = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign  = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding     = 4.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupRounding     = 2.0f;
    style.PopupBorderSize   = 1.0f;
    style.FramePadding      = ImVec2(4.0f, 3.0f);
    style.FrameRounding     = 2.0f;
    style.FrameBorderSize   = 1.0f;
    style.ItemSpacing       = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);
    style.CellPadding       = ImVec2(4.0f, 2.0f);
    style.IndentSpacing     = 21.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize     = 13.0f;
    style.ScrollbarRounding = 12.0f;
    style.GrabMinSize       = 7.0f;
    style.GrabRounding      = 0.0f;
    style.TabRounding       = 0.0f;
    style.TabBorderSize     = 1.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign     = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.28f, 0.28f, 0.28f, 0.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_Border]                = ImVec4(0.36f, 0.36f, 0.36f, 0.60f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(1.00f, 0.39f, 0.00f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.00f, 0.39f, 0.00f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(1.00f, 1.00f, 1.00f, 0.12f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(1.00f, 1.00f, 1.00f, 0.16f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(1.00f, 1.00f, 1.00f, 0.39f);
    colors[ImGuiCol_Header]                = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(1.00f, 0.39f, 0.00f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.00f, 1.00f, 1.00f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.00f, 0.39f, 0.00f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.39f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.58f, 0.58f, 0.58f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.39f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(1.00f, 1.00f, 1.00f, 0.16f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 0.39f, 0.00f, 1.00f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(1.00f, 0.39f, 0.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.39f, 0.00f, 1.00f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.00f, 0.00f, 0.00f, 0.59f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.00f, 0.00f, 0.00f, 0.59f);
}

// ============================================================================
// Cherry Theme
// ============================================================================
inline void ApplyCherryTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha         = 1.0f;
    style.DisabledAlpha = 0.60f;
    style.WindowPadding     = ImVec2(6.0f, 3.0f);
    style.WindowRounding    = 0.0f;
    style.WindowBorderSize  = 1.0f;
    style.WindowMinSize     = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign  = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding     = 0.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupRounding     = 0.0f;
    style.PopupBorderSize   = 1.0f;
    style.FramePadding      = ImVec2(5.0f, 1.0f);
    style.FrameRounding     = 3.0f;
    style.FrameBorderSize   = 1.0f;
    style.ItemSpacing       = ImVec2(7.0f, 3.0f);
    style.ItemInnerSpacing  = ImVec2(1.0f, 1.0f);
    style.CellPadding       = ImVec2(4.0f, 2.0f);
    style.IndentSpacing     = 6.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize     = 13.0f;
    style.ScrollbarRounding = 16.0f;
    style.GrabMinSize       = 20.0f;
    style.GrabRounding      = 2.0f;
    style.TabRounding       = 4.0f;
    style.TabBorderSize     = 1.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign     = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                  = ImVec4(0.86f, 0.93f, 0.89f, 0.88f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.20f, 0.22f, 0.27f, 0.90f);
    colors[ImGuiCol_Border]                = ImVec4(0.54f, 0.48f, 0.25f, 0.16f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.45f, 0.20f, 0.30f, 0.78f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.23f, 0.20f, 0.27f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.50f, 0.07f, 0.25f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.20f, 0.22f, 0.27f, 0.47f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.45f, 0.20f, 0.30f, 0.78f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.45f, 0.20f, 0.30f, 0.86f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.45f, 0.20f, 0.30f, 0.76f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.45f, 0.20f, 0.30f, 0.86f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.50f, 0.07f, 0.25f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.45f, 0.20f, 0.30f, 0.78f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.13f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.45f, 0.20f, 0.30f, 0.43f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    ThemeHelpers::ApplyHue(style, 0.853f);
    ThemeHelpers::ApplyValueMultiplierFront(style, 0.971f, style);
    ThemeHelpers::ApplyValueMultiplierBg(style, 0.969f, style);
    ThemeHelpers::ApplyValueMultiplierText(style, 0.937f, style);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
}

// ============================================================================
// Darcula Theme
// ============================================================================
inline void ApplyDarculaTheme(
    float rounding = 1.f,
    float hue = -1.f,
    float saturation_multiplier = 1.f,
    float value_multiplier_front = 1.f,
    float value_multiplier_bg = 1.f,
    float alpha_bg_transparency = 1.f)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                  = ImVec4(0.73f, 0.73f, 0.73f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.24f, 0.25f, 0.25f, 0.94f * alpha_bg_transparency);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.24f, 0.25f, 0.25f, 0.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.24f, 0.25f, 0.25f, 0.94f * alpha_bg_transparency);
    colors[ImGuiCol_Border]                = ImVec4(0.53f, 0.53f, 0.53f, 0.50f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.16f, 0.16f, 0.16f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.17f, 0.17f, 0.17f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.45f, 0.67f, 1.00f, 0.67f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.47f, 0.47f, 0.47f, 0.67f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.27f, 0.29f, 0.29f, 0.80f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.27f, 0.29f, 0.29f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.42f, 0.41f, 0.52f, 0.51f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.22f, 0.31f, 0.42f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.14f, 0.19f, 0.26f, 0.91f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.90f, 0.90f, 0.90f, 0.83f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.70f, 0.70f, 0.70f, 0.62f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.30f, 0.33f, 0.36f, 0.62f);
    colors[ImGuiCol_Button]                = ImVec4(0.33f, 0.35f, 0.36f, 0.40f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.37f, 0.39f, 0.40f, 0.69f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.20f, 0.22f, 0.23f, 0.69f);
    colors[ImGuiCol_Header]                = ImVec4(0.33f, 0.35f, 0.36f, 0.53f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.37f, 0.39f, 0.40f, 0.69f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.20f, 0.22f, 0.23f, 0.69f);
    colors[ImGuiCol_Separator]             = ImVec4(0.44f, 0.44f, 0.44f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.37f, 0.39f, 0.40f, 0.69f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.20f, 0.22f, 0.23f, 0.69f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.33f, 0.35f, 0.36f, 0.40f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.37f, 0.39f, 0.40f, 0.69f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.20f, 0.22f, 0.23f, 0.69f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.16f, 0.29f, 0.48f, 0.86f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.13f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    style.WindowRounding    = rounding;
    style.FrameRounding     = rounding;
    style.GrabRounding      = rounding;
    style.PopupRounding     = rounding;
    style.ScrollbarRounding = rounding;
    style.TabRounding       = rounding;
    style.FrameBorderSize   = 1.f;

    if (hue >= 0.f)
        ThemeHelpers::ApplyHue(style, hue);
    if (saturation_multiplier != 1.f)
        ThemeHelpers::ApplySaturationMultiplier(style, saturation_multiplier, style);
    if (value_multiplier_front != 1.f)
        ThemeHelpers::ApplyValueMultiplierFront(style, value_multiplier_front, style);
    if (value_multiplier_bg != 1.f)
        ThemeHelpers::ApplyValueMultiplierBg(style, value_multiplier_bg, style);
}

// ============================================================================
// Darcula Darker Theme
// ============================================================================
inline void ApplyDarculaDarkerTheme() {
    ApplyDarculaTheme(3.f, 0.61f, 0.993f, 0.981f, 0.585f, 0.92f);

    ImGuiStyle& style = ImGui::GetStyle();
    ThemeHelpers::ApplyValueMultiplierFrameBg(style, 2.5f, style);
    style.Colors[ImGuiCol_Header] = ThemeHelpers::ColorValueMultiply(style.Colors[ImGuiCol_Header], 1.4f);
    style.Colors[ImGuiCol_Text]         = ImVec4(0.88f, 0.88f, 0.88f, 1.f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.f);
    style.Colors[ImGuiCol_FrameBg]      = ImVec4(37.f / 255.f, 31.f / 255.f, 31.f / 255.f, 222.f / 255.f);
}

// ============================================================================
// Light Rounded Theme
// ============================================================================
inline void ApplyLightRoundedTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding    = 3.f;
    style.ChildRounding     = 3.f;
    style.PopupRounding     = 3.f;
    style.FrameRounding     = 3.f;
    style.GrabRounding      = 3.f;
    style.TabRounding       = 3.f;
    style.ScrollbarRounding = 9.f;

    style.Alpha         = 1.0f;
    style.DisabledAlpha = 0.60f;
    style.WindowPadding     = ImVec2(9.1f, 8.0f);
    style.WindowBorderSize  = 0.0f;
    style.WindowMinSize     = ImVec2(20.0f, 32.0f);
    style.WindowTitleAlign  = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 0.0f;
    style.FramePadding      = ImVec2(2.1f, 1.0f);
    style.FrameBorderSize   = 0.0f;
    style.ItemSpacing       = ImVec2(3.3f, 3.7f);
    style.ItemInnerSpacing  = ImVec2(3.2f, 4.0f);
    style.CellPadding       = ImVec2(2.8f, 1.0f);
    style.IndentSpacing     = 9.2f;
    style.ColumnsMinSpacing = 6.3f;
    style.ScrollbarSize     = 15.5f;
    style.GrabMinSize       = 10.9f;
    style.TabBorderSize     = 1.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign     = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.5f);

    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                  = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.96f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
    colors[ImGuiCol_Button]                = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.35f, 0.35f, 0.35f, 0.17f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.76f, 0.80f, 0.84f, 0.93f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.59f, 0.73f, 0.88f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.92f, 0.93f, 0.93f, 0.99f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.74f, 0.82f, 0.91f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.30f, 0.30f, 0.30f, 0.09f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

// ============================================================================
// Shades of Gray Theme
// ============================================================================
inline void ApplyShadesOfGrayTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha         = 1.0f;
    style.DisabledAlpha = 0.60f;
    style.WindowRounding = 0.0f;

    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                  = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_Border]                = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.00f, 0.47f, 0.84f, 0.20f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
    colors[ImGuiCol_Button]                = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.00f, 0.47f, 0.84f, 0.20f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.00f, 0.47f, 0.84f, 0.20f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.75f, 0.75f, 0.80f, 1.00f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.56f, 0.56f, 0.60f, 0.15f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    colors[ImGuiCol_TitleBg]           = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TitleBgActive]     = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]  = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_Tab]               = ImVec4(0.78f, 0.79f, 0.81f, 0.86f);
    colors[ImGuiCol_TabHovered]        = ImVec4(0.66f, 0.73f, 0.81f, 0.80f);
    colors[ImGuiCol_TabActive]         = ImVec4(0.59f, 0.70f, 0.84f, 1.00f);
    colors[ImGuiCol_TabUnfocused]      = ImVec4(0.64f, 0.74f, 0.86f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]= ImVec4(0.70f, 0.80f, 0.93f, 1.00f);

    ThemeHelpers::ApplySaturationMultiplier(style, 0.21f, style);
    ThemeHelpers::ApplyValueMultiplierBg(style, 1.0f, style);
    ThemeHelpers::ApplyValueMultiplierFront(style, 1.0f, style);
}

// ============================================================================
// Microsoft Style Theme
// ============================================================================
inline void ApplyMicrosoftStyleTheme() {
    ApplyShadesOfGrayTheme();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding     = ImVec2(4.0f, 6.0f);
    style.WindowBorderSize  = 0.0f;
    style.WindowMinSize     = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign  = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding     = 0.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupRounding     = 0.0f;
    style.PopupBorderSize   = 1.0f;
    style.FramePadding      = ImVec2(8.0f, 4.0f);
    style.FrameRounding     = 0.0f;
    style.FrameBorderSize   = 1.0f;
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
    style.CellPadding       = ImVec2(4.0f, 2.0f);
    style.IndentSpacing     = 20.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize     = 20.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabMinSize       = 5.0f;
    style.GrabRounding      = 0.0f;
    style.TabRounding       = 4.0f;
    style.TabBorderSize     = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign     = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
}

// ============================================================================
// White Is White Theme
// ============================================================================
inline void ApplyWhiteIsWhiteTheme() {
    ApplyShadesOfGrayTheme();

    ImGuiStyle& style = ImGui::GetStyle();
    ThemeHelpers::ApplyValueMultiplierFront(style, 0.94f, style);
    ThemeHelpers::ApplyValueMultiplierBg(style, 7.f, style);
    ThemeHelpers::ApplyValueMultiplierFrameBg(style, 0.91f, style);
    style.Colors[ImGuiCol_FrameBg] = ThemeHelpers::ColorSetValue(style.Colors[ImGuiCol_FrameBg], 0.99f);
    style.FrameBorderSize = 1.f;
    style.FrameRounding   = 3.f;
}

// ============================================================================
// Black Is Black Theme
// ============================================================================
inline void ApplyBlackIsBlackTheme() {
    ApplySoDarkTheme(0.548f);

    ImGuiStyle& style = ImGui::GetStyle();
    ThemeHelpers::ApplyValueMultiplierFront(style, 1.356f, style);
    ThemeHelpers::ApplyValueMultiplierBg(style, 0.f, style);
    ThemeHelpers::ApplyValueMultiplierFrameBg(style, 1.12f, style);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(27.f / 255.f, 27.f / 255.f, 27.f / 255.f, 231.f / 255.f);
    style.Colors[ImGuiCol_Header]  = ImVec4(0.25f, 0.25f, 0.25f, 1.f);
}

// ============================================================================
// Document Midnight Theme
// ============================================================================
inline void ApplyDocumentMidnight() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding    = 4.0f;
    style.FrameRounding     = 3.0f;
    style.GrabRounding      = 3.0f;
    style.PopupRounding     = 4.0f;
    style.ScrollbarRounding = 3.0f;
    style.TabRounding       = 3.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.WindowPadding     = ImVec2(8.0f, 8.0f);
    style.FramePadding      = ImVec2(6.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);

    ImVec4* colors = style.Colors;

    ImVec4 bg_deep      = ImVec4(0.078f, 0.071f, 0.043f, 1.00f);
    ImVec4 bg_medium    = ImVec4(0.118f, 0.110f, 0.075f, 1.00f);
    ImVec4 bg_light     = ImVec4(0.153f, 0.145f, 0.106f, 1.00f);
    ImVec4 bg_lighter   = ImVec4(0.196f, 0.188f, 0.157f, 1.00f);
    ImVec4 accent       = ImVec4(0.784f, 0.447f, 0.180f, 1.00f);
    ImVec4 accent_hover = ImVec4(0.847f, 0.514f, 0.290f, 1.00f);
    ImVec4 accent_active= ImVec4(0.647f, 0.361f, 0.133f, 1.00f);
    ImVec4 text_main    = ImVec4(0.969f, 0.969f, 0.957f, 1.00f);
    ImVec4 text_dim     = ImVec4(0.533f, 0.514f, 0.478f, 1.00f);

    colors[ImGuiCol_Text]                  = text_main;
    colors[ImGuiCol_TextDisabled]          = text_dim;
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.784f, 0.447f, 0.180f, 0.28f);
    colors[ImGuiCol_WindowBg]              = bg_deep;
    colors[ImGuiCol_ChildBg]               = bg_deep;
    colors[ImGuiCol_PopupBg]               = bg_medium;
    colors[ImGuiCol_Border]                = ImVec4(0.55f, 0.52f, 0.40f, 0.15f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TitleBg]               = bg_medium;
    colors[ImGuiCol_TitleBgActive]         = bg_light;
    colors[ImGuiCol_TitleBgCollapsed]      = bg_medium;
    colors[ImGuiCol_FrameBg]               = bg_medium;
    colors[ImGuiCol_FrameBgHovered]        = bg_light;
    colors[ImGuiCol_FrameBgActive]         = bg_lighter;
    colors[ImGuiCol_Button]                = bg_medium;
    colors[ImGuiCol_ButtonHovered]         = bg_light;
    colors[ImGuiCol_ButtonActive]          = bg_lighter;
    colors[ImGuiCol_ScrollbarBg]           = bg_deep;
    colors[ImGuiCol_ScrollbarGrab]         = bg_light;
    colors[ImGuiCol_ScrollbarGrabHovered]  = bg_lighter;
    colors[ImGuiCol_ScrollbarGrabActive]   = accent;
    colors[ImGuiCol_CheckMark]             = accent;
    colors[ImGuiCol_SliderGrab]            = accent;
    colors[ImGuiCol_SliderGrabActive]      = accent_active;
    colors[ImGuiCol_Separator]             = ImVec4(0.55f, 0.52f, 0.40f, 0.12f);
    colors[ImGuiCol_SeparatorHovered]      = accent_hover;
    colors[ImGuiCol_SeparatorActive]       = accent_active;
    colors[ImGuiCol_ResizeGrip]            = accent;
    colors[ImGuiCol_ResizeGripHovered]     = accent_hover;
    colors[ImGuiCol_ResizeGripActive]      = accent_active;
    colors[ImGuiCol_Tab]                   = bg_medium;
    colors[ImGuiCol_TabHovered]            = bg_light;
    colors[ImGuiCol_TabActive]             = bg_lighter;
    colors[ImGuiCol_TabUnfocused]          = bg_medium;
    colors[ImGuiCol_TabUnfocusedActive]    = bg_light;
    colors[ImGuiCol_MenuBarBg]             = bg_light;
    colors[ImGuiCol_Header]                = bg_medium;
    colors[ImGuiCol_HeaderHovered]         = bg_light;
    colors[ImGuiCol_HeaderActive]          = bg_lighter;
    colors[ImGuiCol_TableHeaderBg]         = bg_light;
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.55f, 0.52f, 0.40f, 0.22f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.40f, 0.38f, 0.28f, 0.10f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 0.95f, 0.85f, 0.03f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(0.784f, 0.447f, 0.180f, 0.90f);
    colors[ImGuiCol_NavHighlight]          = accent;
    colors[ImGuiCol_NavWindowingHighlight] = accent_hover;
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.08f, 0.07f, 0.04f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.04f, 0.03f, 0.02f, 0.50f);
}

// ============================================================================
// Document Dark Theme
// ============================================================================
inline void ApplyDocumentDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding    = 6.0f;
    style.FrameRounding     = 4.0f;
    style.GrabRounding      = 4.0f;
    style.PopupRounding     = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding       = 4.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.WindowPadding     = ImVec2(8.0f, 8.0f);
    style.FramePadding      = ImVec2(6.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);

    ImVec4* colors = style.Colors;

    ImVec4 bg_deep          = ImVec4(0.11f, 0.10f, 0.08f, 1.00f);
    ImVec4 bg_medium        = ImVec4(0.14f, 0.13f, 0.09f, 1.00f);
    ImVec4 bg_light         = ImVec4(0.18f, 0.16f, 0.13f, 1.00f);
    ImVec4 bg_lighter       = ImVec4(0.21f, 0.20f, 0.16f, 1.00f);
    ImVec4 accent_ink       = ImVec4(0.36f, 0.51f, 0.80f, 1.00f);
    ImVec4 accent_hover     = ImVec4(0.48f, 0.63f, 0.88f, 1.00f);
    ImVec4 accent_active    = ImVec4(0.25f, 0.40f, 0.68f, 1.00f);
    ImVec4 text_cream       = ImVec4(0.91f, 0.89f, 0.83f, 1.00f);
    ImVec4 text_dim         = ImVec4(0.54f, 0.52f, 0.47f, 1.00f);

    colors[ImGuiCol_Text]                  = text_cream;
    colors[ImGuiCol_TextDisabled]          = text_dim;
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.36f, 0.51f, 0.80f, 0.30f);
    colors[ImGuiCol_WindowBg]              = bg_deep;
    colors[ImGuiCol_ChildBg]               = bg_deep;
    colors[ImGuiCol_PopupBg]               = bg_medium;
    colors[ImGuiCol_Border]                = ImVec4(0.55f, 0.53f, 0.47f, 0.25f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TitleBg]               = bg_medium;
    colors[ImGuiCol_TitleBgActive]         = bg_light;
    colors[ImGuiCol_TitleBgCollapsed]      = bg_medium;
    colors[ImGuiCol_FrameBg]               = bg_medium;
    colors[ImGuiCol_FrameBgHovered]        = bg_light;
    colors[ImGuiCol_FrameBgActive]         = bg_lighter;
    colors[ImGuiCol_Button]                = bg_medium;
    colors[ImGuiCol_ButtonHovered]         = bg_light;
    colors[ImGuiCol_ButtonActive]          = bg_lighter;
    colors[ImGuiCol_ScrollbarBg]           = bg_deep;
    colors[ImGuiCol_ScrollbarGrab]         = bg_light;
    colors[ImGuiCol_ScrollbarGrabHovered]  = bg_lighter;
    colors[ImGuiCol_ScrollbarGrabActive]   = accent_ink;
    colors[ImGuiCol_CheckMark]             = accent_ink;
    colors[ImGuiCol_SliderGrab]            = accent_ink;
    colors[ImGuiCol_SliderGrabActive]      = accent_active;
    colors[ImGuiCol_Separator]             = ImVec4(0.55f, 0.53f, 0.47f, 0.22f);
    colors[ImGuiCol_SeparatorHovered]      = accent_hover;
    colors[ImGuiCol_SeparatorActive]       = accent_active;
    colors[ImGuiCol_ResizeGrip]            = accent_ink;
    colors[ImGuiCol_ResizeGripHovered]     = accent_hover;
    colors[ImGuiCol_ResizeGripActive]      = accent_active;
    colors[ImGuiCol_Tab]                   = bg_medium;
    colors[ImGuiCol_TabHovered]            = bg_light;
    colors[ImGuiCol_TabActive]             = bg_lighter;
    colors[ImGuiCol_TabUnfocused]          = bg_medium;
    colors[ImGuiCol_TabUnfocusedActive]    = bg_light;
    colors[ImGuiCol_MenuBarBg]             = bg_medium;
    colors[ImGuiCol_Header]                = bg_medium;
    colors[ImGuiCol_HeaderHovered]         = bg_light;
    colors[ImGuiCol_HeaderActive]          = bg_lighter;
    colors[ImGuiCol_TableHeaderBg]         = bg_medium;
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.55f, 0.53f, 0.47f, 0.40f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.40f, 0.38f, 0.34f, 0.25f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 0.90f, 0.04f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(0.36f, 0.51f, 0.80f, 0.90f);
    colors[ImGuiCol_NavHighlight]          = accent_ink;
    colors[ImGuiCol_NavWindowingHighlight] = accent_hover;
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.10f, 0.10f, 0.10f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.05f, 0.05f, 0.05f, 0.45f);
}

// ============================================================================
// Document Light Theme
// ============================================================================
inline void ApplyDocumentLightTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding    = 6.0f;
    style.FrameRounding     = 4.0f;
    style.GrabRounding      = 4.0f;
    style.PopupRounding     = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding       = 4.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.WindowPadding     = ImVec2(8.0f, 8.0f);
    style.FramePadding      = ImVec2(6.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);

    ImVec4* colors = style.Colors;

    ImVec4 bg_paper         = ImVec4(0.96f, 0.94f, 0.89f, 1.00f);
    ImVec4 bg_medium        = ImVec4(0.90f, 0.88f, 0.83f, 1.00f);
    ImVec4 bg_light         = ImVec4(0.84f, 0.82f, 0.77f, 1.00f);
    ImVec4 bg_lighter       = ImVec4(0.77f, 0.75f, 0.70f, 1.00f);
    ImVec4 accent_ink       = ImVec4(0.18f, 0.32f, 0.62f, 1.00f);
    ImVec4 accent_hover     = ImVec4(0.24f, 0.42f, 0.78f, 1.00f);
    ImVec4 accent_active    = ImVec4(0.12f, 0.22f, 0.50f, 1.00f);
    ImVec4 text_ink         = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    ImVec4 text_dim         = ImVec4(0.42f, 0.42f, 0.46f, 1.00f);

    colors[ImGuiCol_Text]                  = text_ink;
    colors[ImGuiCol_TextDisabled]          = text_dim;
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.18f, 0.32f, 0.62f, 0.25f);
    colors[ImGuiCol_WindowBg]              = bg_paper;
    colors[ImGuiCol_ChildBg]               = bg_paper;
    colors[ImGuiCol_PopupBg]               = bg_medium;
    colors[ImGuiCol_Border]                = ImVec4(0.62f, 0.60f, 0.55f, 0.80f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TitleBg]               = bg_medium;
    colors[ImGuiCol_TitleBgActive]         = bg_light;
    colors[ImGuiCol_TitleBgCollapsed]      = bg_medium;
    colors[ImGuiCol_FrameBg]               = bg_medium;
    colors[ImGuiCol_FrameBgHovered]        = bg_light;
    colors[ImGuiCol_FrameBgActive]         = bg_lighter;
    colors[ImGuiCol_Button]                = bg_medium;
    colors[ImGuiCol_ButtonHovered]         = bg_light;
    colors[ImGuiCol_ButtonActive]          = bg_lighter;
    colors[ImGuiCol_ScrollbarBg]           = bg_paper;
    colors[ImGuiCol_ScrollbarGrab]         = bg_light;
    colors[ImGuiCol_ScrollbarGrabHovered]  = bg_lighter;
    colors[ImGuiCol_ScrollbarGrabActive]   = accent_ink;
    colors[ImGuiCol_CheckMark]             = accent_ink;
    colors[ImGuiCol_SliderGrab]            = accent_ink;
    colors[ImGuiCol_SliderGrabActive]      = accent_active;
    colors[ImGuiCol_Separator]             = ImVec4(0.62f, 0.60f, 0.55f, 0.60f);
    colors[ImGuiCol_SeparatorHovered]      = accent_hover;
    colors[ImGuiCol_SeparatorActive]       = accent_active;
    colors[ImGuiCol_ResizeGrip]            = accent_ink;
    colors[ImGuiCol_ResizeGripHovered]     = accent_hover;
    colors[ImGuiCol_ResizeGripActive]      = accent_active;
    colors[ImGuiCol_Tab]                   = bg_medium;
    colors[ImGuiCol_TabHovered]            = bg_light;
    colors[ImGuiCol_TabActive]             = bg_lighter;
    colors[ImGuiCol_TabUnfocused]          = bg_medium;
    colors[ImGuiCol_TabUnfocusedActive]    = bg_light;
    colors[ImGuiCol_MenuBarBg]             = bg_medium;
    colors[ImGuiCol_Header]                = bg_medium;
    colors[ImGuiCol_HeaderHovered]         = bg_light;
    colors[ImGuiCol_HeaderActive]          = bg_lighter;
    colors[ImGuiCol_TableHeaderBg]         = bg_medium;
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.62f, 0.60f, 0.55f, 0.80f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.75f, 0.73f, 0.68f, 0.50f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.60f, 0.58f, 0.53f, 0.12f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(0.18f, 0.32f, 0.62f, 0.90f);
    colors[ImGuiCol_NavHighlight]          = accent_ink;
    colors[ImGuiCol_NavWindowingHighlight] = accent_hover;
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.70f, 0.70f, 0.70f, 0.35f);
}

inline void ApplyTheme(ThemeType type) {
    switch (type) {
        case ThemeType::DocumentDark: ApplyDocumentDarkTheme(); break;
        case ThemeType::DocumentMidnight: ApplyDocumentMidnight(); break;
        case ThemeType::MaterialFlat: ApplyMaterialFlatTheme(); break;
        case ThemeType::PhotoshopStyle: ApplyPhotoshopTheme(); break;
        case ThemeType::Cherry: ApplyCherryTheme(); break;
        case ThemeType::Darcula: ApplyDarculaTheme(); break;
        case ThemeType::DarculaDarker: ApplyDarculaDarkerTheme(); break;
        case ThemeType::LightRounded: ApplyLightRoundedTheme(); break;
        case ThemeType::SoDarkAccentBlue: ApplySoDarkTheme(0.548f); break;
        case ThemeType::SoDarkAccentYellow: ApplySoDarkTheme(0.163f); break;
        case ThemeType::SoDarkAccentRed: ApplySoDarkTheme(0.f); break;
        case ThemeType::ShadesOfGray: ApplyShadesOfGrayTheme(); break;
        case ThemeType::MicrosoftStyle: ApplyMicrosoftStyleTheme(); break;
        case ThemeType::WhiteIsWhite: ApplyWhiteIsWhiteTheme(); break;
        case ThemeType::BlackIsBlack: ApplyBlackIsBlackTheme(); break;
        default: ApplyDocumentLightTheme(); break;
    }
}
