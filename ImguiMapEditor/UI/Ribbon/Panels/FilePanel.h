#pragma once
#include "../Interfaces/IRibbonPanel.h"
#include <functional>

namespace MapEditor {
namespace UI {
namespace Ribbon {

/**
 * File operations panel for the ribbon.
 * Provides quick access to New, Open, and Save operations.
 */
class FilePanel : public IRibbonPanel {
public:
    using ActionCallback = std::function<void()>;
    
    FilePanel() = default;
    ~FilePanel() override = default;
    
    // IRibbonPanel interface
    const char* GetPanelName() const override { return "File"; }
    const char* GetPanelID() const override { return "File###RibbonFile"; }
    void Render() override;
    
    // Callbacks for actions
    void SetNewMapCallback(ActionCallback cb) { on_new_map_ = std::move(cb); }
    void SetOpenMapCallback(ActionCallback cb) { on_open_map_ = std::move(cb); }
    void SetSaveMapCallback(ActionCallback cb) { on_save_map_ = std::move(cb); }
    void SetSaveAsMapCallback(ActionCallback cb) { on_save_as_map_ = std::move(cb); }
    void SetCloseMapCallback(ActionCallback cb) { on_close_map_ = std::move(cb); }
    
    // Enable/disable save button based on session state
    void SetHasActiveSession(bool has_session) { has_active_session_ = has_session; }

    // Callback to check modified state
    using CheckModifiedCallback = std::function<bool()>;
    void SetCheckModifiedCallback(CheckModifiedCallback cb) { on_check_modified_ = std::move(cb); }

    // Callback to check loading state
    using CheckLoadingCallback = std::function<bool()>;
    void SetCheckLoadingCallback(CheckLoadingCallback cb) { on_check_loading_ = std::move(cb); }

private:
    ActionCallback on_new_map_;
    ActionCallback on_open_map_;
    ActionCallback on_save_map_;
    ActionCallback on_save_as_map_;
    ActionCallback on_close_map_;
    CheckModifiedCallback on_check_modified_;
    CheckLoadingCallback on_check_loading_;
    bool has_active_session_ = false;
};

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor
