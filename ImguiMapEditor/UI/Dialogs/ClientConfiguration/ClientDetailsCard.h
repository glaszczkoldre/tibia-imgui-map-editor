#pragma once
#include "Domain/ClientVersion.h"
#include "Services/ClientVersionRegistry.h"
namespace MapEditor {
namespace UI {

/**
 * Renders a details card showing selected client version info.
 * Extracted from ClientConfigurationDialog for separation of concerns.
 */
class ClientDetailsCard {
public:
  void setRegistry(Services::ClientVersionRegistry *registry) {
    registry_ = registry;
  }

  void setSelectedVersion(uint32_t version) { selected_version_ = version; }

  void render();

private:
  Services::ClientVersionRegistry *registry_ = nullptr;
  uint32_t selected_version_ = 0;
};

} // namespace UI
} // namespace MapEditor
