#pragma once

#include <cstdint>
#include <string>

namespace MapEditor {
namespace UI {

/**
 * Selected map metadata display
 */
struct SelectedMapInfo {
  std::string name;
  std::string description;          // Map description from OTBM
  uint32_t width = 0;               // Map width in tiles
  uint32_t height = 0;              // Map height in tiles
  uint32_t client_version = 0;      // Client version (e.g., 860 = 8.60)
  uint32_t otbm_version = 0;        // OTBM format version (0-4)
  uint32_t items_major_version = 0; // OTB items major version
  uint32_t items_minor_version = 0; // OTB items minor version (raw from file)
  std::string house_file;           // House XML filename
  std::string spawn_file;           // Spawn XML filename
  std::string created;              // Creation/last modified date
  bool valid = false;
};

} // namespace UI
} // namespace MapEditor
