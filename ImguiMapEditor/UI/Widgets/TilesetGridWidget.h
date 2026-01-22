#pragma once
#include "../../Brushes/BrushRegistry.h"
#include "../../Domain/Tileset/Tileset.h"
#include "../../Domain/Tileset/TilesetRegistry.h"
#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace MapEditor::Services {
struct AppSettings;
}

namespace MapEditor {

namespace Services {
class ClientDataService;
class SpriteManager;
} // namespace Services

namespace Brushes {
class BrushController;
} // namespace Brushes

namespace UI {

/**
 * Callback when a brush is selected from the grid.
 * @param itemId Server ID of selected item (for RAW brush)
 * @param brushName Name of selected brush (empty for RAW)
 */
using BrushSelectedCallback =
    std::function<void(uint32_t itemId, const std::string &brushName)>;

/**
 * Callback when a brush is double-clicked (for jump-to-tileset).
 * @param tilesetName Name of tileset containing the brush
 * @param brushName Name of the brush
 */
using BrushDoubleClickCallback = std::function<void(
    const std::string &tilesetName, const std::string &brushName)>;

/**
 * Brush entry with source tileset info for cross-search results.
 */
struct BrushWithSource {
  const Brushes::IBrush *brush;
  std::string sourceTileset;
};

/**
 * Reusable widget to display a tileset's brush/item grid.
 *
 * This is a pure rendering component that can be embedded in:
 * - PaletteWindow (palette-based container)
 * - TilesetWindow (standalone detached window)
 *
 * Features:
 * - Filter input for searching brushes
 * - Icon size slider
 * - Responsive tile grid with selection/hover overlays
 * - Drag-drop reordering
 * - Section separators (collapsible)
 */
class TilesetGridWidget {
public:
  TilesetGridWidget();
  ~TilesetGridWidget();

  /**
   * Initialize with required services.
   */
  void initialize(Services::ClientDataService *clientData,
                  Services::SpriteManager *spriteManager,
                  Brushes::BrushController *brushController,
                  Domain::Tileset::TilesetRegistry &tilesetRegistry,
                  Services::AppSettings *appSettings = nullptr);

  /**
   * Set the tileset to display (directly from Tileset entries).
   */
  void setTileset(const std::string &tilesetName);

  /**
   * Set callback for brush selection.
   */
  void setOnBrushSelected(BrushSelectedCallback callback) {
    onBrushSelected_ = callback;
  }

  /**
   * Set callback for double-click (jump to tileset).
   */
  void setOnBrushDoubleClicked(BrushDoubleClickCallback callback) {
    onBrushDoubleClicked_ = callback;
  }

  /**
   * Set all brushes for cross-tileset search.
   * When filter is active and this is set, search across all provided brushes.
   */
  void setAllBrushes(std::vector<BrushWithSource> brushes) {
    allBrushes_ = std::move(brushes);
    filterDirty_ = true;
  }

  /**
   * Render the full widget (filter + size slider + grid).
   * Call this inside an ImGui window or tab.
   */
  void render();

  /**
   * Render just the controls (dropdown, filter, size slider).
   * Use with renderGridOnly() for custom layouts.
   * @param vertical If true, render vertically stacked. If false, more compact.
   */
  void renderControlsOnly(bool vertical = true);

  /**
   * Render just the brush grid.
   * Use with renderControlsOnly() for custom layouts.
   */
  void renderGridOnly();

  // Icon size - uses global setting if appSettings available, else fallback
  float getIconSize() const;

  // Get current tileset info
  const std::string &getTilesetName() const { return tilesetName_; }

  /**
   * Clear the filter text.
   */
  void clearFilter() {
    filterBuffer_[0] = '\0';
    filterDirty_ = true;
  }

  /**
   * Select a brush by name and optionally scroll to it.
   * Selection is deferred until render to find the correct index-suffixed key.
   * @param pulse If true, starts a pulse animation on the brush.
   */
  void selectBrush(const std::string &brushName, bool scrollTo = true,
                   bool pulse = false) {
    pendingSelectBrushName_ = brushName;
    if (scrollTo) {
      scrollToBrushName_ = brushName;
    }
    if (pulse) {
      pulseBrushName_ = brushName;
      pulseStartTime_ = -1.0f; // Will be set on first render
    }
  }

  /**
   * Callback for tileset modifications (triggers save).
   */
  using TilesetModifiedCallback =
      std::function<void(const std::string &tileset)>;

  void setOnTilesetModified(TilesetModifiedCallback cb) {
    onTilesetModified_ = std::move(cb);
  }

private:
  void renderFilterInput();
  void renderBrushGrid();
  void applyFilter();

  /**
   * Get OpenGL texture ID for a brush (RawBrush or CreatureBrush).
   * Uses PreviewUtils for consistent texture retrieval.
   */
  void *getBrushTextureId(const Brushes::IBrush *brush) const;

  // Services (non-owning)
  Services::ClientDataService *clientData_ = nullptr;
  Services::SpriteManager *spriteManager_ = nullptr;
  Brushes::BrushController *brushController_ = nullptr;
  Domain::Tileset::TilesetRegistry *tilesetRegistry_ = nullptr;

  // Current tileset name
  std::string tilesetName_;

  // State
  float iconSizeFallback_ = 48.0f; // Used only if no AppSettings provided
  char filterBuffer_[128] = "";
  bool filterDirty_ = true;

  // Filtered entries with original index tracking (for separators + brushes)
  struct FilteredEntry {
    size_t originalIndex; // Index in tileset->getEntries()
    Domain::Tileset::TilesetEntry entry;
  };
  std::vector<FilteredEntry> filteredEntries_;

  // App settings (for global icon size)
  Services::AppSettings *appSettings_ = nullptr;

  // Selection
  std::string selectedBrushName_; // Currently selected (index-suffixed key)
  std::string pendingSelectBrushName_; // Brush name to find and select
  std::string scrollToBrushName_;      // Brush name to scroll to on next render

  // All brushes for cross-tileset search
  std::vector<BrushWithSource> allBrushes_;
  // Cross-search results (when filter active and allBrushes_ populated)
  std::vector<BrushWithSource> crossFilteredBrushes_;

  // Callbacks
  BrushSelectedCallback onBrushSelected_;
  BrushDoubleClickCallback onBrushDoubleClicked_;
  TilesetModifiedCallback onTilesetModified_;

  // Drag-drop state
  struct DragState {
    bool isDragging = false;
    int dragSourceIndex = -1; // Index of entry being dragged
    int dropTargetIndex = -1; // Current drop target index
    bool isSwapMode = false;  // True = swap, False = insert
  };
  DragState dragState_;

  // Multi-selection state
  std::set<int> selectedIndices_;
  int lastClickedIndex_ = -1; // For shift-click range select

  // Collapsed sections state (key = original entry index of separator)
  std::unordered_map<size_t, bool> collapsedSections_;

  // Pulse animation state for jump-to-selection highlight
  std::string pulseBrushName_;   // Brush to pulse
  float pulseStartTime_ = -1.0f; // When pulse started (-1 = not set)
  static constexpr float PULSE_DURATION = 2.0f; // Seconds to pulse
};

} // namespace UI
} // namespace MapEditor
