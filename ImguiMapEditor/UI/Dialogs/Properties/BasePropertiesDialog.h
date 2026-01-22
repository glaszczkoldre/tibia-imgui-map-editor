#pragma once
#include <functional>
#include <imgui.h>
#include <string>
#include "Core/Config.h"
#include "ext/fontawesome6/IconsFontAwesome6.h"

namespace MapEditor::UI {

/**
 * Base class for properties dialogs to enforce consistent look and feel and reduce code duplication.
 * Handles the window shell, state management, and standard Save/Cancel buttons.
 */
class BasePropertiesDialog {
public:
    using SaveCallback = std::function<void()>;

    virtual ~BasePropertiesDialog() = default;

    /**
     * Render the dialog. Call each frame.
     * @param title Window title
     * @param min_size Minimum window size (default: auto)
     */
    void render(const char* title, const ImVec2& min_size = ImVec2(0, 0)) {
        if (!is_open_) return;

        if (min_size.x > 0 || min_size.y > 0) {
            ImGui::SetNextWindowSize(min_size, ImGuiCond_FirstUseEver);
        }

        if (ImGui::Begin(title, &is_open_, ImGuiWindowFlags_AlwaysAutoResize)) {
            // Derived class renders specific content
            renderContent();

            ImGui::Separator();

            // Standard Footer: Save and Cancel buttons
            if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save",
                              ImVec2(Config::UI::DIALOG_BUTTON_WIDTH, 0))) {
                onSave();
                if (save_callback_) {
                    save_callback_();
                }
                is_open_ = false;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save changes");

            ImGui::SameLine();

            if (ImGui::Button(ICON_FA_XMARK " Cancel",
                              ImVec2(Config::UI::DIALOG_BUTTON_WIDTH, 0))) {
                is_open_ = false;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Discard changes");
        }
        ImGui::End();

        if (!is_open_) {
            onClose();
        }
    }

    bool isOpen() const { return is_open_; }

protected:
    /**
     * Implement this to render the specific form fields.
     */
    virtual void renderContent() = 0;

    /**
     * Implement this to apply changes to the underlying object.
     * Called when "Save" is clicked.
     */
    virtual void onSave() = 0;

    /**
     * Called when the dialog is closed (either via Save, Cancel, or Window X).
     * Useful for clearing pointers to avoid dangling references.
     */
    virtual void onClose() {
        save_callback_ = nullptr;
    }

    bool is_open_ = false;
    SaveCallback save_callback_;
};

} // namespace MapEditor::UI
