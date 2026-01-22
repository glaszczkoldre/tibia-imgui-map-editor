#pragma once
#include "BrushController.h"
#include "BrushRegistry.h"
#include "Services/BrushSettingsService.h"
#include "Services/Preview/BrushPreviewFactory.h"
#include "Services/TilesetService.h"
#include "UI/Panels/BrushSizePanel.h"
#include "UI/Widgets/TilesetWidget.h"
#include <memory>

namespace MapEditor {

namespace Services {
class ClientDataService;
class SpriteManager;
class ConfigService;

namespace Brushes {
// Lookup services for auto-alignment
class BorderLookupService;
class WallLookupService;
class TableLookupService;
class CarpetLookupService;
} // namespace Brushes
} // namespace Services

namespace AppLogic {
class EditorSession;
}

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
  UI::TilesetWidget &getTilesetWidget() { return tileset_widget_; }
  Services::BrushSettingsService &getSettingsService() {
    return settings_service_;
  }
  UI::Panels::BrushSizePanel &getBrushSizePanel() { return brush_size_panel_; }
  Services::TilesetService &getTilesetService() { return tileset_service_; }

  // Persistence wiring
  void setConfigService(Services::ConfigService *configService);
  void saveBrushes(); // Save custom brushes to JSON
  std::string getBrushSavePath() const { return brushPath_; }

  const BrushRegistry &getRegistry() const { return registry_; }
  const BrushController &getController() const { return controller_; }
  const UI::TilesetWidget &getTilesetWidget() const { return tileset_widget_; }
  const Services::BrushSettingsService &getSettingsService() const {
    return settings_service_;
  }
  const UI::Panels::BrushSizePanel &getBrushSizePanel() const {
    return brush_size_panel_;
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
  UI::TilesetWidget tileset_widget_;
  UI::Panels::BrushSizePanel brush_size_panel_;
  std::string brushPath_; // Path to custom_brushes.json
};

} // namespace Brushes
} // namespace MapEditor
