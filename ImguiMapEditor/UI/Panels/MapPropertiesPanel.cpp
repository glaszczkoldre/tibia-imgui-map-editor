#include "UI/Panels/MapPropertiesPanel.h"
#include "Core/Config.h"
#include "Domain/ChunkedMap.h"
#include "UI/Core/Theme.h"
#include "ext/fontawesome6/IconsFontAwesome6.h"
#include <format>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace MapEditor {
namespace UI {

namespace SC = SemanticColors;

void MapPropertiesPanel::initialize(Services::ClientVersionRegistry *registry) {
  registry_ = registry;
}

MapPropertiesPanel::State
MapPropertiesPanel::loadFromMap(Domain::ChunkedMap *map) {
  State s;
  if (!map)
    return s;

  s.map_name = map->getName();
  s.description = map->getDescription();
  s.map_width = map->getWidth();
  s.map_height = map->getHeight();
  s.house_file = map->getHouseFile();
  s.spawn_file = map->getSpawnFile();

  const auto &ver = map->getVersion();
  s.otbm_version = ver.otbm_version;
  s.items_major = ver.items_major_version;
  s.items_minor = ver.items_minor_version;

  s.client_version_name = std::format("Unknown (v{})", ver.client_version);
  if (registry_) {
    auto *cv = registry_->findBestMatch(ver.items_minor_version,
                                        ver.items_major_version);
    if (cv) {
      s.client_version_name =
          std::format("{} ({})", cv->getName(), cv->getVersion());
    }
  }

  return s;
}

bool MapPropertiesPanel::render(State &state) {
  bool changed = false;
  auto avail = ImGui::GetContentRegionAvail();

  float right_w = avail.x * RIGHT_RATIO - ImGui::GetStyle().ItemSpacing.x * 0.5f;
  float left_w = avail.x - right_w - ImGui::GetStyle().ItemSpacing.x;

  auto label = SC::TextDim();

  // =========================================================
  //  LEFT COLUMN
  // =========================================================
  ImGui::BeginChild("##left", ImVec2(left_w, avail.y), ImGuiChildFlags_Borders);

  // ---- Map Name (read-only) ----
  ImGui::TextColored(label, ICON_FA_FILE " Map Name");
  ImGui::BeginDisabled();
  ImGui::SetNextItemWidth(-1);
  ImGui::InputText("##mapname", &state.map_name, ImGuiInputTextFlags_ReadOnly);
  ImGui::EndDisabled();
  ImGui::Spacing();

  // ---- Client Version (read-only) ----
  ImGui::TextColored(label, ICON_FA_CODE_BRANCH " Client Version");
  ImGui::BeginDisabled();
  ImGui::SetNextItemWidth(-1);
  ImGui::InputText("##clientver", &state.client_version_name,
                   ImGuiInputTextFlags_ReadOnly);
  ImGui::EndDisabled();
  ImGui::Spacing();

  // ---- Version Details (read-only) ----
  ImGui::Spacing();
  ImGui::BeginDisabled();

  float fw = 60.0f;
  ImGui::TextColored(label, "OTBM");      ImGui::SameLine(72);
  ImGui::TextColored(label, "Major");      ImGui::SameLine(144);
  ImGui::TextColored(label, "Minor");

  ImGui::SetNextItemWidth(fw);
  int ov = (int)state.otbm_version;
  ImGui::InputInt("##otbm", &ov, 0, 0);
  ImGui::SameLine(0, 7);
  ImGui::SetNextItemWidth(fw);
  int ma = (int)state.items_major;
  ImGui::InputInt("##major", &ma, 0, 0);
  ImGui::SameLine(0, 7);
  ImGui::SetNextItemWidth(fw);
  int mi = (int)state.items_minor;
  ImGui::InputInt("##minor", &mi, 0, 0);

  ImGui::EndDisabled();

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // ---- Size (read-only) ----
  ImGui::BeginDisabled();
  ImGui::TextColored(label, "Size");
  ImGui::SameLine(90);
  ImGui::TextColored(label, "X");
  ImGui::SameLine(175);
  ImGui::TextColored(label, "Y");

  ImGui::SetNextItemWidth(80);
  int w = state.map_width;
  ImGui::InputInt("##w", &w, 0, 0);
  ImGui::SameLine(0, 8);
  ImGui::SetNextItemWidth(80);
  int h = state.map_height;
  ImGui::InputInt("##h", &h, 0, 0);
  ImGui::EndDisabled();

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // ---- External Files (editable) ----
  ImGui::TextColored(label, ICON_FA_LINK " External Files");

  ImGui::Text("House File:");
  ImGui::SameLine(100);
  ImGui::SetNextItemWidth(-1);
  if (ImGui::InputText("##housefile", &state.house_file))
    changed = true;
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("External XML file for house data (e.g., map-houses.xml)");
  }

  ImGui::Text("Spawn File:");
  ImGui::SameLine(100);
  ImGui::SetNextItemWidth(-1);
  if (ImGui::InputText("##spawnfile", &state.spawn_file))
    changed = true;
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("External XML file for spawn data (e.g., map-spawns.xml)");
  }

  ImGui::EndChild();

  ImGui::SameLine();

  // =========================================================
  //  RIGHT COLUMN — Description
  // =========================================================
  ImGui::BeginChild("##right", ImVec2(right_w, avail.y), ImGuiChildFlags_Borders);
  ImGui::TextColored(label, ICON_FA_FILE_LINES " Description");
  ImGui::Spacing();
  if (ImGui::InputTextMultiline("##desc", &state.description,
                                ImVec2(-1, ImGui::GetContentRegionAvail().y - 1),
                                ImGuiInputTextFlags_None))
    changed = true;
  ImGui::EndChild();

  return changed;
}

} // namespace UI
} // namespace MapEditor
