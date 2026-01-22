#pragma once
#include <cstdint>
#include <format>
#include <string>

namespace MapEditor {
namespace Utils {

/**
 * Format a client version number as "X.YY" string.
 * E.g., 772 -> "7.72", 860 -> "8.60", 1098 -> "10.98"
 */
inline std::string formatVersion(uint32_t version) {
  return std::format("{}.{:02}", version / 100, version % 100);
}

} // namespace Utils
} // namespace MapEditor
