#pragma once
#include "UI/DTOs/RecentMapEntry.h"
#include <functional>
#include <vector>

namespace MapEditor {
namespace UI {

/**
 * Renders the Recent Maps list panel for the StartupDialog.
 * Extracted component for separation of concerns.
 */
class RecentMapsPanel {
public:
  using SelectionCallback =
      std::function<void(int index, const RecentMapEntry &entry)>;
  using DoubleClickCallback =
      std::function<void(int index, const RecentMapEntry &entry)>;

  void setSelectedIndex(int index) { selected_index_ = index; }
  int getSelectedIndex() const { return selected_index_; }

  void setSelectionCallback(SelectionCallback callback) {
    on_selection_ = std::move(callback);
  }

  void setDoubleClickCallback(DoubleClickCallback callback) {
    on_double_click_ = std::move(callback);
  }

  void setLoadEnabled(bool enabled) { load_enabled_ = enabled; }

  void render(const std::vector<RecentMapEntry> &entries);

private:
  int selected_index_ = -1;
  bool load_enabled_ = false;
  SelectionCallback on_selection_;
  DoubleClickCallback on_double_click_;
};

} // namespace UI
} // namespace MapEditor
