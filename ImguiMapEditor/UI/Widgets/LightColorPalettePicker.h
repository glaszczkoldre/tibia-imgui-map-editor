#pragma once

#include <cstdint>

namespace MapEditor::UI {

bool LightColorPalettePicker(const char* id,
                             uint8_t& color,
                             bool show_index = true,
                             const char* tooltip = "Server Light Color");

} // namespace MapEditor::UI
