#pragma once
/**
 * @file BrushEnums.h
 * @brief Enumerations for brush types, edge alignments, and neighbor masks.
 *
 * These enums map directly to RME's brush_enums.h values and XML attribute
 * strings. All string conversions are provided for XML parsing compatibility.
 */

#include "../../Utils/EnumFlags.h"
#include <cstdint>
#include <string_view>

namespace MapEditor::Brushes {

// ═══════════════════════════════════════════════════════════════════════════
// Edge Name Constants (Match XML brush attributes exactly)
// ═══════════════════════════════════════════════════════════════════════════

/**
 * String constants that match RME brushes.xml edge attribute values.
 * Used for XML parsing and serialization.
 */
namespace EdgeName {
// Cardinal directions (borders, carpets)
constexpr std::string_view N = "n";
constexpr std::string_view S = "s";
constexpr std::string_view E = "e";
constexpr std::string_view W = "w";

// Corners (c = outer corner)
constexpr std::string_view CNE = "cne"; // corner northeast
constexpr std::string_view CNW = "cnw"; // corner northwest
constexpr std::string_view CSE = "cse"; // corner southeast
constexpr std::string_view CSW = "csw"; // corner southwest

// Diagonals (d = diagonal/inner)
constexpr std::string_view DNE = "dne"; // diagonal northeast
constexpr std::string_view DNW = "dnw"; // diagonal northwest
constexpr std::string_view DSE = "dse"; // diagonal southeast
constexpr std::string_view DSW = "dsw"; // diagonal southwest

// Carpet center
constexpr std::string_view CENTER = "center";
} // namespace EdgeName

// ═══════════════════════════════════════════════════════════════════════════
// EdgeType - Border/Carpet edge alignment
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Edge types for ground borders and carpets.
 * Maps to XML edge attribute: n, s, e, w, cne, cnw, cse, csw, dne, dnw, dse,
 * dsw
 */
enum class EdgeType : uint8_t {
  None = 0,
  N = 1,      // "n" - north horizontal
  S = 2,      // "s" - south horizontal
  E = 3,      // "e" - east horizontal
  W = 4,      // "w" - west horizontal
  CNE = 5,    // "cne" - corner northeast (outer)
  CNW = 6,    // "cnw" - corner northwest (outer)
  CSE = 7,    // "cse" - corner southeast (outer)
  CSW = 8,    // "csw" - corner southwest (outer)
  DNE = 9,    // "dne" - diagonal northeast (inner)
  DNW = 10,   // "dnw" - diagonal northwest (inner)
  DSE = 11,   // "dse" - diagonal southeast (inner)
  DSW = 12,   // "dsw" - diagonal southwest (inner)
  Center = 13 // "center" - carpet center only
};

// ═══════════════════════════════════════════════════════════════════════════
// TableAlign - Table brush alignment
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Table alignment types.
 * Maps to XML <table align="...">: alone, east, horizontal, north, south,
 * vertical, west
 */
enum class TableAlign : uint8_t {
  Alone = 0,      // "alone"
  North = 1,      // "north"
  South = 2,      // "south"
  East = 3,       // "east"
  West = 4,       // "west"
  Horizontal = 5, // "horizontal"
  Vertical = 6    // "vertical"
};

// ═══════════════════════════════════════════════════════════════════════════
// WallAlign - Wall brush alignment
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Wall alignment types.
 * Maps to XML <wall type="...">: pole, corner, horizontal, vertical, etc.
 * NOTE: XML uses spaced strings like "east T", "north end"
 */
enum class WallAlign : uint8_t {
  Pole = 0,               // "pole"
  Corner = 1,             // "corner" (northwest diagonal)
  Horizontal = 2,         // "horizontal"
  Vertical = 3,           // "vertical"
  NorthEnd = 4,           // "north end"
  SouthEnd = 5,           // "south end"
  EastEnd = 6,            // "east end"
  WestEnd = 7,            // "west end"
  NorthT = 8,             // "north T"
  SouthT = 9,             // "south T"
  EastT = 10,             // "east T"
  WestT = 11,             // "west T"
  Intersection = 12,      // "intersection"
  NortheastDiagonal = 13, // "northeast diagonal"
  SouthwestDiagonal = 14, // "southwest diagonal"
  Untouchable = 15        // internal only
};

// ═══════════════════════════════════════════════════════════════════════════
// DoorType - Door/window types within walls
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Door types within wall brushes.
 * Maps to XML <door type="...">
 */
enum class DoorType : uint8_t {
  Undefined = 0,
  Archway = 1,    // "archway"
  Normal = 2,     // "normal"
  Locked = 3,     // "locked"
  Quest = 4,      // "quest"
  Magic = 5,      // "magic"
  NormalAlt = 6,  // "normal_alt"
  Window = 7,     // "window"
  HatchWindow = 8 // "hatch_window"
};

// ═══════════════════════════════════════════════════════════════════════════
// TileNeighbor - 8-bit bitmask for neighbor detection (lookup table index)
// ═══════════════════════════════════════════════════════════════════════════

/**
 * 8-neighbor bitmask for auto-border calculation.
 * Used as index into 256-entry lookup tables.
 * Matches RME's TileAlignment enum values.
 */
enum class TileNeighbor : uint8_t {
  None = 0,
  Northwest = 1 << 0, // 1
  North = 1 << 1,     // 2
  Northeast = 1 << 2, // 4
  West = 1 << 3,      // 8
  East = 1 << 4,      // 16
  Southwest = 1 << 5, // 32
  South = 1 << 6,     // 64
  Southeast = 1 << 7  // 128
};
ENABLE_BITMASK_OPERATORS(TileNeighbor)

// ═══════════════════════════════════════════════════════════════════════════
// WallNeighbor - 4-bit bitmask for wall alignment
// ═══════════════════════════════════════════════════════════════════════════

/**
 * 4-neighbor bitmask for wall alignment calculation.
 * Used as index into 16-entry lookup tables.
 */
enum class WallNeighbor : uint8_t {
  None = 0,
  North = 1 << 0, // 1
  West = 1 << 1,  // 2
  East = 1 << 2,  // 4
  South = 1 << 3  // 8
};
ENABLE_BITMASK_OPERATORS(WallNeighbor)

// ═══════════════════════════════════════════════════════════════════════════
// ZoneFlag - Tile metadata flags
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Zone flags for tile metadata (PZ, noPVP, etc.)
 */
enum class ZoneFlag : uint32_t {
  None = 0,
  ProtectionZone = 1 << 0,
  NoPvp = 1 << 1,
  NoLogout = 1 << 2,
  PvpZone = 1 << 3,
  Refresh = 1 << 4
};
ENABLE_BITMASK_OPERATORS(ZoneFlag)

// ═══════════════════════════════════════════════════════════════════════════
// XML String ↔ Enum Conversion Functions
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Parse edge name from XML attribute to EdgeType.
 * @param name XML attribute value (e.g., "n", "cne", "dsw")
 * @return Corresponding EdgeType, or EdgeType::None if not found
 */
EdgeType parseEdgeName(std::string_view name);

/**
 * Convert EdgeType to XML string.
 * @param type The edge type
 * @return XML attribute string (e.g., "n", "cne", "dsw")
 */
std::string_view edgeTypeToString(EdgeType type);

/**
 * Parse table alignment from XML attribute to TableAlign.
 */
TableAlign parseTableAlign(std::string_view name);

/**
 * Convert TableAlign to XML string.
 */
std::string_view tableAlignToString(TableAlign align);

/**
 * Parse wall type from XML attribute to WallAlign.
 * Handles spaced strings like "east T", "north end"
 */
WallAlign parseWallType(std::string_view name);

/**
 * Convert WallAlign to XML string.
 */
std::string_view wallAlignToString(WallAlign align);

/**
 * Parse door type from XML attribute to DoorType.
 */
DoorType parseDoorType(std::string_view name);

/**
 * Convert DoorType to XML string.
 */
std::string_view doorTypeToString(DoorType type);

} // namespace MapEditor::Brushes
