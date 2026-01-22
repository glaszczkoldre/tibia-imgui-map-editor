#pragma once

#include <string>
#include <functional>

namespace MapEditor {
namespace UI {

/**
 * Generic confirmation dialog for destructive operations.
 * Shows a warning message and OK/Cancel buttons.
 */
class ConfirmationDialog {
public:
    enum class Result {
        None,       // Dialog still open
        Confirmed,  // User clicked OK/Confirm
        Cancelled   // User clicked Cancel
    };
    
    using ConfirmCallback = std::function<void()>;
    
    /**
     * Show the confirmation dialog.
     * @param title Dialog title
     * @param message Warning message to display
     * @param confirm_label Label for confirm button (default: "OK")
     */
    void show(const std::string& title, 
              const std::string& message,
              const std::string& confirm_label = "OK");
    
    /**
     * Render the dialog. Call every frame.
     * @return Result of user action
     */
    Result render();
    
    /**
     * Check if dialog is open
     */
    bool isOpen() const { return is_open_; }

private:
    bool should_open_ = false;
    bool is_open_ = false;
    std::string title_;
    std::string message_;
    std::string confirm_label_;
};

} // namespace UI
} // namespace MapEditor
