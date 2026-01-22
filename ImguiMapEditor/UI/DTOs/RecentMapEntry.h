#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace MapEditor {
namespace UI {

/**
 * Recent map entry for the startup dialog list
 */
struct RecentMapEntry {
  std::filesystem::path path;
  std::string filename;
  std::string last_modified;
  uint32_t detected_version = 0;
  bool exists = true;
};

} // namespace UI
} // namespace MapEditor
