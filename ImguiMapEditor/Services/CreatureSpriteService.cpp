#include "CreatureSpriteService.h"
#include "Core/Config.h"
#include "Core/OutfitColors.h"
#include "IO/SprReader.h"
#include "Utils/ImageBlending.h"
#include "Utils/SpriteUtils.h"
#include <algorithm>

namespace MapEditor {
namespace Services {

CreatureSpriteService::CreatureSpriteService(
    std::shared_ptr<IO::SprReader> spr_reader,
    Rendering::AtlasManager &atlas_manager)
    : spr_reader_(std::move(spr_reader)), atlas_manager_(atlas_manager) {}

uint64_t CreatureSpriteService::makeOutfitCacheKey(uint32_t base_id,
                                                   uint32_t template_id,
                                                   uint8_t head, uint8_t body,
                                                   uint8_t legs, uint8_t feet) {
  // Key bit layout: base(20b) | template(20b) | head(6b) | body(6b) | legs(6b)
  // | feet(6b)
  return (static_cast<uint64_t>(base_id & 0xFFFFF) << 44) |
         (static_cast<uint64_t>(template_id & 0xFFFFF) << 24) |
         (static_cast<uint64_t>(head & 0x3F) << 18) |
         (static_cast<uint64_t>(body & 0x3F) << 12) |
         (static_cast<uint64_t>(legs & 0x3F) << 6) |
         (static_cast<uint64_t>(feet & 0x3F));
}

std::vector<uint8_t> CreatureSpriteService::colorizeSprite(
    uint32_t base_sprite_id, uint32_t template_sprite_id, uint8_t head,
    uint8_t body, uint8_t legs, uint8_t feet) {

  if (base_sprite_id == 0 || !spr_reader_) {
    return {};
  }

  // Load base sprite
  auto base_sprite = spr_reader_->loadSprite(base_sprite_id);
  if (!base_sprite) {
    return {};
  }
  if (!base_sprite->is_decoded) {
    base_sprite->decode();
  }
  if (base_sprite->rgba_data.empty()) {
    return {};
  }

  // Copy base sprite data
  std::vector<uint8_t> colorized_data = base_sprite->rgba_data;

  // Load and apply template if present
  if (template_sprite_id != 0 && template_sprite_id != base_sprite_id) {
    auto template_sprite = spr_reader_->loadSprite(template_sprite_id);
    if (template_sprite) {
      if (!template_sprite->is_decoded) {
        template_sprite->decode();
      }
      if (!template_sprite->rgba_data.empty() &&
          template_sprite->rgba_data.size() >= 32 * 32 * 4) {
        Domain::Outfit outfit;
        outfit.lookHead = head;
        outfit.lookBody = body;
        outfit.lookLegs = legs;
        outfit.lookFeet = feet;

        Rendering::OutfitColorizer::colorize(colorized_data.data(),
                                             template_sprite->rgba_data.data(),
                                             32 * 32, outfit);
      }
    }
  }

  return colorized_data;
}

Rendering::Texture *CreatureSpriteService::getColorizedOutfitSprite(
    uint32_t base_sprite_id, uint32_t template_sprite_id, uint8_t head,
    uint8_t body, uint8_t legs, uint8_t feet) {

  uint64_t cache_key = makeOutfitCacheKey(base_sprite_id, template_sprite_id,
                                          head, body, legs, feet);

  auto it = colorized_outfit_cache_.find(cache_key);
  if (it != colorized_outfit_cache_.end()) {
    return it->second.get();
  }

  auto colorized_data = colorizeSprite(base_sprite_id, template_sprite_id, head,
                                       body, legs, feet);
  if (colorized_data.empty()) {
    return nullptr;
  }

  auto texture =
      std::make_unique<Rendering::Texture>(32, 32, colorized_data.data());
  Rendering::Texture *ptr = texture.get();
  colorized_outfit_cache_[cache_key] = std::move(texture);

  return ptr;
}

const Rendering::AtlasRegion *CreatureSpriteService::getColorizedOutfitRegion(
    uint32_t base_sprite_id, uint32_t template_sprite_id, uint8_t head,
    uint8_t body, uint8_t legs, uint8_t feet) {

  uint64_t cache_key = makeOutfitCacheKey(base_sprite_id, template_sprite_id,
                                          head, body, legs, feet);

  auto it = colorized_outfit_region_cache_.find(cache_key);
  if (it != colorized_outfit_region_cache_.end()) {
    return it->second;
  }

  auto colorized_data = colorizeSprite(base_sprite_id, template_sprite_id, head,
                                       body, legs, feet);
  if (colorized_data.empty()) {
    return nullptr;
  }

  uint32_t atlas_sprite_id =
      Config::Rendering::COLORIZED_OUTFIT_OFFSET + next_colorized_id_++;

  const Rendering::AtlasRegion *region =
      atlas_manager_.addSprite(atlas_sprite_id, colorized_data.data());
  if (region) {
    colorized_outfit_region_cache_[cache_key] = region;
  }

  return region;
}

Rendering::Texture *CreatureSpriteService::getCompositedCreatureTexture(
    const IO::ClientItem *outfit_data, uint8_t head, uint8_t body, uint8_t legs,
    uint8_t feet) {

  if (!outfit_data || outfit_data->sprite_ids.empty() || !spr_reader_) {
    return nullptr;
  }

  // Create cache key from outfit ID + colors
  uint64_t cache_key = (static_cast<uint64_t>(outfit_data->id) << 24) |
                       (static_cast<uint64_t>(head) << 18) |
                       (static_cast<uint64_t>(body) << 12) |
                       (static_cast<uint64_t>(legs) << 6) |
                       static_cast<uint64_t>(feet);

  auto it = composited_creature_cache_.find(cache_key);
  if (it != composited_creature_cache_.end()) {
    // LRU: Move accessed entry to front using splice (avoids reallocation)
    auto lru_it = composited_lru_map_.find(cache_key);
    composited_lru_order_.splice(composited_lru_order_.begin(),
                                 composited_lru_order_, lru_it->second);
    return it->second.get();
  }

  const int width = std::max<int>(1, outfit_data->width);
  const int height = std::max<int>(1, outfit_data->height);
  const int layers = std::max<int>(1, outfit_data->layers);
  const int pattern_x = std::max<int>(1, outfit_data->pattern_x);

  const int composite_size = std::max(width, height) * 32;
  std::vector<uint8_t> composite_rgba(composite_size * composite_size * 4, 0);

  // Gray background
  constexpr uint8_t BG_SHADE = 48;
  for (size_t i = 0; i < composite_rgba.size(); i += 4) {
    composite_rgba[i + 0] = BG_SHADE;
    composite_rgba[i + 1] = BG_SHADE;
    composite_rgba[i + 2] = BG_SHADE;
    composite_rgba[i + 3] = 255;
  }

  // Direction 2 = south facing
  int dir = 2 % pattern_x;
  int addon = 0, mount = 0, frame = 0;

  // Composite each tile
  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
      uint32_t base_idx = Utils::SpriteUtils::getSpriteIndex(
          outfit_data, w, h, 0, dir, addon, mount, frame);
      if (base_idx >= outfit_data->sprite_ids.size())
        continue;

      uint32_t base_sprite_id = outfit_data->sprite_ids[base_idx];
      if (base_sprite_id == 0)
        continue;

      // Determine template sprite ID for colorization
      uint32_t template_sprite_id = 0;
      if (layers >= 2) {
        uint32_t template_idx = Utils::SpriteUtils::getSpriteIndex(
            outfit_data, w, h, 1, dir, addon, mount, frame);
        if (template_idx < outfit_data->sprite_ids.size()) {
          template_sprite_id = outfit_data->sprite_ids[template_idx];
        }
      }

      // Get colorized sprite data using shared helper
      std::vector<uint8_t> tile_data = colorizeSprite(
          base_sprite_id, template_sprite_id, head, body, legs, feet);
      if (tile_data.size() < 32 * 32 * 4) {
        continue;
      }

      // Center creature in square canvas
      int offset_x = (composite_size - width * 32) / 2;
      int offset_y = (composite_size - height * 32) / 2;
      int dest_x = offset_x + (width - w - 1) * 32;
      int dest_y = offset_y + (height - h - 1) * 32;

      // Blend tile onto composite using ImageBlending utility
      Utils::ImageBlending::blendSpriteTile(tile_data.data(),
                                            composite_rgba.data(),
                                            composite_size, dest_x, dest_y);
    }
  }

  auto texture = std::make_unique<Rendering::Texture>(
      composite_size, composite_size, composite_rgba.data());
  Rendering::Texture *result = texture.get();
  composited_creature_cache_[cache_key] = std::move(texture);

  // LRU: Add new entry to front
  composited_lru_order_.push_front(cache_key);
  composited_lru_map_[cache_key] = composited_lru_order_.begin();

  // LRU: Evict oldest entries if cache exceeds limit
  while (composited_creature_cache_.size() > MAX_COMPOSITED_CACHE_SIZE) {
    uint64_t oldest_key = composited_lru_order_.back();
    composited_lru_order_.pop_back();
    composited_lru_map_.erase(oldest_key);
    composited_creature_cache_.erase(oldest_key);
  }

  return result;
}

void CreatureSpriteService::clearCache() {
  colorized_outfit_cache_.clear();
  colorized_outfit_region_cache_.clear();
  composited_creature_cache_.clear();
  composited_lru_order_.clear();
  composited_lru_map_.clear();
}

size_t CreatureSpriteService::getCacheSize() const {
  return colorized_outfit_cache_.size() +
         colorized_outfit_region_cache_.size() +
         composited_creature_cache_.size();
}

} // namespace Services
} // namespace MapEditor
