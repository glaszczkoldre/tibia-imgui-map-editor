#pragma once
#include "Domain/ChunkedMap.h"
#include "UI/Panels/MapPropertiesPanel.h"
#include <string>

namespace MapEditor {
namespace UI {

class MapPropertiesDialog {
public:
  enum class Result {
    None,
    Applied,
    Cancelled
  };

  void initialize(Services::ClientVersionRegistry *registry);
  void show(Domain::ChunkedMap *map);
  Result render();
  bool isOpen() const { return is_open_; }

private:
  void applyToMap();

  bool should_open_ = false;
  bool is_open_ = false;
  Domain::ChunkedMap *map_ = nullptr;

  MapPropertiesPanel panel_;
  MapPropertiesPanel::State state_;
};

} // namespace UI
} // namespace MapEditor
