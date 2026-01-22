#pragma once
#include "UI/DTOs/ClientInfo.h"
#include "UI/DTOs/SelectedMapInfo.h"
#include <string>

namespace MapEditor {
namespace UI {

/**
 * Renders the Client information panel for the StartupDialog.
 * Compares client version with selected map metadata.
 */
class ClientInfoPanel {
public:
  void setClientInfo(const ClientInfo &info) { client_info_ = info; }
  const ClientInfo &getClientInfo() const { return client_info_; }

  void setMapInfo(const SelectedMapInfo &info) { map_info_ = info; }

  void setSignatureMismatch(bool mismatch, const std::string &message) {
    signature_mismatch_ = mismatch;
    signature_mismatch_message_ = message;
  }

  void setClientNotConfigured(bool not_configured) {
    client_not_configured_ = not_configured;
  }

  void render();

private:
  ClientInfo client_info_;
  SelectedMapInfo map_info_;
  bool signature_mismatch_ = false;
  std::string signature_mismatch_message_;
  bool client_not_configured_ = false;
};

} // namespace UI
} // namespace MapEditor
