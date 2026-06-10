/**
 * @file BrushEnums.cpp
 * @brief Implementation of XML string ↔ enum conversion functions.
 */

#include "BrushEnums.h"
#include <algorithm>
#include <array>
#include <spdlog/spdlog.h>

namespace MapEditor::Brushes {

// ═══════════════════════════════════════════════════════════════════════════
// EdgeType Conversions
// ═══════════════════════════════════════════════════════════════════════════

EdgeType parseEdgeName(std::string_view name) {
  static constexpr std::array<std::pair<std::string_view, EdgeType>, 13> kMap = {{
      {"center", EdgeType::Center},
      {"cne", EdgeType::CNE},
      {"cnw", EdgeType::CNW},
      {"cse", EdgeType::CSE},
      {"csw", EdgeType::CSW},
      {"dne", EdgeType::DNE},
      {"dnw", EdgeType::DNW},
      {"dse", EdgeType::DSE},
      {"dsw", EdgeType::DSW},
      {"e", EdgeType::E},
      {"n", EdgeType::N},
      {"s", EdgeType::S},
      {"w", EdgeType::W},
  }};
  auto it = std::lower_bound(kMap.begin(), kMap.end(), name,
      [](const auto& entry, std::string_view key) { return entry.first < key; });
  if (it != kMap.end() && it->first == name) return it->second;
  spdlog::warn("[BrushEnums] Unknown edge name: {}", name);
  return EdgeType::None;
}

// ═══════════════════════════════════════════════════════════════════════════
// TableAlign Conversions
// ═══════════════════════════════════════════════════════════════════════════

TableAlign parseTableAlign(std::string_view name) {
  static constexpr std::array<std::pair<std::string_view, TableAlign>, 7> kMap = {{
      {"alone", TableAlign::Alone},
      {"east", TableAlign::East},
      {"horizontal", TableAlign::Horizontal},
      {"north", TableAlign::North},
      {"south", TableAlign::South},
      {"vertical", TableAlign::Vertical},
      {"west", TableAlign::West},
  }};
  auto it = std::lower_bound(kMap.begin(), kMap.end(), name,
      [](const auto& entry, std::string_view key) { return entry.first < key; });
  if (it != kMap.end() && it->first == name) return it->second;
  spdlog::warn("[BrushEnums] Unknown table align: {}", name);
  return TableAlign::Alone;
}

// ═══════════════════════════════════════════════════════════════════════════
// WallAlign Conversions (handles spaced strings like "east T")
// ═══════════════════════════════════════════════════════════════════════════

WallAlign parseWallType(std::string_view name) {
  static constexpr std::array<std::pair<std::string_view, WallAlign>, 18> kMap = {{
      {"corner", WallAlign::Corner},
      {"east end", WallAlign::EastEnd},
      {"east T", WallAlign::EastT},
      {"horizontal", WallAlign::Horizontal},
      {"intersection", WallAlign::Intersection},
      {"north end", WallAlign::NorthEnd},
      {"north T", WallAlign::NorthT},
      {"northeast diagonal", WallAlign::NortheastDiagonal},
      {"northwest diagonal", WallAlign::NorthwestDiagonal},
      {"pole", WallAlign::Pole},
      {"south end", WallAlign::SouthEnd},
      {"south T", WallAlign::SouthT},
      {"southeast diagonal", WallAlign::SoutheastDiagonal},
      {"southwest diagonal", WallAlign::SouthwestDiagonal},
      {"untouchable", WallAlign::Untouchable},
      {"vertical", WallAlign::Vertical},
      {"west end", WallAlign::WestEnd},
      {"west T", WallAlign::WestT},
  }};
  auto it = std::lower_bound(kMap.begin(), kMap.end(), name,
      [](const auto& entry, std::string_view key) { return entry.first < key; });
  if (it != kMap.end() && it->first == name) return it->second;
  spdlog::warn("[BrushEnums] Unknown wall type: {}", name);
  return WallAlign::Pole;
}

// ═══════════════════════════════════════════════════════════════════════════
// DoorType Conversions
// ═══════════════════════════════════════════════════════════════════════════

DoorType parseDoorType(std::string_view name) {
  static constexpr std::array<std::pair<std::string_view, DoorType>, 9> kMap = {{
      {"archway", DoorType::Archway},
      {"hatch window", DoorType::HatchWindow},
      {"hatch_window", DoorType::HatchWindow},
      {"locked", DoorType::Locked},
      {"magic", DoorType::Magic},
      {"normal", DoorType::Normal},
      {"normal_alt", DoorType::NormalAlt},
      {"quest", DoorType::Quest},
      {"window", DoorType::Window},
  }};
  auto it = std::lower_bound(kMap.begin(), kMap.end(), name,
      [](const auto& entry, std::string_view key) { return entry.first < key; });
  if (it != kMap.end() && it->first == name) return it->second;
  spdlog::warn("[BrushEnums] Unknown door type: {}", name);
  return DoorType::Undefined;
}

} // namespace MapEditor::Brushes
