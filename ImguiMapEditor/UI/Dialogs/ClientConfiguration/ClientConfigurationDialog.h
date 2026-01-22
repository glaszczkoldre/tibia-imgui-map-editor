#pragma once
#include "UI/Dialogs/ClientConfiguration/ClientDetailsCard.h"
#include "UI/Dialogs/ClientConfiguration/ClientEditModal.h"
#include "UI/Dialogs/ClientConfiguration/ClientTableWidget.h"
#include "UI/DTOs/ClientEditData.h"
#include <filesystem>
#include <functional>
#include <imgui.h>
#include <string>

namespace MapEditor {
namespace Services {
class ClientVersionRegistry;
}

namespace UI {

/**
 * Enhanced dialog for configuring all client versions.
 * Shows all fields from clients.json and allows full CRUD operations.
 * Delegates rendering to extracted components.
 */
class ClientConfigurationDialog {
public:
  ClientConfigurationDialog() = default;

  // Open the dialog
  void open(Services::ClientVersionRegistry &registry);

  // Render the dialog (returns true while open)
  bool render();

  // Close the dialog
  void close();

  // Check if dialog is open
  bool isOpen() const { return is_open_; }

  // Callback when changes are saved
  std::function<void()> onSave;

private:
  void renderDeleteConfirmation();
  void browseForPath();

  Services::ClientVersionRegistry *registry_ = nullptr;
  bool is_open_ = false;

  // Selection state
  uint32_t selected_version_ = 0;
  char filter_buffer_[64] = {};

  // Extracted components
  ClientTableWidget table_widget_;
  ClientDetailsCard details_card_;
  ClientEditModal edit_modal_;

  // Delete confirmation state
  bool show_delete_confirmation_ = false;
  uint32_t version_to_delete_ = 0;
};

} // namespace UI
} // namespace MapEditor
