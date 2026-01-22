#pragma once

#include <cstdint>
#include <string>

namespace MapEditor {
namespace UI {

/**
 * Client version information display
 */
struct ClientInfo {
  // Section 1: Basic Info
  std::string client_name; // e.g., "7.4"
  uint32_t version = 0;
  std::string version_string; // e.g., "Tibia 13.20"
  std::string data_directory; // e.g., "740"

  // Section 2: Version Comparison
  uint32_t otbm_version = 0;        // OTBM format version (for comparison)
  uint32_t items_major_version = 0; // Items major version (for comparison)
  uint32_t items_minor_version = 0; // Items minor version (for comparison)

  // Section 3: Signatures & Description
  std::string dat_signature; // DAT file signature (hex)
  std::string spr_signature; // SPR file signature (hex)
  std::string description;   // User-editable description

  // Status
  std::string status; // "Matched", "Not Found"
  bool signatures_match = false;
};

} // namespace UI
} // namespace MapEditor
