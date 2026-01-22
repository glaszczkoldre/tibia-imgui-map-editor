#include "BrushesPanel.h"
#include "Brushes/BrushController.h"
#include "IconsFontAwesome6.h"
#include "Services/BrushSettingsService.h"
#include "UI/Ribbon/Utils/RibbonUtils.h"
#include <imgui.h>

namespace MapEditor {
namespace UI {
namespace Ribbon {

BrushesPanel::BrushesPanel(Brushes::BrushController *controller,
                           Services::BrushSettingsService *settingsService)
    : controller_(controller), settingsService_(settingsService) {}

void BrushesPanel::Render() {
  // Ground Brush
  Utils::RenderToggleButton(
      ICON_FA_PAINTBRUSH, selected_brush_ == 0, "Paint ground tiles (G)",
      [this]() { selected_brush_ = 0; }, " Ground");

  ImGui::SameLine();

  // Raw Brush
  Utils::RenderToggleButton(
      ICON_FA_PEN, selected_brush_ == 1, "Paint raw items (walls, objects) (R)",
      [this]() { selected_brush_ = 1; }, " Raw");

  ImGui::SameLine();

  // Spawn Brush
  Utils::RenderToggleButton(
      ICON_FA_LOCATION_DOT, selected_brush_ == 2, "Place spawn points (S)",
      [this]() {
        selected_brush_ = 2;
        if (controller_) {
          controller_->activateSpawnBrush();
        }
      },
      " Spawn");

  ImGui::SameLine();

  // Spawn settings (only show when spawn brush is selected)
  if (selected_brush_ == 2 && settingsService_) {
    // Auto-create spawn checkbox
    bool autoSpawn = settingsService_->getAutoCreateSpawn();
    if (ImGui::Checkbox("##AutoSpawn", &autoSpawn)) {
      settingsService_->setAutoCreateSpawn(autoSpawn);
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Auto-create spawn when placing creatures");
    ImGui::SameLine();

    // Spawn radius
    ImGui::Text(ICON_FA_CIRCLE_NOTCH);
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Spawn radius");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    int radius = settingsService_->getDefaultSpawnRadius();
    if (ImGui::SliderInt("##SpawnRadius", &radius, 1, 10, "%d")) {
      settingsService_->setDefaultSpawnRadius(radius);
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Spawn radius: %d tiles", radius);
    ImGui::SameLine();

    // Spawn timer
    ImGui::Text(ICON_FA_CLOCK);
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Spawn timer (seconds)");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    int time = settingsService_->getDefaultSpawnTime();
    if (ImGui::InputInt("##SpawnTime", &time, 0, 0)) {
      settingsService_->setDefaultSpawnTime(std::clamp(time, 1, 86400));
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Spawn timer: %d seconds", time);
    ImGui::SameLine();
  }

  // Zone Flag Brushes
  Utils::RenderToggleButton(
      ICON_FA_SHIELD_HALVED, selected_brush_ == 3, "Protection Zone flag (PZ)",
      [this]() {
        selected_brush_ = 3;
        if (controller_)
          controller_->activatePZBrush();
      },
      " PZ");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_HAND, selected_brush_ == 4, "No PvP Zone flag",
      [this]() {
        selected_brush_ = 4;
        if (controller_)
          controller_->activateNoPvpBrush();
      },
      " NoPvP");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_DOOR_CLOSED, selected_brush_ == 5, "No Logout Zone flag",
      [this]() {
        selected_brush_ = 5;
        if (controller_)
          controller_->activateNoLogoutBrush();
      },
      " NoLog");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_SKULL, selected_brush_ == 6, "PvP Zone flag",
      [this]() {
        selected_brush_ = 6;
        if (controller_)
          controller_->activatePvpZoneBrush();
      },
      " PvP");
  ImGui::SameLine();

  // Eraser Brush
  Utils::RenderToggleButton(
      ICON_FA_ERASER, selected_brush_ == 7,
      "Eraser - remove items from tiles (E)",
      [this]() {
        selected_brush_ = 7;
        if (controller_)
          controller_->activateEraserBrush();
      },
      " Eraser");
  ImGui::SameLine();

  // House Brush
  Utils::RenderToggleButton(
      ICON_FA_HOUSE, selected_brush_ == 8, "House - assign tiles to houses (H)",
      [this]() {
        selected_brush_ = 8;
        if (controller_)
          controller_->activateHouseBrush();
      },
      " House");
  ImGui::SameLine();

  // Waypoint Brush
  Utils::RenderToggleButton(
      ICON_FA_LOCATION_PIN, selected_brush_ == 9,
      "Waypoint - place navigation waypoints (W)",
      [this]() {
        selected_brush_ = 9;
        if (controller_)
          controller_->activateWaypointBrush();
      },
      " Waypoint");
  ImGui::SameLine();

  Utils::RenderSeparator();
  ImGui::SameLine();

  // Shape and size controls
  renderShapeControls();
  ImGui::SameLine();
  Utils::RenderSeparator();
  ImGui::SameLine();
  renderSizeControls();
}

void BrushesPanel::renderShapeControls() {
  if (!settingsService_) {
    ImGui::TextDisabled(ICON_FA_SHAPES " Shape: N/A");
    return;
  }

  ImGui::Text(ICON_FA_SHAPES);
  ImGui::SameLine();

  auto currentType = settingsService_->getBrushType();

  // Square toggle
  bool isSquare = (currentType == Services::BrushType::Square);
  Utils::RenderToggleButton(
      ICON_FA_VECTOR_SQUARE, isSquare, "Square brush shape",
      [this]() { settingsService_->setBrushType(Services::BrushType::Square); },
      "##Square");

  ImGui::SameLine();

  // Circle toggle
  bool isCircle = (currentType == Services::BrushType::Circle);
  Utils::RenderToggleButton(
      ICON_FA_CIRCLE, isCircle, "Circle brush shape",
      [this]() { settingsService_->setBrushType(Services::BrushType::Circle); },
      "##Circle");

  ImGui::SameLine();

  // Custom toggle (only if custom brushes exist)
  bool isCustom = (currentType == Services::BrushType::Custom);
  Utils::RenderToggleButton(
      ICON_FA_PUZZLE_PIECE, isCustom, "Custom brush shape",
      [this]() { settingsService_->setBrushType(Services::BrushType::Custom); },
      "##Custom");
}

void BrushesPanel::renderSizeControls() {
  if (!settingsService_) {
    ImGui::TextDisabled(ICON_FA_CIRCLE_DOT " Size: N/A");
    return;
  }

  ImGui::Text(ICON_FA_CIRCLE_DOT " Size:");
  ImGui::SameLine();

  int brush_size = settingsService_->getStandardSize();
  int original_size = brush_size;

  // Minus button
  if (ImGui::Button(ICON_FA_MINUS "##BrushMinus")) {
    if (brush_size > 1) {
      brush_size--;
    }
  }
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Decrease brush size (-)");

  ImGui::SameLine();

  // Visual size indicator in slider
  ImGui::SetNextItemWidth(80.0f);
  ImGui::SliderInt("##BrushSize", &brush_size, 1, 10, "%d");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Brush Radius: %d tiles (%dx%d)", brush_size,
                      brush_size * 2 + 1, brush_size * 2 + 1);
  }

  ImGui::SameLine();

  // Plus button
  if (ImGui::Button(ICON_FA_PLUS "##BrushPlus")) {
    if (brush_size < 10) {
      brush_size++;
    }
  }
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Increase brush size (+)");

  // Update service if changed
  if (brush_size != original_size) {
    settingsService_->setStandardSize(brush_size);
  }
}

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor
