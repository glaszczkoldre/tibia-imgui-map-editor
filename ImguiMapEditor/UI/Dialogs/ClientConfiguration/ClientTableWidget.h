#pragma once
#include "Services/ClientVersionRegistry.h"
#include <cstdint>
#include <functional>
#include <string>

namespace MapEditor {
namespace UI {

/**
 * Renders the client version table for ClientConfigurationDialog.
 * Extracted component for separation of concerns.
 */
class ClientTableWidget {
public:
  using SelectionCallback = std::function<void(uint32_t version)>;
  using DefaultChangedCallback = std::function<void(uint32_t version)>;
  using EditCallback = std::function<void(uint32_t version)>;
  using DeleteCallback = std::function<void(uint32_t version)>;

  void setRegistry(Services::ClientVersionRegistry *registry) {
    registry_ = registry;
  }

  void setSelectedVersion(uint32_t version) { selected_version_ = version; }
  uint32_t getSelectedVersion() const { return selected_version_; }

  void setFilter(const char *filter) { filter_ = filter ? filter : ""; }

  void setSelectionCallback(SelectionCallback callback) {
    on_selection_ = std::move(callback);
  }

  void setDefaultChangedCallback(DefaultChangedCallback callback) {
    on_default_changed_ = std::move(callback);
  }

  void setEditCallback(EditCallback callback) {
    on_edit_ = std::move(callback);
  }

  void setDeleteCallback(DeleteCallback callback) {
    on_delete_ = std::move(callback);
  }

  void render(float height);

private:
  Services::ClientVersionRegistry *registry_ = nullptr;
  uint32_t selected_version_ = 0;
  std::string filter_;
  SelectionCallback on_selection_;
  DefaultChangedCallback on_default_changed_;
  EditCallback on_edit_;
  DeleteCallback on_delete_;
};

} // namespace UI
} // namespace MapEditor
