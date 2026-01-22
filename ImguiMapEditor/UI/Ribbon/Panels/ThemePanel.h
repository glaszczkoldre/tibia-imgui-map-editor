#pragma once
#include "../Interfaces/IRibbonPanel.h"
#include "UI/Core/Theme.h"

namespace MapEditor {
namespace UI {
namespace Ribbon {

/**
 * Theme selection panel for the ribbon.
 * Provides quick access to available UI themes.
 */
class ThemePanel : public IRibbonPanel {
public:
    ThemePanel() = default;
    ~ThemePanel() override = default;
    
    // IRibbonPanel interface
    const char* GetPanelName() const override { return "Theme"; }
    const char* GetPanelID() const override { return "Theme###RibbonTheme"; }
    void Render() override;
    
    // Link to persistent theme setting
    void setThemePtr(ThemeType* theme_ptr) { current_theme_ = theme_ptr; }
    
private:
    ThemeType* current_theme_ = nullptr;
};

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor

