#pragma once

#include <string>
#include <functional>

namespace MapEditor {
namespace UI {

/**
 * ImGui modal for confirming unsaved changes when closing a map.
 * 
 * Used by Close Map and Exit flows.
 */
class UnsavedChangesModal {
public:
    enum class Result {
        None,       // Modal still open or not shown
        Save,       // User chose to save
        Discard,    // User chose to discard changes
        Cancel      // User cancelled the operation
    };
    
    using SaveCallback = std::function<void()>;
    
    /**
     * Show the modal for a specific map.
     * @param map_name Display name of the map (e.g. filename)
     */
    void show(const std::string& map_name);
    
    /**
     * Render the modal. Call this every frame.
     * @return Result of user action, or None if still pending
     */
    Result render();
    
    /**
     * Check if modal is currently shown
     */
    bool isOpen() const { return is_open_; }
    
    /**
     * Set callback to invoke when user chooses Save
     */
    void setSaveCallback(SaveCallback cb) { on_save_ = std::move(cb); }
    
    /**
     * Set callback to invoke when user chooses Discard
     */
    void setDiscardCallback(SaveCallback cb) { on_discard_ = std::move(cb); }
    
private:
    bool is_open_ = false;
    bool should_open_ = false;
    std::string map_name_;
    SaveCallback on_save_;
    SaveCallback on_discard_;
};

} // namespace UI
} // namespace MapEditor
