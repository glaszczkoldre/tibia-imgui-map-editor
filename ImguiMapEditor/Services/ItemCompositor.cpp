#include "ItemCompositor.h"
#include "IO/SprReader.h"
#include "Utils/ImageBlending.h"
#include "Utils/SpriteUtils.h"
#include <algorithm>

namespace MapEditor {
namespace Services {

ItemCompositor::ItemCompositor(std::shared_ptr<IO::SprReader> spr_reader)
    : spr_reader_(std::move(spr_reader)) {}

Rendering::Texture *
ItemCompositor::getCompositedItemTexture(const Domain::ItemType *type) {

  if (!type || type->sprite_ids.empty() || !spr_reader_) {
    return nullptr;
  }

  const uint16_t client_id = type->client_id;

  // Check cache first
  auto it = cache_.find(client_id);
  if (it != cache_.end()) {
    return it->second.get();
  }

  // For single-tile items (1x1), load directly and cache
  if (type->width == 1 && type->height == 1) {
    uint32_t sprite_id = type->sprite_ids[0];
    auto sprite_data =
        Utils::SpriteUtils::loadDecodedSprite(spr_reader_, sprite_id);
    if (sprite_data.empty()) {
      return nullptr;
    }

    // Create and cache texture
    auto texture =
        std::make_unique<Rendering::Texture>(32, 32, sprite_data.data());
    Rendering::Texture *ptr = texture.get();
    cache_[client_id] = std::move(texture);
    return ptr;
  }

  // Multi-tile item: need to composite all parts
  const uint8_t width = type->width;
  const uint8_t height = type->height;
  const int composite_size = std::max(width, height) * 32;

  // Create RGBA buffer for the composited image
  std::vector<uint8_t> composite_rgba(composite_size * composite_size * 4, 0);

  // Background color (same as RME's ICON_BACKGROUND setting - gray)
  constexpr uint8_t BG_SHADE = 48;
  for (size_t i = 0; i < composite_rgba.size(); i += 4) {
    composite_rgba[i + 0] = BG_SHADE;
    composite_rgba[i + 1] = BG_SHADE;
    composite_rgba[i + 2] = BG_SHADE;
    composite_rgba[i + 3] = 255;
  }

  // Composite each sprite part
  for (uint8_t h = 0; h < height; ++h) {
    for (uint8_t w = 0; w < width; ++w) {
      size_t sprite_index = static_cast<size_t>(h) * width + w;

      if (sprite_index >= type->sprite_ids.size()) {
        continue;
      }

      uint32_t sprite_id = type->sprite_ids[sprite_index];
      auto sprite_data =
          Utils::SpriteUtils::loadDecodedSprite(spr_reader_, sprite_id);
      if (sprite_data.size() < 32 * 32 * 4) {
        continue;
      }

      // Calculate destination position (same as RME: width-w-1, height-h-1)
      int dest_x = (width - w - 1) * 32;
      int dest_y = (height - h - 1) * 32;

      // Blend sprite tile onto composite canvas
      Utils::ImageBlending::blendSpriteTile(sprite_data.data(),
                                            composite_rgba.data(),
                                            composite_size, dest_x, dest_y);
    }
  }

  // Create texture at full composite resolution
  auto texture = std::make_unique<Rendering::Texture>(
      composite_size, composite_size, composite_rgba.data());
  Rendering::Texture *ptr = texture.get();
  cache_[client_id] = std::move(texture);

  return ptr;
}

void ItemCompositor::clearCache() { cache_.clear(); }

} // namespace Services
} // namespace MapEditor
