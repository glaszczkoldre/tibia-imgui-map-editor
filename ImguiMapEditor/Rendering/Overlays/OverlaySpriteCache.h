#pragma once
#include "IO/SprReader.h"
#include "Rendering/Core/Texture.h"
#include <memory>
#include <unordered_map>


namespace MapEditor {
namespace Rendering {

/**
 * Simple texture cache for overlay/ImGui rendering.
 *
 * Provides individual textures for use with ImGui AddImage and similar APIs
 * that require 2D texture IDs. This is separate from the atlas-based
 * batched rendering system used for map tiles.
 *
 * Usage: PreviewRenderer, UI widgets that need sprite textures
 */
class OverlaySpriteCache {
public:
  explicit OverlaySpriteCache(std::shared_ptr<IO::SprReader> spr_reader)
      : spr_reader_(std::move(spr_reader)) {
    createPlaceholder();
  }

  ~OverlaySpriteCache() = default;

  // Non-copyable
  OverlaySpriteCache(const OverlaySpriteCache &) = delete;
  OverlaySpriteCache &operator=(const OverlaySpriteCache &) = delete;

  /**
   * Get texture for a sprite ID, or placeholder if not found.
   * Never returns nullptr.
   */
  Texture &getTextureOrPlaceholder(uint32_t sprite_id);

  /**
   * Clear all cached textures.
   */
  void clearCache() { cache_.clear(); }

  /**
   * Get cache size.
   */
  size_t getCacheSize() const { return cache_.size(); }

private:
  void createPlaceholder();
  Texture *loadSprite(uint32_t sprite_id);

  std::shared_ptr<IO::SprReader> spr_reader_;
  std::unordered_map<uint32_t, std::unique_ptr<Texture>> cache_;
  std::unique_ptr<Texture> placeholder_;
};

} // namespace Rendering
} // namespace MapEditor
