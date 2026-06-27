#include "BrushesPanel.h"
#include "Brushes/BrushController.h"
#include "IconsFontAwesome6.h"
#include "UI/Ribbon/Utils/RibbonUtils.h"
#include <algorithm>
#include <cstdio>
#include <format>
#include <imgui.h>
#include <string>
#include <string_view>

namespace MapEditor {
namespace UI {
namespace Ribbon {

namespace {

[[nodiscard]] int resolveSelectedBrushId(
    Brushes::BrushController *controller) {
  if (!controller) {
    return -1;
  }

  if (controller->isCurrentBrush(controller->getSpawnBrush())) {
    return 2;
  }
  if (controller->isCurrentBrush(controller->getPZBrush())) {
    return 3;
  }
  if (controller->isCurrentBrush(controller->getNoPvpBrush())) {
    return 4;
  }
  if (controller->isCurrentBrush(controller->getNoLogoutBrush())) {
    return 5;
  }
  if (controller->isCurrentBrush(controller->getPvpZoneBrush())) {
    return 6;
  }
  if (controller->isCurrentBrush(controller->getEraserBrush())) {
    return 7;
  }
  if (controller->isCurrentBrush(controller->getHouseBrush())) {
    return 8;
  }
  if (controller->isCurrentBrush(controller->getWaypointBrush())) {
    return 9;
  }
  if (controller->isCurrentBrush(controller->getHouseExitBrush())) {
    return 10;
  }
  if (controller->isCurrentBrush(controller->getOptionalBorderBrush())) {
    return 11;
  }
  if (controller->isCurrentBrush(controller->getNormalDoorBrush())) {
    return 12;
  }
  if (controller->isCurrentBrush(controller->getLockedDoorBrush())) {
    return 13;
  }
  if (controller->isCurrentBrush(controller->getQuestDoorBrush())) {
    return 14;
  }
  if (controller->isCurrentBrush(controller->getMagicDoorBrush())) {
    return 15;
  }
  if (controller->isCurrentBrush(controller->getArchwayBrush())) {
    return 16;
  }
  if (controller->isCurrentBrush(controller->getWindowBrush()) ||
      controller->isCurrentBrush(controller->getHatchWindowBrush())) {
    return 17;
  }
  if (controller->isCurrentBrush(controller->getNormalAltDoorBrush())) {
    return 18;
  }

  const auto *currentBrush = controller->getCurrentBrush();
  if (!currentBrush) {
    return -1;
  }

  switch (currentBrush->getType()) {
  case Brushes::BrushType::Ground:
    return 0;
  case Brushes::BrushType::Raw:
    return 1;
  default:
    return -1;
  }
}

} // namespace

BrushesPanel::BrushesPanel(Brushes::BrushController *controller)
    : controller_(controller) {}

void BrushesPanel::Render() {
  selected_brush_ = resolveSelectedBrushId(controller_);

  Utils::RenderToggleButton(
      ICON_FA_PAINTBRUSH, selected_brush_ == 0, "Paint ground tiles (G)",
      [this]() { selected_brush_ = 0; }, " Ground");

  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_PEN, selected_brush_ == 1, "Paint raw items (walls, objects) (R)",
      [this]() { selected_brush_ = 1; }, " Raw");

  ImGui::SameLine();

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

  Utils::RenderToggleButton(
      ICON_FA_SHIELD_HALVED, selected_brush_ == 3, "Protection Zone flag (PZ)",
      [this]() {
        selected_brush_ = 3;
        if (controller_) {
          controller_->activatePZBrush();
        }
      },
      " PZ");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_HAND, selected_brush_ == 4, "No PvP Zone flag",
      [this]() {
        selected_brush_ = 4;
        if (controller_) {
          controller_->activateNoPvpBrush();
        }
      },
      " NoPvP");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_DOOR_CLOSED, selected_brush_ == 5, "No Logout Zone flag",
      [this]() {
        selected_brush_ = 5;
        if (controller_) {
          controller_->activateNoLogoutBrush();
        }
      },
      " NoLog");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_SKULL, selected_brush_ == 6, "PvP Zone flag",
      [this]() {
        selected_brush_ = 6;
        if (controller_) {
          controller_->activatePvpZoneBrush();
        }
      },
      " PvP");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_ERASER, selected_brush_ == 7,
      "Eraser - remove items from tiles (E)",
      [this]() {
        selected_brush_ = 7;
        if (controller_) {
          controller_->activateEraserBrush();
        }
      },
      " Eraser");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_HOUSE, selected_brush_ == 8, "House - assign tiles to houses (H)",
      [this]() {
        selected_brush_ = 8;
        if (controller_) {
          controller_->activateHouseBrush();
        }
      },
      " House");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_LOCATION_PIN, selected_brush_ == 9,
      "Waypoint - place navigation waypoints (W)",
      [this]() {
        selected_brush_ = 9;
        if (controller_) {
          controller_->activateWaypointBrush();
        }
      },
      " Waypoint");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_RIGHT_FROM_BRACKET, selected_brush_ == 10,
      "House exit - set the selected house entrance tile",
      [this]() {
        selected_brush_ = 10;
        if (controller_) {
          controller_->activateHouseExitBrush();
        }
      },
      " Exit");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_BORDER_ALL, selected_brush_ == 11,
      "Optional border tool for grounds with optional overlays",
      [this]() {
        selected_brush_ = 11;
        if (controller_) {
          controller_->activateOptionalBorderBrush();
        }
      },
      " Opt");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_DOOR_OPEN, selected_brush_ == 12, "Normal door brush",
      [this]() {
        selected_brush_ = 12;
        if (controller_) {
          controller_->activateNormalDoorBrush();
        }
      },
      " Door");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_DOOR_OPEN, selected_brush_ == 18, "Normal Alt door brush",
      [this]() {
        selected_brush_ = 18;
        if (controller_) {
          controller_->activateNormalAltDoorBrush();
        }
      },
      " Alt");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_KEY, selected_brush_ == 13, "Locked door brush",
      [this]() {
        selected_brush_ = 13;
        if (controller_) {
          controller_->activateLockedDoorBrush();
        }
      },
      " Locked");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_SCROLL, selected_brush_ == 14, "Quest door brush",
      [this]() {
        selected_brush_ = 14;
        if (controller_) {
          controller_->activateQuestDoorBrush();
        }
      },
      " Quest");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_WAND_MAGIC_SPARKLES, selected_brush_ == 15, "Magic door brush",
      [this]() {
        selected_brush_ = 15;
        if (controller_) {
          controller_->activateMagicDoorBrush();
        }
      },
      " Magic");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_ARCHWAY, selected_brush_ == 16, "Archway brush",
      [this]() {
        selected_brush_ = 16;
        if (controller_) {
          controller_->activateArchwayBrush();
        }
      },
      " Arch");
  ImGui::SameLine();

  Utils::RenderToggleButton(
      ICON_FA_WINDOW_MAXIMIZE, selected_brush_ == 17, "Window brush",
      [this]() {
        selected_brush_ = 17;
        if (controller_) {
          controller_->activateWindowBrush();
        }
      },
      " Window");
}

} // namespace Ribbon
} // namespace UI
} // namespace MapEditor
