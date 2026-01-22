#pragma once
#include "Services/ClientVersionRegistry.h"
#include <cstdint>
#include <functional>

namespace MapEditor {
namespace UI {

/**
 * Renders the Available Clients list panel for the StartupDialog.
 * Shows all clients with configured paths.
 */
class AvailableClientsPanel {
public:
  using SelectionCallback = std::function<void(uint32_t version)>;

  void setRegistry(Services::ClientVersionRegistry *registry) {
    registry_ = registry;
  }

  void setSelectedVersion(uint32_t version) { selected_version_ = version; }
  uint32_t getSelectedVersion() const { return selected_version_; }

  void setSelectionCallback(SelectionCallback callback) {
    on_selection_ = std::move(callback);
  }

  void render();

private:
  Services::ClientVersionRegistry *registry_ = nullptr;
  uint32_t selected_version_ = 0;
  SelectionCallback on_selection_;
};

} // namespace UI
} // namespace MapEditor
