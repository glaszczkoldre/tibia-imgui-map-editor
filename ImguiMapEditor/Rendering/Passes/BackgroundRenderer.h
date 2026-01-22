#pragma once
#include "Rendering/Core/Texture.h"
#include <string>

namespace MapEditor {
namespace Rendering {

/**
 * Renders a background image for startup screens.
 *
 * Loads data/background.jpg on first use and stretches it to fill the viewport.
 * Gracefully handles missing files.
 */
class BackgroundRenderer {
public:
  BackgroundRenderer() = default;
  ~BackgroundRenderer() = default;

  // Non-copyable
  BackgroundRenderer(const BackgroundRenderer &) = delete;
  BackgroundRenderer &operator=(const BackgroundRenderer &) = delete;

  /**
   * Attempt to load the background image from data/background.jpg.
   * Safe to call multiple times - only loads once.
   * @return true if image was loaded, false if missing or failed
   */
  bool tryLoad();

  /**
   * Render the tiled background to fill the viewport.
   * Uses ImGui background draw list to render behind all windows.
   * No-op if image not loaded.
   */
  void render();

  /**
   * Check if background image is loaded and ready.
   */
  bool isLoaded() const { return texture_.isValid(); }

private:
  Texture texture_;
  bool load_attempted_ = false;
};

} // namespace Rendering
} // namespace MapEditor
