#include "ViewPanel.h"
#include "Core/Config.h"
#include "IconsFontAwesome6.h"
#include "Services/ViewSettings.h"
#include "UI/Map/MapPanel.h"
#include "UI/Ribbon/Utils/RibbonUtils.h"
#include <imgui.h>
#include <format>

namespace MapEditor {
namespace UI {
namespace Ribbon {

ViewPanel::ViewPanel(Services::ViewSettings &view_settings, MapPanel *map_panel)
    : view_settings_(view_settings), map_panel_(map_panel) {}

void ViewPanel::Render() {
  // Zoom controls
  Utils::RenderButton(ICON_FA_MAGNIFYING_GLASS_PLUS, nullptr, true,
                      "Zoom In (Ctrl++)", nullptr,
                      [this]() { view_settings_.zoomIn(); });

  ImGui::SameLine();

  Utils::RenderButton(ICON_FA_MAGNIFYING_GLASS_MINUS, nullptr, true,
                      "Zoom Out (Ctrl+-)", nullptr,
                      [this]() { view_settings_.zoomOut(); });

  ImGui::SameLine();

  Utils::RenderButton(ICON_FA_MAGNIFYING_GLASS, nullptr, true,
                      "Reset Zoom (Ctrl+0)", nullptr,
                      [this]() { view_settings_.zoomReset(); });

  ImGui::SameLine();

  std::string home_tooltip = std::format("Reset Camera to Center ({:.0f}, {:.0f}, {}) (Home)",
        Config::Camera::DEFAULT_CENTER_X, Config::Camera::DEFAULT_CENTER_Y, Config::Camera::DEFAULT_CENTER_Z);

  Utils::RenderButton(ICON_FA_LOCATION_CROSSHAIRS, nullptr, true,
                      home_tooltip.c_str(), nullptr, [this]() {
                        if (map_panel_) {
                          map_panel_->setCameraCenter(
                              Config::Camera::DEFAULT_CENTER_X,
                              Config::Camera::DEFAULT_CENTER_Y,
                              Config::Camera::DEFAULT_CENTER_Z);
                        }
                      });

  ImGui::SameLine();
  Utils::RenderSeparator();
  ImGui::SameLine();

  // Floor controls
  // We must use map_panel to change floor, as ViewSettings is just a data sink
  // for MapPanel
  Utils::RenderButton(ICON_FA_ARROW_UP, nullptr, true, "Floor Up (PgUp)",
                      nullptr, [this]() {
                        if (map_panel_)
                          map_panel_->floorUp();
                      });

  ImGui::SameLine();

  Utils::RenderButton(ICON_FA_ARROW_DOWN, nullptr, true, "Floor Down (PgDn)",
                      nullptr, [this]() {
                        if (map_panel_)
                          map_panel_->floorDown();
                      });

  ImGui::SameLine();
  Utils::RenderSeparator();
  ImGui::SameLine();

  // Toggle buttons with visual feedback and keyboard shortcuts
  Utils::RenderToggleButton(
      ICON_FA_BORDER_ALL, view_settings_.show_grid, "Show Grid (Shift+G)",
      [this]() { view_settings_.show_grid = !view_settings_.show_grid; });

  ImGui::SameLine();

  Utils::RenderToggleButton(ICON_FA_LAYER_GROUP, view_settings_.show_all_floors,
                            "View all visible floors (Ctrl+W)", [this]() {
                              view_settings_.show_all_floors =
                                  !view_settings_.show_all_floors;
                            });

  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_GHOST, view_settings_.ghost_items,
      "Show items on other floors as semi-transparent (G)",
      [this]() { view_settings_.ghost_items = !view_settings_.ghost_items; });

  ImGui::SameLine();
  Utils::RenderSeparator(); // Visuals Separator
  ImGui::SameLine();

  Utils::RenderToggleButton(ICON_FA_SUN, view_settings_.show_shade,
                            "Render lighting/shade layers (Q)", [this]() {
                              view_settings_.show_shade =
                                  !view_settings_.show_shade;
                            });

  ImGui::SameLine();

  Utils::RenderToggleButton(ICON_FA_EYE, view_settings_.show_creatures,
                            "Show Creatures (F)", [this]() {
                              view_settings_.show_creatures =
                                  !view_settings_.show_creatures;
                            });

  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_CROSSHAIRS, view_settings_.show_spawns, "Show Spawns (S)",
      [this]() { view_settings_.show_spawns = !view_settings_.show_spawns; });

  ImGui::SameLine();
  Utils::RenderSeparator(); // Simulation Separator
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_PERSON_WALKING, view_settings_.simulate_creatures,
      "Simulate Creatures (Enable random movement animation)", [this]() {
        view_settings_.simulate_creatures = !view_settings_.simulate_creatures;
      });

  ImGui::SameLine();
  Utils::RenderSeparator();
  ImGui::SameLine();

  // Overlay Toggles
  Utils::RenderToggleButton(ICON_FA_BAN, view_settings_.show_blocking,
                            "Show Blocking Tiles (O)", [this]() {
                              view_settings_.show_blocking =
                                  !view_settings_.show_blocking;
                            });

  ImGui::SameLine();

  Utils::RenderToggleButton(ICON_FA_STAR, view_settings_.show_special_tiles,
                            "Show Special Tiles (PZ, PVP) (E)", [this]() {
                              view_settings_.show_special_tiles =
                                  !view_settings_.show_special_tiles;
                            });

  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_HOUSE, view_settings_.show_houses, "Show House Tiles (Ctrl+H)",
      [this]() { view_settings_.show_houses = !view_settings_.show_houses; });

  ImGui::SameLine();

  Utils::RenderToggleButton(ICON_FA_HIGHLIGHTER, view_settings_.highlight_items,
                            "Highlight Items (V)", [this]() {
                              view_settings_.highlight_items =
                                  !view_settings_.highlight_items;
                            });

  ImGui::SameLine();

  Utils::RenderToggleButton(ICON_FA_LOCK, view_settings_.highlight_locked_doors,
                            "Highlight Locked Doors (U)", [this]() {
                              view_settings_.highlight_locked_doors =
                                  !view_settings_.highlight_locked_doors;
                            });

  ImGui::SameLine();
  Utils::RenderSeparator();
}

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor
