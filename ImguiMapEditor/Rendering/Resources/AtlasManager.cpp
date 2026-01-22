#include "Rendering/Resources/AtlasManager.h"
#include <algorithm>
#include <spdlog/spdlog.h>
#include <vector>

namespace MapEditor {
namespace Rendering {

bool AtlasManager::ensureInitialized() {
  if (atlas_.isValid()) {
    return true;
  }

  // Pre-allocate 32 layers to prevent runtime expansion which causes black
  // tiles. 32 layers = 524,288 sprites capacity (16,384 sprites per layer).
  // This eliminates the texture object recreation that causes stale references.
  static constexpr int INITIAL_LAYERS = 32;

  if (!atlas_.initialize(INITIAL_LAYERS)) {
    spdlog::error("AtlasManager: Failed to initialize texture array");
    return false;
  }

  spdlog::info(
      "AtlasManager: Texture array initialized ({}x{}, {} initial layers)",
      TextureAtlas::ATLAS_SIZE, TextureAtlas::ATLAS_SIZE, INITIAL_LAYERS);
  return true;
}

const AtlasRegion *AtlasManager::addSprite(uint32_t sprite_id,
                                           const uint8_t *rgba_data) {
  // Fast check via direct lookup for common sprites
  if (sprite_id < DIRECT_LOOKUP_SIZE && direct_lookup_[sprite_id] != nullptr) {
    return direct_lookup_[sprite_id];
  }

  // Check hash map for already-added sprites
  auto it = sprite_regions_.find(sprite_id);
  if (it != sprite_regions_.end()) {
    return it->second;
  }

  if (!rgba_data) {
    spdlog::error("AtlasManager::addSprite called with null data for sprite {}",
                  sprite_id);
    return nullptr;
  }

  if (!ensureInitialized()) {
    return nullptr;
  }

  // Add to texture array
  auto region = atlas_.addSprite(rgba_data);
  if (!region.has_value()) {
    spdlog::error("Failed to add sprite {} to texture array", sprite_id);
    return nullptr;
  }

  // Store in stable deque
  region_storage_.push_back(*region);
  AtlasRegion *ptr = &region_storage_.back();

  // Store pointer in hash map
  auto [inserted_it, success] = sprite_regions_.emplace(sprite_id, ptr);
  if (!success) {
    spdlog::error("Failed to store region for sprite {}", sprite_id);
    return nullptr;
  }

  // Also store in direct lookup for O(1) access
  if (sprite_id < DIRECT_LOOKUP_SIZE) {
    direct_lookup_[sprite_id] = ptr;
  }

  return ptr;
}

const AtlasRegion *AtlasManager::addSpriteFromPBO(uint32_t sprite_id,
                                                  const uint8_t *pbo_offset) {
  // Fast check via direct lookup
  if (sprite_id < DIRECT_LOOKUP_SIZE && direct_lookup_[sprite_id] != nullptr) {
    return direct_lookup_[sprite_id];
  }

  auto it = sprite_regions_.find(sprite_id);
  if (it != sprite_regions_.end()) {
    return it->second;
  }

  if (!ensureInitialized()) {
    return nullptr;
  }

  auto region = atlas_.addSpriteFromPBO(pbo_offset);
  if (!region.has_value()) {
    spdlog::error("Failed to add sprite {} via PBO", sprite_id);
    return nullptr;
  }

  // Store in stable deque
  region_storage_.push_back(*region);
  AtlasRegion *ptr = &region_storage_.back();

  auto [inserted_it, success] = sprite_regions_.emplace(sprite_id, ptr);
  if (!success) {
    spdlog::error("Failed to store region for sprite {}", sprite_id);
    return nullptr;
  }

  if (sprite_id < DIRECT_LOOKUP_SIZE) {
    direct_lookup_[sprite_id] = ptr;
  }

  return ptr;
}

const AtlasRegion *AtlasManager::getWhitePixel() {
  if (sprite_regions_.count(WHITE_PIXEL_ID)) {
    return sprite_regions_.at(WHITE_PIXEL_ID);
  }

  // Create 32x32 white texture
  std::vector<uint8_t> white_data(32 * 32 * 4, 255);
  return addSprite(WHITE_PIXEL_ID, white_data.data());
}

const AtlasRegion *AtlasManager::getInvalidItemPlaceholder() {
  auto it = sprite_regions_.find(INVALID_PLACEHOLDER_ID);
  if (it != sprite_regions_.end()) {
    return it->second;
  }

  // Create 32x32 RGBA red square with some transparency
  constexpr size_t SIZE = TextureAtlas::SPRITE_SIZE;
  constexpr size_t PIXELS = SIZE * SIZE;
  std::vector<uint8_t> rgba(PIXELS * 4);

  for (size_t i = 0; i < PIXELS; ++i) {
    rgba[i * 4 + 0] = 255; // R - full red
    rgba[i * 4 + 1] = 64;  // G - slight tint for visibility
    rgba[i * 4 + 2] = 64;  // B - slight tint for visibility
    rgba[i * 4 + 3] = 200; // A - mostly opaque
  }

  const auto *region = addSprite(INVALID_PLACEHOLDER_ID, rgba.data());

  if (region) {
    spdlog::debug("AtlasManager: Created invalid item placeholder sprite");
  } else {
    spdlog::warn("AtlasManager: Failed to create invalid item placeholder sprite");
  }

  return region;
}

bool AtlasManager::hasSprite(uint32_t sprite_id) const {
  if (sprite_id < DIRECT_LOOKUP_SIZE) {
    return direct_lookup_[sprite_id] != nullptr;
  }
  return sprite_regions_.find(sprite_id) != sprite_regions_.end();
}

void AtlasManager::bind(uint32_t slot) const { atlas_.bind(slot); }

size_t AtlasManager::getLayerCount() const {
  return static_cast<size_t>(atlas_.getLayerCount());
}

GLuint AtlasManager::getTextureId() const { return atlas_.id(); }

void AtlasManager::forEachSprite(
    std::function<void(uint32_t, const AtlasRegion &)> callback) const {
  if (!callback)
    return;
  for (const auto &[id, region] : sprite_regions_) {
    if (region) {
      callback(id, *region);
    }
  }
}

void AtlasManager::clear() {
  atlas_ = TextureAtlas();
  region_storage_.clear();
  sprite_regions_.clear();
  std::fill(direct_lookup_.begin(), direct_lookup_.end(), nullptr);
  spdlog::debug("AtlasManager cleared");
}

} // namespace Rendering
} // namespace MapEditor
