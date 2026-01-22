// ImHotKey GLFW Edition
// Based on ImHotKey v1.0 by Cedric Guillemet
// Modified: GLFW keys + mouse + color coding + resizable + GREEN pulse
//
// The MIT License(MIT) - Copyright(c) 2019 Cedric Guillemet
//
#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include <GLFW/glfw3.h>
#include <cmath>

namespace ImHotKey
{
    enum class EditResult { None, Applied, Closed };

    struct HotKey
    {
        const char* functionName;
        const char* functionLib;
        int key;
        int mods;
        bool isMouse = false;
    };

    struct Key
    {
        const char* lib = nullptr;
        int glfwKey = 0;
        bool isMod = false;
        int modFlag = 0;
        float offset = 0;
        float width = 40;
    };

    struct MouseButton { const char* lib; int glfwButton; };

    // Colors - Selected keys pulse GREEN
    namespace Colors {
        static constexpr ImU32 ModDefault = IM_COL32(80, 60, 120, 200);
        static constexpr ImU32 KeyDefault = IM_COL32(60, 60, 70, 200);
        static constexpr ImU32 MouseDefault = IM_COL32(50, 90, 100, 200);
        static constexpr ImU32 Selected = IM_COL32(40, 200, 80, 255);  // GREEN for all selected
    }

    static const MouseButton MouseButtons[5] = {
        {"LMB", GLFW_MOUSE_BUTTON_LEFT}, {"RMB", GLFW_MOUSE_BUTTON_RIGHT},
        {"MMB", GLFW_MOUSE_BUTTON_MIDDLE}, {"Mouse4", GLFW_MOUSE_BUTTON_4}, {"Mouse5", GLFW_MOUSE_BUTTON_5}
    };

    // Fixed keyboard layout - arrow keys properly placed
    static const Key Keys[6][18] = {
        // Row 0: Function keys
        { {"Esc", GLFW_KEY_ESCAPE, false, 0, 0, 42}, {"F1", GLFW_KEY_F1, false, 0, 20, 38}, {"F2", GLFW_KEY_F2, false, 0, 0, 38}, {"F3", GLFW_KEY_F3, false, 0, 0, 38}, {"F4", GLFW_KEY_F4, false, 0, 0, 38}, {"F5", GLFW_KEY_F5, false, 0, 15, 38}, {"F6", GLFW_KEY_F6, false, 0, 0, 38}, {"F7", GLFW_KEY_F7, false, 0, 0, 38}, {"F8", GLFW_KEY_F8, false, 0, 0, 38}, {"F9", GLFW_KEY_F9, false, 0, 15, 38}, {"F10", GLFW_KEY_F10, false, 0, 0, 38}, {"F11", GLFW_KEY_F11, false, 0, 0, 38}, {"F12", GLFW_KEY_F12, false, 0, 0, 38} },
        // Row 1: Numbers
        { {"`", GLFW_KEY_GRAVE_ACCENT, false, 0, 0, 38}, {"1", GLFW_KEY_1, false, 0, 0, 38}, {"2", GLFW_KEY_2, false, 0, 0, 38}, {"3", GLFW_KEY_3, false, 0, 0, 38}, {"4", GLFW_KEY_4, false, 0, 0, 38}, {"5", GLFW_KEY_5, false, 0, 0, 38}, {"6", GLFW_KEY_6, false, 0, 0, 38}, {"7", GLFW_KEY_7, false, 0, 0, 38}, {"8", GLFW_KEY_8, false, 0, 0, 38}, {"9", GLFW_KEY_9, false, 0, 0, 38}, {"0", GLFW_KEY_0, false, 0, 0, 38}, {"-", GLFW_KEY_MINUS, false, 0, 0, 38}, {"=", GLFW_KEY_EQUAL, false, 0, 0, 38}, {"Bksp", GLFW_KEY_BACKSPACE, false, 0, 9, 66}, {"Ins", GLFW_KEY_INSERT, false, 0, 14, 38}, {"Hm", GLFW_KEY_HOME, false, 0, 0, 38}, {"PU", GLFW_KEY_PAGE_UP, false, 0, 0, 38} },
        // Row 2: QWERTY
        { {"Tab", GLFW_KEY_TAB, false, 0, 0, 56}, {"Q", GLFW_KEY_Q, false, 0, 0, 38}, {"W", GLFW_KEY_W, false, 0, 0, 38}, {"E", GLFW_KEY_E, false, 0, 0, 38}, {"R", GLFW_KEY_R, false, 0, 0, 38}, {"T", GLFW_KEY_T, false, 0, 0, 38}, {"Y", GLFW_KEY_Y, false, 0, 0, 38}, {"U", GLFW_KEY_U, false, 0, 0, 38}, {"I", GLFW_KEY_I, false, 0, 0, 38}, {"O", GLFW_KEY_O, false, 0, 0, 38}, {"P", GLFW_KEY_P, false, 0, 0, 38}, {"[", GLFW_KEY_LEFT_BRACKET, false, 0, 0, 38}, {"]", GLFW_KEY_RIGHT_BRACKET, false, 0, 0, 38}, {"\\", GLFW_KEY_BACKSLASH, false, 0, 0, 56}, {"Del", GLFW_KEY_DELETE, false, 0, 15, 38}, {"End", GLFW_KEY_END, false, 0, 0, 38}, {"PD", GLFW_KEY_PAGE_DOWN, false, 0, 0, 38} },
        // Row 3: ASDF
        { {"Caps", GLFW_KEY_CAPS_LOCK, false, 0, 0, 66}, {"A", GLFW_KEY_A, false, 0, 0, 38}, {"S", GLFW_KEY_S, false, 0, 0, 38}, {"D", GLFW_KEY_D, false, 0, 0, 38}, {"F", GLFW_KEY_F, false, 0, 0, 38}, {"G", GLFW_KEY_G, false, 0, 0, 38}, {"H", GLFW_KEY_H, false, 0, 0, 38}, {"J", GLFW_KEY_J, false, 0, 0, 38}, {"K", GLFW_KEY_K, false, 0, 0, 38}, {"L", GLFW_KEY_L, false, 0, 0, 38}, {";", GLFW_KEY_SEMICOLON, false, 0, 0, 38}, {"'", GLFW_KEY_APOSTROPHE, false, 0, 0, 38}, {"Enter", GLFW_KEY_ENTER, false, 0, 0, 86} },
        // Row 4: ZXCV + Up arrow
        { {"Shift", GLFW_KEY_LEFT_SHIFT, true, GLFW_MOD_SHIFT, 0, 86}, {"Z", GLFW_KEY_Z, false, 0, 0, 38}, {"X", GLFW_KEY_X, false, 0, 0, 38}, {"C", GLFW_KEY_C, false, 0, 0, 38}, {"V", GLFW_KEY_V, false, 0, 0, 38}, {"B", GLFW_KEY_B, false, 0, 0, 38}, {"N", GLFW_KEY_N, false, 0, 0, 38}, {"M", GLFW_KEY_M, false, 0, 0, 38}, {",", GLFW_KEY_COMMA, false, 0, 0, 38}, {".", GLFW_KEY_PERIOD, false, 0, 0, 38}, {"/", GLFW_KEY_SLASH, false, 0, 0, 38}, {"Shift", GLFW_KEY_RIGHT_SHIFT, true, GLFW_MOD_SHIFT, 0, 106}, {"Up", GLFW_KEY_UP, false, 0, 55, 38} },
        // Row 5: Bottom row + Left/Down/Right arrows
        { {"Ctrl", GLFW_KEY_LEFT_CONTROL, true, GLFW_MOD_CONTROL, 0, 56}, {"Alt", GLFW_KEY_LEFT_ALT, true, GLFW_MOD_ALT, 0, 56}, {"Space", GLFW_KEY_SPACE, false, 0, 0, 242}, {"Alt", GLFW_KEY_RIGHT_ALT, true, GLFW_MOD_ALT, 0, 56}, {"Ctrl", GLFW_KEY_RIGHT_CONTROL, true, GLFW_MOD_CONTROL, 0, 56}, {"<", GLFW_KEY_LEFT, false, 0, 141, 38}, {"Dn", GLFW_KEY_DOWN, false, 0, 0, 38}, {">", GLFW_KEY_RIGHT, false, 0, 0, 38} }
    };

    inline const char* GetKeyName(int glfwKey, bool isMouse = false)
    {
        if (isMouse) {
            for (int i = 0; i < 5; i++) if (MouseButtons[i].glfwButton == glfwKey) return MouseButtons[i].lib;
            return "Mouse?";
        }
        for (int y = 0; y < 6; y++)
            for (int x = 0; x < 18 && Keys[y][x].lib; x++)
                if (Keys[y][x].glfwKey == glfwKey) return Keys[y][x].lib;
        return "?";
    }

    inline void GetHotKeyLib(const HotKey& hk, char* buffer, size_t bufferSize)
    {
        buffer[0] = 0; char* p = buffer; size_t remaining = bufferSize;
        if (hk.mods & GLFW_MOD_CONTROL) { int w = snprintf(p, remaining, "Ctrl+"); p += w; remaining -= w; }
        if (hk.mods & GLFW_MOD_SHIFT) { int w = snprintf(p, remaining, "Shift+"); p += w; remaining -= w; }
        if (hk.mods & GLFW_MOD_ALT) { int w = snprintf(p, remaining, "Alt+"); p += w; remaining -= w; }
        snprintf(p, remaining, "%s", GetKeyName(hk.key, hk.isMouse));
    }

    inline int ImGuiKeyToGLFW(ImGuiKey imKey)
    {
        if (imKey >= ImGuiKey_A && imKey <= ImGuiKey_Z) return GLFW_KEY_A + (imKey - ImGuiKey_A);
        if (imKey >= ImGuiKey_0 && imKey <= ImGuiKey_9) return GLFW_KEY_0 + (imKey - ImGuiKey_0);
        if (imKey >= ImGuiKey_F1 && imKey <= ImGuiKey_F12) return GLFW_KEY_F1 + (imKey - ImGuiKey_F1);
        switch (imKey) {
            case ImGuiKey_Space: return GLFW_KEY_SPACE; case ImGuiKey_Escape: return GLFW_KEY_ESCAPE;
            case ImGuiKey_Enter: return GLFW_KEY_ENTER; case ImGuiKey_Tab: return GLFW_KEY_TAB;
            case ImGuiKey_Backspace: return GLFW_KEY_BACKSPACE; case ImGuiKey_Insert: return GLFW_KEY_INSERT;
            case ImGuiKey_Delete: return GLFW_KEY_DELETE; case ImGuiKey_Home: return GLFW_KEY_HOME;
            case ImGuiKey_End: return GLFW_KEY_END; case ImGuiKey_PageUp: return GLFW_KEY_PAGE_UP;
            case ImGuiKey_PageDown: return GLFW_KEY_PAGE_DOWN; case ImGuiKey_LeftArrow: return GLFW_KEY_LEFT;
            case ImGuiKey_RightArrow: return GLFW_KEY_RIGHT; case ImGuiKey_UpArrow: return GLFW_KEY_UP;
            case ImGuiKey_DownArrow: return GLFW_KEY_DOWN; case ImGuiKey_LeftShift: return GLFW_KEY_LEFT_SHIFT;
            case ImGuiKey_RightShift: return GLFW_KEY_RIGHT_SHIFT; case ImGuiKey_LeftCtrl: return GLFW_KEY_LEFT_CONTROL;
            case ImGuiKey_RightCtrl: return GLFW_KEY_RIGHT_CONTROL; case ImGuiKey_LeftAlt: return GLFW_KEY_LEFT_ALT;
            case ImGuiKey_RightAlt: return GLFW_KEY_RIGHT_ALT; case ImGuiKey_Minus: return GLFW_KEY_MINUS;
            case ImGuiKey_Equal: return GLFW_KEY_EQUAL; case ImGuiKey_LeftBracket: return GLFW_KEY_LEFT_BRACKET;
            case ImGuiKey_RightBracket: return GLFW_KEY_RIGHT_BRACKET; case ImGuiKey_Backslash: return GLFW_KEY_BACKSLASH;
            case ImGuiKey_Semicolon: return GLFW_KEY_SEMICOLON; case ImGuiKey_Apostrophe: return GLFW_KEY_APOSTROPHE;
            case ImGuiKey_GraveAccent: return GLFW_KEY_GRAVE_ACCENT; case ImGuiKey_Comma: return GLFW_KEY_COMMA;
            case ImGuiKey_Period: return GLFW_KEY_PERIOD; case ImGuiKey_Slash: return GLFW_KEY_SLASH;
            case ImGuiKey_CapsLock: return GLFW_KEY_CAPS_LOCK; default: return -1;
        }
    }

    // GREEN pulse for selected buttons
    inline ImU32 GreenPulse(float time)
    {
        float pulse = 0.6f + 0.4f * sinf(time * 5.0f);
        return IM_COL32(int(40 * pulse), int(220 * pulse), int(80 * pulse), 255);
    }

    inline EditResult Edit(HotKey* hotkey, size_t hotkeyCount, const char* popupModal, int* appliedIndex = nullptr)
    {
        static int editingHotkey = -1;
        static int selectedKey = 0;
        static int selectedMods = 0;
        static bool selectedIsMouse = false;
        static float listWidth = 200.0f;
        
        EditResult result = EditResult::None;
        if (!hotkeyCount) return result;
        
        float time = (float)ImGui::GetTime();

        ImGui::SetNextWindowSizeConstraints(ImVec2(900, 480), ImVec2(1400, 800));
        if (!ImGui::BeginPopupModal(popupModal, NULL, ImGuiWindowFlags_None))
            return result;
        
        ImVec2 windowSize = ImGui::GetContentRegionAvail();

        // LEFT: Hotkey list
        ImGui::BeginChild("HotkeyList", ImVec2(listWidth, windowSize.y - 45), true);
        for (size_t i = 0; i < hotkeyCount; i++) {
            char shortcut[64]; GetHotKeyLib(hotkey[i], shortcut, sizeof(shortcut));
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::Selectable("##sel", editingHotkey == int(i), 0, ImVec2(0, 26))) {
                editingHotkey = int(i); selectedKey = hotkey[i].key;
                selectedMods = hotkey[i].mods; selectedIsMouse = hotkey[i].isMouse;
            }
            ImGui::SameLine(6); ImGui::BeginGroup();
            ImGui::Text("%s", hotkey[i].functionName);
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", shortcut[0] ? shortcut : "(none)");
            ImGui::EndGroup(); ImGui::PopID();
        }
        if (editingHotkey == -1 && hotkeyCount > 0) {
            editingHotkey = 0; selectedKey = hotkey[0].key;
            selectedMods = hotkey[0].mods; selectedIsMouse = hotkey[0].isMouse;
        }
        ImGui::EndChild();
        
        // Splitter
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(80, 80, 90, 255));
        ImGui::Button("||", ImVec2(8, windowSize.y - 45));
        if (ImGui::IsItemActive()) { listWidth += ImGui::GetIO().MouseDelta.x; listWidth = ImClamp(listWidth, 150.0f, 300.0f); }
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        
        // RIGHT: Keyboard area with left padding
        ImGui::BeginChild("KeyboardArea", ImVec2(0, windowSize.y - 45), false);
        
        // Add left margin
        ImGui::Indent(12);
        
        // Info box
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(35, 35, 45, 220));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
        ImGui::BeginChild("InfoBox", ImVec2(-12, 38), true);
        ImGui::TextColored(ImVec4(0.55f, 0.35f, 0.8f, 1.0f), "Purple"); ImGui::SameLine(); ImGui::Text("= Modifier");
        ImGui::SameLine(0, 20); ImGui::TextColored(ImVec4(0.4f, 0.5f, 0.6f, 1.0f), "Gray"); ImGui::SameLine(); ImGui::Text("= Key");
        ImGui::SameLine(0, 20); ImGui::TextColored(ImVec4(0.2f, 0.7f, 0.7f, 1.0f), "Teal"); ImGui::SameLine(); ImGui::Text("= Mouse");
        ImGui::SameLine(0, 20); ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "GREEN"); ImGui::SameLine(); ImGui::Text("= Selected");
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
        
        // Keyboard container with rounded corners - FIXED HEIGHT
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(35, 35, 45, 220));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::BeginChild("KeyboardContainer", ImVec2(-12, 280), true);
        
        // Key detection
        for (ImGuiKey imKey = ImGuiKey_NamedKey_BEGIN; imKey < ImGuiKey_NamedKey_END; imKey = (ImGuiKey)(imKey + 1)) {
            if (ImGui::IsKeyPressed(imKey, false)) {
                int glfwKey = ImGuiKeyToGLFW(imKey);
                if (glfwKey > 0) {
                    if (glfwKey == GLFW_KEY_LEFT_CONTROL || glfwKey == GLFW_KEY_RIGHT_CONTROL) selectedMods ^= GLFW_MOD_CONTROL;
                    else if (glfwKey == GLFW_KEY_LEFT_SHIFT || glfwKey == GLFW_KEY_RIGHT_SHIFT) selectedMods ^= GLFW_MOD_SHIFT;
                    else if (glfwKey == GLFW_KEY_LEFT_ALT || glfwKey == GLFW_KEY_RIGHT_ALT) selectedMods ^= GLFW_MOD_ALT;
                    else { selectedKey = glfwKey; selectedIsMouse = false; }
                }
            }
        }

        // Draw keyboard
        int buttonId = 0;
        for (int y = 0; y < 6; y++) {
            ImGui::BeginGroup();
            int x = 0;
            while (Keys[y][x].lib) {
                const Key& key = Keys[y][x];
                float ofs = key.offset + (x ? 3.f : 0.f);
                if (x) ImGui::SameLine(0.f, ofs);
                else if (ofs >= 1.f) ImGui::Indent(ofs);

                bool isSelected = key.isMod ? (selectedMods & key.modFlag) != 0 : (!selectedIsMouse && selectedKey == key.glfwKey);
                
                ImU32 color = isSelected ? GreenPulse(time) : (key.isMod ? Colors::ModDefault : Colors::KeyDefault);

                ImGui::PushID(buttonId++);
                ImGui::PushStyleColor(ImGuiCol_Button, color);
                if (ImGui::Button(key.lib, ImVec2(key.width, 32))) {
                    if (key.isMod) selectedMods ^= key.modFlag;
                    else { selectedKey = key.glfwKey; selectedIsMouse = false; }
                }
                ImGui::PopStyleColor(); ImGui::PopID();
                x++;
            }
            ImGui::EndGroup();
        }
        
        // Mouse buttons
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.2f, 0.7f, 0.7f, 1.0f), "Mouse:");
        ImGui::SameLine();
        for (int i = 0; i < 5; i++) {
            if (i > 0) ImGui::SameLine();
            bool isSelected = selectedIsMouse && selectedKey == MouseButtons[i].glfwButton;
            ImU32 color = isSelected ? GreenPulse(time) : Colors::MouseDefault;
            ImGui::PushID(200 + i);
            ImGui::PushStyleColor(ImGuiCol_Button, color);
            if (ImGui::Button(MouseButtons[i].lib, ImVec2(65, 28))) { selectedKey = MouseButtons[i].glfwButton; selectedIsMouse = true; }
            ImGui::PopStyleColor(); ImGui::PopID();
        }
        
        ImGui::EndChild();  // KeyboardContainer
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        
        ImGui::Unindent(12);
        ImGui::EndChild();  // KeyboardArea

        // Bottom bar
        ImGui::Separator();
        if (editingHotkey >= 0) {
            ImGui::Text("Editing: %s", hotkey[editingHotkey].functionName);
            ImGui::SameLine(180);
            char newShortcut[64]; HotKey temp = {"", "", selectedKey, selectedMods, selectedIsMouse};
            GetHotKeyLib(temp, newShortcut, sizeof(newShortcut));
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "-> %s", newShortcut[0] ? newShortcut : "(none)");
        }
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 240);
        if (ImGui::Button("Clear", ImVec2(65, 24))) { selectedKey = 0; selectedMods = 0; selectedIsMouse = false; }
        ImGui::SameLine();
        bool canApply = editingHotkey >= 0 && (selectedKey != 0 || selectedMods != 0);
        ImGui::BeginDisabled(!canApply);
        if (ImGui::Button("Apply", ImVec2(65, 24))) {
            hotkey[editingHotkey].key = selectedKey; hotkey[editingHotkey].mods = selectedMods;
            hotkey[editingHotkey].isMouse = selectedIsMouse;
            if (appliedIndex) *appliedIndex = editingHotkey; result = EditResult::Applied;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Done", ImVec2(65, 24))) { result = EditResult::Closed; ImGui::CloseCurrentPopup(); }
        
        ImGui::EndPopup();
        return result;
    }
}
