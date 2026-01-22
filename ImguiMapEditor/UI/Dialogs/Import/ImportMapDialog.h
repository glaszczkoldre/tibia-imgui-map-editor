#pragma once
#include "Domain/Position.h"
#include <filesystem>
#include <string>
#include <functional>

namespace MapEditor {
namespace UI {

/**
 * Dialog for importing another OTBM map into the current map.
 * 
 * Allows setting an offset position and merge mode.
 */
class ImportMapDialog {
public:
    struct ImportOptions {
        std::filesystem::path source_path;
        Domain::Position offset{0, 0, 7};
        bool overwrite_existing = false;  // If true, overwrite; if false, merge
    };
    
    enum class Result {
        None,       // Dialog still open
        Confirmed,  // User confirmed import
        Cancelled   // User cancelled
    };
    
    using BrowseCallback = std::function<std::filesystem::path()>;
    
    /**
     * Show the dialog
     */
    void show();
    
    /**
     * Render the dialog. Call every frame.
     * @return Result of user action
     */
    Result render();
    
    /**
     * Get the configured import options after Confirmed result
     */
    const ImportOptions& getOptions() const { return options_; }
    
    /**
     * Check if dialog is open
     */
    bool isOpen() const { return is_open_; }
    
    /**
     * Set callback for browsing files
     */
    void setBrowseCallback(BrowseCallback cb) { on_browse_ = std::move(cb); }
    
private:
    bool is_open_ = false;
    bool should_open_ = false;
    ImportOptions options_;
    char path_buffer_[512] = {};
    BrowseCallback on_browse_;
};

} // namespace UI
} // namespace MapEditor
