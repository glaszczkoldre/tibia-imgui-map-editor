#pragma once

#include "BrushSettingsService.h"
#include <string>
#include <vector>

namespace MapEditor::Services {

class BrushSettingsSerializer {
public:
  [[nodiscard]] static bool saveCustomBrushes(
      const std::string &filepath,
      const std::vector<CustomBrushShape> &customBrushes);

  [[nodiscard]] static bool loadCustomBrushes(
      const std::string &filepath,
      std::vector<CustomBrushShape> &customBrushes);
};

} // namespace MapEditor::Services
