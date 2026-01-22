#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace MapEditor {
namespace Services {
class BrushSettingsService;
} // namespace Services
} // namespace MapEditor

namespace MapEditor {
namespace UI {
namespace Panels {

/**
 * Compact dockable panel for brush size and shape settings.
 *
 * Features:
 * - Icon-only shape buttons (Square, Circle, Custom)
 * - Dual W/H sliders with symmetric lock (Square/Circle modes)
 * - Interactive 11×11 preview grid (editable in Custom mode)
 * - Inline brush management (dropdown, save/edit/delete)
 * - Quick preset buttons (3×3, 5×5, Diamond, Cross)
 */
class BrushSizePanel {
public:
  using SaveCallback = std::function<void()>;

  explicit BrushSizePanel(Services::BrushSettingsService *brushService,
                          SaveCallback onSave = nullptr);
  ~BrushSizePanel() = default;

  // Non-copyable
  BrushSizePanel(const BrushSizePanel &) = delete;
  BrushSizePanel &operator=(const BrushSizePanel &) = delete;

  void render(bool *p_visible = nullptr);

private:
  Services::BrushSettingsService *service_;
  SaveCallback onSave_;
  bool symmetricSize_ = true;

  // Custom brush editing state
  bool isEditingCustomBrush_ = false;
  bool isNewBrushMode_ = false; // For pulsing "New" state
  std::string editingBrushName_;
  std::vector<std::vector<bool>> customGrid_; // 11×11 editable grid
  static constexpr int GRID_SIZE = 11;

  // Layout sections
  void renderTopRow();
  void renderSizeSliders();
  void renderCustomBrushControls();
  void renderPreviewSection(float availableHeight, bool isInteractive);
  void renderBottomButtons(); // New/Save/Clear/Delete buttons
  void renderPresetButtons();
  void renderSpawnSection(); // Spawn settings UI

  // Grid drawing (interactive or read-only)
  void drawInteractiveGrid(float maxSize);
  void drawReadOnlyGrid(float maxSize);

  // Custom brush management
  void loadSelectedBrushToGrid();
  void saveGridAsNewBrush();
  void saveGridToCurrentBrush();
  void deleteCurrentBrush();
  void applyPreset(const char *preset);
  void syncGridToService();

  // Persistence helpers
  void autoSaveBrushes();
};

} // namespace Panels
} // namespace UI
} // namespace MapEditor
