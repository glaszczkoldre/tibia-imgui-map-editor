#pragma once
#include "Domain/ChunkedMap.h"
#include <string>

namespace MapEditor {
namespace UI {

/**
 * Dialog for editing map properties/metadata.
 * 
 * Editable properties:
 * - Description (multi-line text)
 * - Width and Height
 * - External house/spawn file references
 * 
 * Note: Version conversion is deferred to a future release.
 * The dialog displays version info but doesn't allow changes yet.
 */
class MapPropertiesDialog {
public:
    enum class Result {
        None,       // Dialog still open
        Applied,    // User clicked OK - changes applied
        Cancelled   // User cancelled - no changes
    };
    
    /**
     * Show the dialog with the given map.
     */
    void show(Domain::ChunkedMap* map);
    
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
    void loadFromMap();
    void applyToMap();
    
    bool should_open_ = false;
    bool is_open_ = false;
    Domain::ChunkedMap* map_ = nullptr;
    
    // Buffers for editing
    char description_buffer_[4096] = {};
    int width_ = 2048;
    int height_ = 2048;
    char house_filename_[256] = {};
    char spawn_filename_[256] = {};
    
    // Read-only version display
    uint32_t otbm_version_ = 0;
    uint32_t client_version_ = 0;
};

} // namespace UI
} // namespace MapEditor
