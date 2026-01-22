#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace MapEditor {

namespace Domain {
namespace Tileset {
class TilesetRegistry;
}
namespace Palette {
class PaletteRegistry;
}
} // namespace Domain

namespace Services {
class ClientDataService;
class SpriteManager;
struct AppSettings;
} // namespace Services

namespace Brushes {
class BrushController;
} // namespace Brushes

namespace UI {

class PaletteWindow;

/**
 * Manages all palette windows.
 *
 * Responsibilities:
 * - Tracks open palette windows (one per palette, created on demand)
 * - Renders all managed windows
 * - Palette names come from PaletteRegistry
 */
class PaletteWindowManager {
public:
  PaletteWindowManager();
  ~PaletteWindowManager();

  /**
   * Initialize with required services.
   */
  void initialize(Services::ClientDataService *clientData,
                  Services::SpriteManager *spriteManager,
                  Brushes::BrushController *brushController,
                  Domain::Tileset::TilesetRegistry &tilesetRegistry,
                  Domain::Palette::PaletteRegistry &paletteRegistry,
                  Services::AppSettings *appSettings = nullptr);

  /**
   * Set app settings (can be called before or after initialize).
   */
  void setAppSettings(Services::AppSettings *appSettings);

  /**
   * Open/show a palette window. Creates it if first time.
   */
  void openPaletteWindow(const std::string &paletteName);

  /**
   * Toggle visibility of a palette window.
   */
  void togglePaletteWindow(const std::string &paletteName);

  /**
   * Check if a palette window is visible.
   */
  bool isPaletteWindowVisible(const std::string &paletteName) const;

  /**
   * Render all managed windows. Call from main render loop.
   */
  void renderAllWindows();

  /**
   * Save visibility state to AppSettings (call before shutdown).
   */
  void saveState();

  /**
   * Restore visibility state from AppSettings (call after initialize).
   */
  void restoreState();

private:
  void createPaletteWindow(const std::string &paletteName);

  // Services (non-owning)
  Services::ClientDataService *clientData_ = nullptr;
  Services::SpriteManager *spriteManager_ = nullptr;
  Brushes::BrushController *brushController_ = nullptr;
  Services::AppSettings *appSettings_ = nullptr;
  Domain::Tileset::TilesetRegistry *tilesetRegistry_ = nullptr;
  Domain::Palette::PaletteRegistry *paletteRegistry_ = nullptr;

  // Palette windows (one per palette, created on demand)
  std::map<std::string, std::unique_ptr<PaletteWindow>> paletteWindows_;
};

} // namespace UI
} // namespace MapEditor
