#pragma once
#include "../Interfaces/IRibbonPanel.h"

namespace MapEditor {
namespace Services {
class ViewSettings;
}
namespace UI {
class MapPanel;
namespace Ribbon {

/**
 * View controls panel for the ribbon.
 * Provides zoom controls and display toggles.
 */
class ViewPanel : public IRibbonPanel {
public:
    ViewPanel(Services::ViewSettings& view_settings, MapPanel* map_panel);
    ~ViewPanel() override = default;
    
    // IRibbonPanel interface
    const char* GetPanelName() const override { return "View"; }
    const char* GetPanelID() const override { return "View###RibbonView"; }
    void Render() override;

private:
    Services::ViewSettings& view_settings_;
    MapPanel* map_panel_;
};

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor
