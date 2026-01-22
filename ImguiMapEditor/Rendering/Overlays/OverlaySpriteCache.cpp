#include "OverlaySpriteCache.h"

namespace MapEditor {
namespace Rendering {

void OverlaySpriteCache::createPlaceholder() {
  // Create a 32x32 magenta/black checkerboard placeholder texture
  std::vector<uint8_t> data(32 * 32 * 4);

  for (int y = 0; y < 32; ++y) {
    for (int x = 0; x < 32; ++x) {
      int idx = (y * 32 + x) * 4;
      bool checker = ((x / 8) + (y / 8)) % 2 == 0;

      if (checker) {
        data[idx + 0] = 255;
        data[idx + 1] = 0;
        data[idx + 2] = 255;
        data[idx + 3] = 255;
      } else {
        data[idx + 0] = 0;
        data[idx + 1] = 0;
        data[idx + 2] = 0;
        data[idx + 3] = 255;
      }
    }
  }

  placeholder_ = std::make_unique<Texture>(32, 32, data.data());
}

Texture *OverlaySpriteCache::loadSprite(uint32_t sprite_id) {
  if (!spr_reader_) {
    return nullptr;
  }

  auto sprite = spr_reader_->loadSprite(sprite_id);
  if (!sprite) {
    return nullptr;
  }

  if (!sprite->is_decoded) {
    sprite->decode();
  }

  if (sprite->rgba_data.empty()) {
    return nullptr;
  }

  auto texture = std::make_unique<Texture>(32, 32, sprite->rgba_data.data());
  Texture *ptr = texture.get();
  cache_[sprite_id] = std::move(texture);

  return ptr;
}

Texture &OverlaySpriteCache::getTextureOrPlaceholder(uint32_t sprite_id) {
  if (sprite_id == 0) {
    return *placeholder_;
  }

  // Check cache
  auto it = cache_.find(sprite_id);
  if (it != cache_.end()) {
    return *it->second;
  }

  // Load
  Texture *tex = loadSprite(sprite_id);
  if (tex) {
    return *tex;
  }

  return *placeholder_;
}

} // namespace Rendering
} // namespace MapEditor
