#pragma once

namespace MapEditor::Brushes {
class IBrush;
}

namespace MapEditor::UI {

/**
 * Shared visibility and capability helper for brush tool options.
 * Matches RME's ToolOptionsSurface logic to decide which controls
 * to expose in the BrushSettingsPanel.
 */
class BrushToolOptionsModel {
public:
  static bool isCreatureToolMode(const Brushes::IBrush *brush);
  static bool hasBrushSizeControls(const Brushes::IBrush *brush);
  static bool hasThicknessControl(const Brushes::IBrush *brush);
  static bool hasPreviewBorderControl(const Brushes::IBrush *brush);
  static bool hasAutoBorderControl(const Brushes::IBrush *brush);
  static bool hasLockDoorsControl(const Brushes::IBrush *brush);
  static bool hasSpawnControls(const Brushes::IBrush *brush);
};

} // namespace MapEditor::UI
