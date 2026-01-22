#pragma once
#include "../Interfaces/IRibbonPanel.h"
#include "Domain/SelectionSettings.h"
namespace MapEditor {
namespace AppLogic {
class MapTabManager;
}
namespace UI {
namespace Ribbon {

/**
 * Selection controls panel for the ribbon.
 * Provides selection mode options and selection actions.
 */
class SelectionPanel : public IRibbonPanel {
public:
    SelectionPanel(Domain::SelectionSettings& selection_settings, 
                   AppLogic::MapTabManager* tab_manager);
    ~SelectionPanel() override = default;
    
    // IRibbonPanel interface
    const char* GetPanelName() const override { return "Selection"; }
    const char* GetPanelID() const override { return "Selection###RibbonSelection"; }
    void Render() override;

private:
    Domain::SelectionSettings& selection_settings_;
    AppLogic::MapTabManager* tab_manager_;
};

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor
