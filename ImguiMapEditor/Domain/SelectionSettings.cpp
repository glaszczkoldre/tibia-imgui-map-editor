#include "SelectionSettings.h"
#include "Services/ConfigService.h"
namespace MapEditor::Domain {

void SelectionSettings::loadFromConfig(const Services::ConfigService &config) {
  floor_scope = static_cast<SelectionFloorScope>(
      config.get<int>("selection.floor_scope", 0));
  use_pixel_perfect = config.get<bool>("selection.use_pixel_perfect", false);
}

void SelectionSettings::saveToConfig(Services::ConfigService &config) const {
  config.set("selection.floor_scope", static_cast<int>(floor_scope));
  config.set("selection.use_pixel_perfect", use_pixel_perfect);
}

} // namespace MapEditor::Domain
