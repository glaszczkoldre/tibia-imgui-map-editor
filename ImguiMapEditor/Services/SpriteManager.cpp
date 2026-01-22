#include "SpriteManager.h"
#include "Core/OutfitColors.h"
#include "Rendering/Overlays/OutfitOverlay.h"
#include "Services/SecondaryClientConstants.h"
#include <glad/glad.h>
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace Services {

SpriteManager::SpriteManager(std::shared_ptr<IO::SprReader> spr_reader)
    : spr_reader_(std::move(spr_reader)) {
  // Create ItemCompositor
  item_compositor_ = std::make_unique<ItemCompositor>(spr_reader_);
  // Create CreatureSpriteService
  creature_sprite_service_ =
      std::make_unique<CreatureSpriteService>(spr_reader_, atlas_manager_);
  // Create OverlaySpriteCache for ImGui rendering
  overlay_sprite_cache_ =
      std::make_unique<Rendering::OverlaySpriteCache>(spr_reader_);
}

SpriteManager::~SpriteManager() {
  // Load queue will shutdown its threads in its destructor
}

// ========== ASYNC LOADING API ==========

bool SpriteManager::initializeAsync(size_t worker_threads) {
  if (async_loader_ && async_loader_->isInitialized()) {
    return true; // Already initialized
  }

  // Create async loader
  async_loader_ = std::make_unique<SpriteAsyncLoader>();

  // Set up loader callback (runs on worker threads)
  auto loader = [this](uint32_t sprite_id) -> std::vector<uint8_t> {
    return loadSpriteData(sprite_id);
  };

  if (!async_loader_->initialize(worker_threads, std::move(loader))) {
    return false;
  }

  // Initialize sprite atlas LUT for GPU-side ID→UV lookup
  if (!sprite_lut_.initialize()) {
    spdlog::warn("SpriteManager: Failed to initialize SpriteAtlasLUT, GPU "
                 "lookup disabled");
  }

  spdlog::info("SpriteManager: Async loading enabled with {} threads",
               worker_threads);
  return true;
}

std::vector<uint8_t> SpriteManager::loadSpriteData(uint32_t sprite_id) {
  // This runs on worker threads - must be thread-safe!

  // Check if this is a secondary client sprite (ID >= 1 million)
  // Uses SECONDARY_SPRITE_OFFSET from namespace scope

  IO::SprReader *reader = nullptr;
  uint32_t base_id = sprite_id;

  if (sprite_id >= SECONDARY_SPRITE_OFFSET) {
    // Secondary client sprite - use offset reader via provider
    IO::SprReader *secondary_reader = secondary_spr_reader_provider_
                                          ? secondary_spr_reader_provider_()
                                          : nullptr;
    if (!secondary_reader) {
      return {}; // No secondary client loaded
    }
    reader = secondary_reader;
    base_id = sprite_id - SECONDARY_SPRITE_OFFSET;
  } else {
    // Primary client sprite
    if (!spr_reader_) {
      return {};
    }
    reader = spr_reader_.get();
  }

  // Load sprite data from the appropriate reader
  auto sprite = reader->loadSprite(base_id);
  if (!sprite) {
    return {};
  }

  // Decode if compressed
  if (!sprite->is_decoded) {
    sprite->decode();
  }

  return sprite->rgba_data;
}

size_t SpriteManager::processAsyncLoads() {
  if (!async_loader_ || !async_loader_->isInitialized()) {
    return 0;
  }

  // Delegate processing to loader
  // We pass the LUT so it can be updated during upload
  size_t uploaded = async_loader_->process(atlas_manager_, &sprite_lut_);

  // Notify listeners that sprites have been loaded (for cache invalidation)
  // Only fire when sprites were uploaded AND no more pending - prevents
  // constant invalidation during bulk loading. Chunks will be regenerated
  // once all initially visible sprites have finished loading.
  if (uploaded > 0 && async_loader_->getPendingCount() == 0 && on_sprites_loaded_) {
    on_sprites_loaded_();
  }

  return uploaded;
}

void SpriteManager::syncLUTWithAtlas() {
  if (!async_loader_ || !async_loader_->isInitialized() || !sprite_lut_.isInitialized()) {
    return;
  }

  size_t count = 0;
  atlas_manager_.forEachSprite(
      [this, &count](uint32_t sprite_id, const Rendering::AtlasRegion &region) {
        // Skip special IDs that might be out of LUT bounds (MAX_SPRITES)
        // e.g. white pixel or invalid item placeholder
        if (sprite_id >= Rendering::SpriteAtlasLUT::MAX_SPRITES) {
          return;
        }

        sprite_lut_.update(sprite_id, region);
        count++;
      });

  spdlog::info("SpriteManager: Synchronized {} existing sprites to LUT", count);
}

void SpriteManager::requestSpritesAsync(
    const std::vector<uint32_t> &sprite_ids) {
  if (!async_loader_ || !async_loader_->isInitialized()) {
    return;
  }

  std::vector<uint32_t> to_request;
  to_request.reserve(sprite_ids.size());

  for (uint32_t id : sprite_ids) {
    // Skip if already in atlas or pending
    if (id == 0)
      continue;
    if (atlas_manager_.hasSprite(id))
      continue;

    // We don't check pending here because async_loader_ does it efficiently
    to_request.push_back(id);
  }

  if (!to_request.empty()) {
    async_loader_->request(to_request);
  }
}

bool SpriteManager::isLoading(uint32_t sprite_id) const {
  return async_loader_ && async_loader_->isPending(sprite_id);
}

size_t SpriteManager::getPendingLoadCount() const {
  return async_loader_ ? async_loader_->getPendingCount() : 0;
}

// ========== BATCHED RENDERING API ==========

const Rendering::AtlasRegion *
SpriteManager::getSpriteRegion(uint32_t sprite_id) {
  if (sprite_id == 0) {
    return nullptr;
  }

  // Check if already in atlas
  const auto *region = atlas_manager_.getRegion(sprite_id);
  if (region) {
    return region;
  }

  if (async_loader_ && async_loader_->isInitialized()) {
    // Async mode: queue load if not pending, return nullptr
    async_loader_->request(sprite_id);
    return nullptr; // Caller should use placeholder
  } else {
    // Sync mode: load immediately (may stall!)
    return loadSpriteToAtlas(sprite_id);
  }
}

const Rendering::AtlasRegion *
SpriteManager::preloadSprite(uint32_t sprite_id) {
  return loadSpriteToAtlas(sprite_id);
}

const Rendering::AtlasRegion *
SpriteManager::loadSpriteToAtlas(uint32_t sprite_id) {
  if (!spr_reader_) {
    return nullptr;
  }

  // Load sprite data
  auto sprite = spr_reader_->loadSprite(sprite_id);
  if (!sprite) {
    spdlog::trace("Failed to load sprite {} for atlas", sprite_id);
    return nullptr;
  }

  // Decode if compressed
  if (!sprite->is_decoded) {
    sprite->decode();
  }

  if (sprite->rgba_data.empty()) {
    spdlog::trace("Sprite {} has no decoded data", sprite_id);
    return nullptr;
  }

  // Add to atlas
  const auto *region =
      atlas_manager_.addSprite(sprite_id, sprite->rgba_data.data());

  // Update LUT for GPU lookup
  if (region && sprite_lut_.isInitialized()) {
    sprite_lut_.update(sprite_id, *region);
  }

  return region;
}

bool SpriteManager::hasSprite(uint32_t sprite_id) const {
  if (!spr_reader_ || sprite_id == 0) {
    return false;
  }
  return sprite_id <= spr_reader_->getSpriteCount();
}

void SpriteManager::clearCache() {
  atlas_manager_.clear();
  if (async_loader_) {
    async_loader_->clear();
  }
  // Delegate to sub-services
  if (item_compositor_) {
    item_compositor_->clearCache();
  }
  if (creature_sprite_service_) {
    creature_sprite_service_->clearCache();
  }
  if (overlay_sprite_cache_) {
    overlay_sprite_cache_->clearCache();
  }
  spdlog::debug("Sprite cache cleared");
}

const Rendering::AtlasRegion *SpriteManager::getInvalidItemPlaceholder() {
  return atlas_manager_.getInvalidItemPlaceholder();
}

} // namespace Services
} // namespace MapEditor
