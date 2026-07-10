#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <imgui.h>

#include "Rendering/Core/Texture.h"

namespace MapEditor::UI {

/**
 * RAII cache for UI icon textures loaded from PNG files.
 * Ensures textures are loaded once, owned safely, and cleaned up correctly.
 */
class IconTextureCache {
public:
  IconTextureCache() = default;
  ~IconTextureCache();

  // Non-copyable
  IconTextureCache(const IconTextureCache &) = delete;
  IconTextureCache &operator=(const IconTextureCache &) = delete;

  // Moveable
  IconTextureCache(IconTextureCache &&) noexcept = default;
  IconTextureCache &operator=(IconTextureCache &&) noexcept = default;

  /**
   * Load all required tool PNG assets from the specified folder.
   * Throws std::runtime_error on failure.
   */
  void loadAll(const std::string &assetsDir);

  struct IconInfo {
    ImTextureID textureId = {};
    uint32_t width = 0;
    uint32_t height = 0;
  };

  /**
   * Get loaded icon info by name (e.g. "eraser", "protection_zone").
   * Returns empty/null IconInfo if the icon was not loaded.
   */
  IconInfo getIcon(const std::string &name) const;

private:
  std::unordered_map<std::string, std::unique_ptr<Rendering::Texture>> textures_;
};

} // namespace MapEditor::UI
