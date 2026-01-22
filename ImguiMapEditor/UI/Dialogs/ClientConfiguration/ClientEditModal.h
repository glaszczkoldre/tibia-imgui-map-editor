#pragma once
#include "UI/DTOs/ClientEditData.h"
#include "Domain/ClientVersion.h"
#include "Services/ClientVersionRegistry.h"
#include <functional>
#include <string>

namespace MapEditor {
namespace UI {

/**
 * Modal dialog for adding or editing client versions.
 * Extracted from ClientConfigurationDialog for separation of concerns.
 */
class ClientEditModal {
public:
  using SaveCallback = std::function<void()>;
  using BrowseCallback = std::function<void(ClientEditData &)>;

  void setRegistry(Services::ClientVersionRegistry *registry) {
    registry_ = registry;
  }

  void setCallbacks(SaveCallback on_save, BrowseCallback on_browse) {
    on_save_ = std::move(on_save);
    on_browse_ = std::move(on_browse);
  }

  // Open modal for adding new client
  void openForAdd();

  // Open modal for editing existing client
  void openForEdit(uint32_t version);

  // Render the modal (call every frame)
  void render();

  // Check if modal is open
  bool isOpen() const { return show_modal_; }

private:
  void fillEditData(uint32_t version);
  void clearEditData();
  bool saveClient();

  Services::ClientVersionRegistry *registry_ = nullptr;
  bool show_modal_ = false;
  bool is_new_client_ = false;
  ClientEditData edit_data_ = {};

  SaveCallback on_save_;
  BrowseCallback on_browse_;
};

} // namespace UI
} // namespace MapEditor
