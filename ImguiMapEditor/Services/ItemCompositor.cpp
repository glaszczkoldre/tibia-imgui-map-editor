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

  auto it = cache_.find(client_id);
  if (it != cache_.end()) {
    return it->second.get();
  }

  // Single-tile single-layer: load directly
  if (type->width == 1 && type->height == 1 && type->layers <= 1 &&
      !type->sprite_ids.empty()) {
    uint32_t sprite_id = type->sprite_ids[0];
    auto sprite_data =
        Utils::SpriteUtils::loadDecodedSprite(spr_reader_, sprite_id);
    if (sprite_data.empty()) {
      return nullptr;
    }
    auto texture =
        std::make_unique<Rendering::Texture>(32, 32, sprite_data.data());
    Rendering::Texture *ptr = texture.get();
    cache_[client_id] = std::move(texture);
    return ptr;
  }

  // Multi-tile or multi-layer: composite all parts
  const int w = std::max<int>(1, type->width);
  const int h = std::max<int>(1, type->height);
  const int layers = std::max<int>(1, type->layers);
  const int composite_size = std::max(w, h) * 32;

  std::vector<uint8_t> composite_rgba(composite_size * composite_size * 4, 0);

  // Iterate layers, then tile grid (matches map renderer order)
  for (int layer = 0; layer < layers; ++layer) {
    for (int cy = 0; cy < h; ++cy) {
      for (int cx = 0; cx < w; ++cx) {
        size_t sprite_index = Utils::SpriteUtils::getSpriteIndex(
            type, cx, cy, layer, 0, 0, 0, 0);
        if (sprite_index >= type->sprite_ids.size())
          continue;

        uint32_t sprite_id = type->sprite_ids[sprite_index];
        auto sprite_data =
            Utils::SpriteUtils::loadDecodedSprite(spr_reader_, sprite_id);
        if (sprite_data.size() < 32 * 32 * 4)
          continue;

        // Reversed position (matches OTClient/RME: anchor at bottom-right)
        int dest_x = (w - cx - 1) * 32;
        int dest_y = (h - cy - 1) * 32;

        Utils::ImageBlending::blendSpriteTile(sprite_data.data(),
                                              composite_rgba.data(),
                                              composite_size, dest_x, dest_y);
      }
    }
  }

  auto texture = std::make_unique<Rendering::Texture>(
      composite_size, composite_size, composite_rgba.data());
  Rendering::Texture *ptr = texture.get();
  cache_[client_id] = std::move(texture);

  return ptr;
}

void ItemCompositor::clearCache() { cache_.clear(); }

} // namespace Services
} // namespace MapEditor
