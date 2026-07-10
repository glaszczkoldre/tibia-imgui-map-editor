#pragma once

#include <memory>

// Forward declarations
namespace MapEditor {
namespace AppLogic {
class HotkeyController;
class MapInputController;
class SimulationController;
class ClipboardService;
class SearchController;
class MapOperationHandler;
class StartupController;
} // namespace AppLogic
namespace UI {
class StartupDialog;
class MapPanel;
class IngameBoxWindow;
class MinimapWindow;
class BrowseTileWindow;
class PaletteWindowManager;
class TilesetWidget;
class IconTextureCache;
namespace Panels {
class BrushSettingsPanel;
} // namespace Panels
namespace Ribbon {
class RibbonController;
class FilePanel;
class ThemePanel;
} // namespace Ribbon
} // namespace UI
namespace Presentation {
class MenuBar;
class MainWindow;
class WorkspaceController;
} // namespace Presentation
} // namespace MapEditor

namespace MapEditor {

/**
 * Container for UI components and controllers.
 * Extracted from Application to group UI ownership.
 */
struct UIComponentContainer {
  std::unique_ptr<UI::StartupDialog> startup_dialog;
  std::unique_ptr<UI::MapPanel> map_panel;
  std::unique_ptr<AppLogic::HotkeyController> hotkey_controller;
  std::unique_ptr<Presentation::MenuBar> menu_bar;
  std::unique_ptr<AppLogic::MapInputController> input_controller;
  std::unique_ptr<AppLogic::SimulationController> simulation_controller;
  std::unique_ptr<AppLogic::ClipboardService> clipboard_service;
  std::unique_ptr<AppLogic::MapOperationHandler> map_operations;
  std::unique_ptr<AppLogic::StartupController> startup_controller;
  std::unique_ptr<UI::Ribbon::RibbonController> ribbon_controller;
  std::unique_ptr<AppLogic::SearchController> search_controller;
  std::unique_ptr<Presentation::MainWindow> main_window;
  std::unique_ptr<Presentation::WorkspaceController> workspace_controller;

  // Phase 7 Refactor: Windows moved from Application
  std::unique_ptr<UI::IngameBoxWindow> ingame_box_window;
  std::unique_ptr<UI::MinimapWindow> minimap_window;
  std::unique_ptr<UI::BrowseTileWindow> browse_tile_window;
  std::unique_ptr<UI::PaletteWindowManager> palette_window_manager;
  std::unique_ptr<UI::TilesetWidget> tileset_widget;
  std::unique_ptr<UI::Panels::BrushSettingsPanel> brush_settings_panel;
  std::unique_ptr<UI::IconTextureCache> icon_texture_cache;

  // Side pointers (non-owning) for wiring
  UI::Ribbon::FilePanel *file_panel_ptr = nullptr;
  UI::Ribbon::ThemePanel *theme_panel_ptr = nullptr;
};

} // namespace MapEditor
