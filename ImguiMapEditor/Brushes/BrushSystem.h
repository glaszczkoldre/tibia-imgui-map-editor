#pragma once
#include "BrushController.h"
#include "BrushRegistry.h"
#include "Services/BrushSettingsService.h"
#include "Services/Preview/BrushPreviewFactory.h"
#include "Services/TilesetService.h"

#include <memory>

namespace MapEditor::UI {
class TilesetWidget;
namespace Panels {
class BrushSizePanel;
} // namespace Panels
} // namespace MapEditor::UI

namespace MapEditor {

namespace Services {
class ClientDataService;
class SpriteManager;
class ConfigService;
} // namespace Services

namespace Brushes {

/**
 * Manages the Brush system components: Registry, Controller, Settings, and UI.
 * Encapsulates the wiring and lifecycle of brush-related components.
 *
 * Also owns TilesetService since it depends on BrushRegistry for brush lookups.
 */
class BrushSystem {
public:
  BrushSystem();
  ~BrushSystem();

  // Accessors
  BrushRegistry &getRegistry() { return registry_; }
  BrushController &getController() { return controller_; }
  UI::TilesetWidget &getTilesetWidget() { return *tileset_widget_; }
  Services::BrushSettingsService &getSettingsService() {
    return settings_service_;
  }
  UI::Panels::BrushSizePanel &getBrushSizePanel() { return *brush_size_panel_; }
  Services::TilesetService &getTilesetService() { return tileset_service_; }

  // Persistence wiring
  void setConfigService(Services::ConfigService *configService);
  void saveBrushes(); // Save custom brushes to JSON
  void saveSettings();

  const BrushRegistry &getRegistry() const { return registry_; }
  const BrushController &getController() const { return controller_; }
  const UI::TilesetWidget &getTilesetWidget() const { return *tileset_widget_; }
  const Services::BrushSettingsService &getSettingsService() const {
    return settings_service_;
  }
  const UI::Panels::BrushSizePanel &getBrushSizePanel() const {
    return *brush_size_panel_;
  }
  const Services::TilesetService &getTilesetService() const {
    return tileset_service_;
  }

private:
  BrushRegistry registry_;
  Services::TilesetService
      tileset_service_; // Owns registries, depends on registry_
  Services::BrushSettingsService
      settings_service_; // Must be before controller/panel
  Services::Preview::BrushPreviewFactory preview_factory_; // Preview factory
  BrushController controller_;
  std::unique_ptr<UI::TilesetWidget> tileset_widget_;
  std::unique_ptr<UI::Panels::BrushSizePanel> brush_size_panel_;
  std::string brushPath_; // Path to custom_brushes.json
  Services::ConfigService *config_service_ = nullptr;
};

} // namespace Brushes
} // namespace MapEditor
