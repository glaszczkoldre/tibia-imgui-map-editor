#include "BrushSettingsPanel.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>

#include "IconsFontAwesome6.h"
#include "Brushes/BrushController.h"
#include "Brushes/BrushRegistry.h"
#include "Services/BrushSettingsService.h"
#include "UI/Utils/IconTextureCache.h"
#include "UI/Helpers/BrushToolOptionsModel.h"

namespace MapEditor::UI::Panels {

BrushSettingsPanel::BrushSettingsPanel(Services::BrushSettingsService *brushService,
                                       Brushes::BrushController *brushController,
                                       Brushes::BrushRegistry *brushRegistry,
                                       UI::IconTextureCache *iconCache)
    : service_(brushService), controller_(brushController),
      registry_(brushRegistry), iconCache_(iconCache) {}

void BrushSettingsPanel::render(bool *p_visible) {
  if (p_visible && !*p_visible) {
    return;
  }

  // Stable ImGui ID is BrushSettings, displayed title is "Brush settings"
  if (ImGui::Begin("Brush settings###BrushSettings", p_visible)) {
    renderMainTools();
    ImGui::Separator();
    renderSize();
    ImGui::Separator();
    renderOther();
  }
  ImGui::End();
}

void BrushSettingsPanel::renderMainTools() {
  if (ImGui::CollapsingHeader("Main tools", ImGuiTreeNodeFlags_DefaultOpen)) {
    const auto *activeBrush = controller_->getCurrentBrush();
    const bool creatureMode = BrushToolOptionsModel::isCreatureToolMode(activeBrush);
    const float buttonSize = 34.0f;
    const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
    const float colWidth = buttonSize + itemSpacing;
    const float windowWidth = ImGui::GetContentRegionAvail().x;

    // Push tight frame padding and rounding for a sleek look
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

    if (creatureMode) {
      // 2-column grid for place_creature and place_spawn stretching to fill the width
      if (ImGui::BeginTable("CreatureToolsGrid", 2, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);

        // PLACE button
        ImGui::TableNextColumn();
        float cellWidth = ImGui::GetContentRegionAvail().x;
        if (cellWidth > buttonSize) {
          ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (cellWidth - buttonSize) * 0.5f);
        }
        const bool isCreatureSelected = activeBrush && activeBrush->getType() == Brushes::BrushType::Creature;
        const auto creatureIcon = iconCache_->getIcon("place_creature");
        if (isCreatureSelected) {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
        } else {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.08f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.15f));
        }
        ImGui::PushID("place_creature_btn");
        if (ImGui::ImageButton("##creature", creatureIcon.textureId, ImVec2(buttonSize, buttonSize))) {
          selectCreatureTool();
        }
        ImGui::SetItemTooltip("Place Creature");
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        // SPAWN button
        ImGui::TableNextColumn();
        cellWidth = ImGui::GetContentRegionAvail().x;
        if (cellWidth > buttonSize) {
          ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (cellWidth - buttonSize) * 0.5f);
        }
        const bool isSpawnSelected = activeBrush && activeBrush->getType() == Brushes::BrushType::Spawn;
        const auto spawnIcon = iconCache_->getIcon("place_spawn");
        if (isSpawnSelected) {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
        } else {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.08f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.15f));
        }
        ImGui::PushID("place_spawn_btn");
        if (ImGui::ImageButton("##spawn", spawnIcon.textureId, ImVec2(buttonSize, buttonSize))) {
          controller_->activateSpawnBrush();
        }
        ImGui::SetItemTooltip("Place Spawn");
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        ImGui::EndTable();
      }
    } else {
      // Dynamic column grid of 34x34 icon buttons for standard tools
      struct ToolItem {
        std::string name;
        Brushes::IBrush *brush;
        std::string tooltip;
      };

      std::vector<ToolItem> tools = {
          {"optional_border", controller_->getOptionalBorderBrush(), "Optional Border"},
          {"eraser", controller_->getEraserBrush(), "Eraser"},
          {"protection_zone", controller_->getPZBrush(), "Protection Zone"},
          {"no_pvp", controller_->getNoPvpBrush(), "No PvP Zone"},
          {"no_logout", controller_->getNoLogoutBrush(), "No Logout Zone"},
          {"pvp_zone", controller_->getPvpZoneBrush(), "PvP Zone"},
          {"door_normal", controller_->getNormalDoorBrush(), "Normal Door"},
          {"door_locked", controller_->getLockedDoorBrush(), "Locked Door"},
          {"door_magic", controller_->getMagicDoorBrush(), "Magic Door"},
          {"door_quest", controller_->getQuestDoorBrush(), "Quest Door"},
          {"door_normal_alt", controller_->getNormalAltDoorBrush(), "Normal Alt Door"},
          {"window_hatch", controller_->getHatchWindowBrush(), "Hatch Window"},
          {"window_normal", controller_->getWindowBrush(), "Window"},
          {"door_archway", controller_->getArchwayBrush(), "Archway"}};

      int totalTools = 0;
      for (const auto &tool : tools) {
        if (tool.brush) totalTools++;
      }

      int cols = static_cast<int>(std::floor(windowWidth / colWidth));
      if (cols < 1) cols = 1;
      if (cols > totalTools) cols = totalTools;

      if (ImGui::BeginTable("MainToolsGrid", cols, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoKeepColumnsVisible)) {
        for (int i = 0; i < cols; ++i) {
          ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
        }

        for (const auto &tool : tools) {
          if (!tool.brush) {
            continue;
          }

          ImGui::TableNextColumn();
          
          float cellWidth = ImGui::GetContentRegionAvail().x;
          if (cellWidth > buttonSize) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (cellWidth - buttonSize) * 0.5f);
          }

          const bool isSelected = controller_->isCurrentBrush(tool.brush);
          const auto icon = iconCache_->getIcon(tool.name);

          if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
          } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.08f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.15f));
          }

          std::string btnId = "##" + tool.name;
          ImGui::PushID(tool.name.c_str());
          if (ImGui::ImageButton(btnId.c_str(), icon.textureId, ImVec2(buttonSize, buttonSize))) {
            controller_->setBrush(tool.brush);
          }
          ImGui::SetItemTooltip("%s", tool.tooltip.c_str());

          ImGui::PopStyleColor(3);
          ImGui::PopID();
        }
        ImGui::EndTable();
      }
    }

    ImGui::PopStyleVar(2);
  }
}

void BrushSettingsPanel::renderSize() {
  const auto *activeBrush = controller_->getCurrentBrush();
  if (!BrushToolOptionsModel::hasBrushSizeControls(activeBrush)) {
    return;
  }

  if (ImGui::CollapsingHeader("Size", ImGuiTreeNodeFlags_DefaultOpen)) {
    bool exact = service_->isExactBrushSize();
    int sizeX = service_->getBrushSizeX();
    int sizeY = service_->getBrushSizeY();

    // Row X
    ImGui::PushID("exact_toggle");
    if (exact) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
    }
    if (ImGui::Button(ICON_FA_BULLSEYE, ImVec2(26, 26))) {
      service_->setExactBrushSize(!exact);
    }
    if (exact) {
      ImGui::PopStyleColor(3);
    }
    ImGui::SetItemTooltip("Toggle exact size (bullseye)");
    ImGui::PopID();

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("X");
    ImGui::SameLine();

    int minVal = exact ? 1 : 0;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40.0f);
    if (ImGui::SliderInt("##SizeX", &sizeX, minVal, 15)) {
      service_->setBrushSizeX(sizeX);
    }
    ImGui::SameLine();
    ImGui::Text("%d", service_->getEffectiveAxisSpanX());

    // Row Y
    bool aspectLocked = service_->isBrushAspectRatioLocked();
    ImGui::PushID("aspect_toggle");
    if (aspectLocked) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
    }
    if (ImGui::Button(aspectLocked ? ICON_FA_LINK : ICON_FA_LINK_SLASH, ImVec2(26, 26))) {
      service_->setBrushAspectRatioLocked(!aspectLocked);
    }
    if (aspectLocked) {
      ImGui::PopStyleColor(3);
    }
    ImGui::SetItemTooltip("Toggle aspect ratio lock");
    ImGui::PopID();

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Y");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40.0f);
    if (ImGui::SliderInt("##SizeY", &sizeY, minVal, 15)) {
      service_->setBrushSizeY(sizeY);
    }
    ImGui::SameLine();
    ImGui::Text("%d", service_->getEffectiveAxisSpanY());
  }
}

void BrushSettingsPanel::renderOther() {
  const auto *activeBrush = controller_->getCurrentBrush();
  const bool showThickness = BrushToolOptionsModel::hasThicknessControl(activeBrush);
  const bool showSpawn = BrushToolOptionsModel::hasSpawnControls(activeBrush);
  const bool showPreviewBorder = BrushToolOptionsModel::hasPreviewBorderControl(activeBrush);
  const bool showAutoBorder = BrushToolOptionsModel::hasAutoBorderControl(activeBrush);
  const bool showLockDoors = BrushToolOptionsModel::hasLockDoorsControl(activeBrush);

  if (!showThickness && !showSpawn && !showPreviewBorder && !showAutoBorder && !showLockDoors) {
    return;
  }

  if (ImGui::CollapsingHeader("Other", ImGuiTreeNodeFlags_DefaultOpen)) {
    const float alignX = 120.0f;
    const bool hasNumericInputs = showThickness || showSpawn;

    // --- Part 1: Numeric Inputs (Aligned) ---
    if (showThickness) {
      int thicknessPercent = static_cast<int>(std::round(controller_->getBrushThickness() * 100.0f));
      thicknessPercent = std::clamp(thicknessPercent, 1, 100);

      ImGui::PushID("thickness_reset");
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
      if (ImGui::Button(ICON_FA_PAINTBRUSH, ImVec2(26, 26))) {
        controller_->setBrushThickness(1.0f);
      }
      ImGui::PopStyleColor(3);
      ImGui::SetItemTooltip("Reset thickness to 100%");
      ImGui::PopID();

      ImGui::SameLine();
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("T");
      
      ImGui::SameLine();
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40.0f);
      if (ImGui::SliderInt("##Thickness", &thicknessPercent, 1, 100, "%d%%")) {
        controller_->setBrushThickness(static_cast<float>(thicknessPercent) / 100.0f);
      }
      ImGui::SameLine();
      ImGui::Text("%d%%", thicknessPercent);
    }

    if (showSpawn) {
      int spawnTime = service_->getDefaultSpawnTime();
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Spawntime");
      
      ImGui::SameLine();
      ImGui::SetCursorPosX(alignX);
      ImGui::SetNextItemWidth(120.0f);
      if (ImGui::InputInt("##Spawntime", &spawnTime, 10, 60)) {
        service_->setDefaultSpawnTime(std::clamp(spawnTime, 0, 86400));
      }

      int spawnSize = service_->getDefaultSpawnRadius();
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Spawn Size");
      
      ImGui::SameLine();
      ImGui::SetCursorPosX(alignX);
      ImGui::SetNextItemWidth(120.0f);
      if (ImGui::InputInt("##SpawnSize", &spawnSize, 1, 1)) {
        spawnSize = std::clamp(spawnSize, 1, 15);
        service_->setDefaultSpawnRadius(spawnSize);
        service_->setStandardSize(spawnSize);
      }
    }

    // --- Part 2: Separator ---
    const bool hasCheckboxes = showSpawn || showPreviewBorder || showAutoBorder || showLockDoors;
    if (hasNumericInputs && hasCheckboxes) {
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();
    }

    // --- Part 3: Checkboxes ---
    if (showSpawn) {
      bool autoSpawn = service_->getAutoCreateSpawn();
      if (ImGui::Checkbox("Place spawn with creature", &autoSpawn)) {
        service_->setAutoCreateSpawn(autoSpawn);
      }
    }

    if (showAutoBorder) {
      bool autoBorder = service_->getAutoBorder();
      if (ImGui::Checkbox("Automated Bordering (A)", &autoBorder)) {
        service_->setAutoBorder(autoBorder);
      }
    }

    if (showPreviewBorder) {
      bool previewBorder = service_->getPreviewBorder();
      if (ImGui::Checkbox("Preview Border", &previewBorder)) {
        service_->setPreviewBorder(previewBorder);
      }
    }

    if (showLockDoors) {
      bool lockDoors = service_->getLockDoors();
      if (ImGui::Checkbox("Lock Doors (Shift)", &lockDoors)) {
        service_->setLockDoors(lockDoors);
      }
    }
  }
}

void BrushSettingsPanel::selectCreatureTool() {
  if (lastCreatureBrush_) {
    controller_->setBrush(const_cast<Brushes::IBrush *>(lastCreatureBrush_));
  } else {
    // Search the registry for any CreatureBrush to use as default
    for (auto *brush : registry_->getAllBrushes()) {
      if (brush && brush->getType() == Brushes::BrushType::Creature) {
        lastCreatureBrush_ = brush;
        controller_->setBrush(brush);
        break;
      }
    }
  }
}

} // namespace MapEditor::UI::Panels
