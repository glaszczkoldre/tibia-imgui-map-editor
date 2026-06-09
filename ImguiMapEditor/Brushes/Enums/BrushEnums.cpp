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

std::string_view edgeTypeToString(EdgeType type) {
  switch (type) {
  case EdgeType::N:
    return "n";
  case EdgeType::E:
    return "e";
  case EdgeType::S:
    return "s";
  case EdgeType::W:
    return "w";
  case EdgeType::CNW:
    return "cnw";
  case EdgeType::CNE:
    return "cne";
  case EdgeType::CSW:
    return "csw";
  case EdgeType::CSE:
    return "cse";
  case EdgeType::DNW:
    return "dnw";
  case EdgeType::DNE:
    return "dne";
  case EdgeType::DSE:
    return "dse";
  case EdgeType::DSW:
    return "dsw";
  case EdgeType::Center:
    return "center";
  default:
    return "";
  }
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

std::string_view tableAlignToString(TableAlign align) {
  switch (align) {
  case TableAlign::Alone:
    return "alone";
  case TableAlign::North:
    return "north";
  case TableAlign::South:
    return "south";
  case TableAlign::East:
    return "east";
  case TableAlign::West:
    return "west";
  case TableAlign::Horizontal:
    return "horizontal";
  case TableAlign::Vertical:
    return "vertical";
  default:
    return "alone";
  }
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

std::string_view wallAlignToString(WallAlign align) {
  switch (align) {
  case WallAlign::Pole:
    return "pole";
  case WallAlign::NorthwestDiagonal:
    return "corner";
  case WallAlign::SouthEnd:
    return "south end";
  case WallAlign::EastEnd:
    return "east end";
  case WallAlign::NortheastDiagonal:
    return "northeast diagonal";
  case WallAlign::Horizontal:
    return "horizontal";
  case WallAlign::SouthT:
    return "south T";
  case WallAlign::NorthEnd:
    return "north end";
  case WallAlign::Vertical:
    return "vertical";
  case WallAlign::SouthwestDiagonal:
    return "southwest diagonal";
  case WallAlign::EastT:
    return "east T";
  case WallAlign::SoutheastDiagonal:
    return "southeast diagonal";
  case WallAlign::WestEnd:
    return "west end";
  case WallAlign::NorthT:
    return "north T";
  case WallAlign::WestT:
    return "west T";
  case WallAlign::Intersection:
    return "intersection";
  case WallAlign::Untouchable:
    return "untouchable";
  default:
    return "pole";
  }
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

std::string_view doorTypeToString(DoorType type) {
  switch (type) {
  case DoorType::Archway:
    return "archway";
  case DoorType::Normal:
    return "normal";
  case DoorType::Locked:
    return "locked";
  case DoorType::Quest:
    return "quest";
  case DoorType::Magic:
    return "magic";
  case DoorType::NormalAlt:
    return "normal_alt";
  case DoorType::Window:
    return "window";
  case DoorType::HatchWindow:
    return "hatch_window";
  default:
    return "undefined";
  }
}

} // namespace MapEditor::Brushes
