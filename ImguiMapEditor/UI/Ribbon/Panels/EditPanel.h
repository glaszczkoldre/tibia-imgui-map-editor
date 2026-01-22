#pragma once
#include "../Interfaces/IRibbonPanel.h"

namespace MapEditor {
namespace AppLogic {
class MapTabManager;
}
namespace UI {
namespace Ribbon {

/**
 * Edit operations panel for the ribbon.
 * Provides Undo, Redo, Cut, Copy, Paste, and Delete buttons.
 */
class EditPanel : public IRibbonPanel {
public:
    explicit EditPanel(AppLogic::MapTabManager* tab_manager);
    ~EditPanel() override = default;
    
    // IRibbonPanel interface
    const char* GetPanelName() const override { return "Edit"; }
    const char* GetPanelID() const override { return "Edit###RibbonEdit"; }
    void Render() override;

private:
    AppLogic::MapTabManager* tab_manager_;
};

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor
