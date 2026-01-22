#pragma once
#include "UI/Panels/NewMapPanel.h"
#include <functional>

namespace MapEditor {
namespace UI {

/**
 * Standalone modal dialog for creating new maps from Editor state.
 * Uses NewMapPanel as the content component.
 */
class NewMapDialog {
public:
  using OnConfirmCallback = std::function<void(const NewMapPanel::State&)>;

  NewMapDialog() = default;

  void initialize(Services::ClientVersionRegistry* registry);

  void show();
  void render();
  
  bool isVisible() const { return visible_; }
  
  void setOnConfirm(OnConfirmCallback callback) { on_confirm_ = std::move(callback); }

private:
  bool visible_ = false;
  NewMapPanel panel_;
  NewMapPanel::State state_;
  OnConfirmCallback on_confirm_;
};

} // namespace UI
} // namespace MapEditor
