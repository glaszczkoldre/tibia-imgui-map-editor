#pragma once

#include <cstdint>

namespace MapEditor {
namespace UI {

/**
 * Struct to hold client data being edited in the modal.
 * Extracted from ClientConfigurationDialog.h for separation of concerns.
 */
struct ClientEditData {
  uint32_t version = 0;
  char name[64] = {};
  char description[256] = {};
  char data_directory[64] = {};
  char client_path[512] = {};
  uint32_t otb_id = 0;
  uint32_t otb_major = 0;
  uint32_t otbm_version = 0;
  char dat_signature[16] = {};
  char spr_signature[16] = {};
  bool is_default = false;
};

} // namespace UI
} // namespace MapEditor
