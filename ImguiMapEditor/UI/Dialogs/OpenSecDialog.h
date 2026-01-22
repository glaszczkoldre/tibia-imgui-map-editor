#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <vector>

namespace MapEditor {
namespace Services {
class ClientVersionRegistry;
}

namespace UI {

/**
 * Standalone modal dialog for opening SEC maps from Editor state.
 */
class OpenSecDialog {
public:
  using OnConfirmCallback = std::function<void(const std::filesystem::path&, uint32_t)>;

  OpenSecDialog() = default;

  void initialize(Services::ClientVersionRegistry* registry);

  void show();
  void render();
  
  bool isVisible() const { return visible_; }
  
  void setOnConfirm(OnConfirmCallback callback) { on_confirm_ = std::move(callback); }

private:
  bool visible_ = false;
  Services::ClientVersionRegistry* registry_ = nullptr;
  
  // Modal state
  std::filesystem::path sec_folder_;
  uint32_t sec_version_ = 772;
  bool folder_valid_ = false;
  
  // Cached version list (populated in show() to avoid per-frame recalculation)
  std::vector<uint32_t> sec_versions_;
  
  OnConfirmCallback on_confirm_;
};

} // namespace UI
} // namespace MapEditor
