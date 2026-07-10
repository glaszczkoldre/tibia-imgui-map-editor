#pragma once

#include "Application/UIComponentContainer.h"
#include <memory>

// Forward declarations
namespace MapEditor {
class AppStateManager;
namespace Services {
class ViewSettings;
class AppSettings;
class HotkeyRegistry;
class ConfigService;
class ClientVersionRegistry;
class OtbmSettings;
class RecentLocationsService;
class TilesetService;
} // namespace Services
namespace Domain {
struct SelectionSettings;
}
namespace AppLogic {
class MapTabManager;
} // namespace AppLogic
namespace Brushes {
class BrushController;
class BrushRegistry;
class BrushSystem;
} // namespace Brushes
} // namespace MapEditor

namespace MapEditor {

struct UIFactoryContext {
  Services::ViewSettings &view_settings;
  Domain::SelectionSettings &selection_settings;
  Services::HotkeyRegistry &hotkey_registry;
  Services::AppSettings &app_settings;
  Services::ConfigService &config;
  Services::ClientVersionRegistry &version_registry;
  Services::RecentLocationsService &recent_locations;
  Services::OtbmSettings &otbm_settings;
  AppLogic::MapTabManager &tab_manager;
  AppStateManager &state_manager;
  Brushes::BrushSystem &brush_system;
  Brushes::BrushController &brush_controller;
  Brushes::BrushRegistry &brush_registry;
  Services::TilesetService &tileset_service; // For palette/tileset UI access
};

/**
 * Factory for creating UI components and controllers.
 * Extracted from Application to separate construction logic.
 */
class UIFactory {
public:
  static UIComponentContainer create(const UIFactoryContext &ctx);
};

} // namespace MapEditor
