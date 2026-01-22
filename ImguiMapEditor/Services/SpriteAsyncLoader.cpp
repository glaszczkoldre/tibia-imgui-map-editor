#include "SpriteAsyncLoader.h"
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace Services {

SpriteAsyncLoader::~SpriteAsyncLoader() {
  // Queue handles its own shutdown
}

bool SpriteAsyncLoader::initialize(size_t worker_threads, DataLoader loader) {
  if (initialized_) {
    return true;
  }

  // Create load queue
  load_queue_ = std::make_unique<SpriteLoadQueue>(worker_threads);
  load_queue_->setLoader(std::move(loader));

  // Create PBO
  pbo_ = std::make_unique<Rendering::PixelBufferObject>();
  if (!pbo_->initialize()) {
    spdlog::error("SpriteAsyncLoader: Failed to initialize PBO");
    pbo_.reset();
    load_queue_.reset();
    return false;
  }

  initialized_ = true;
  spdlog::info("SpriteAsyncLoader: Initialized with {} threads", worker_threads);
  return true;
}

size_t SpriteAsyncLoader::process(Rendering::AtlasManager &atlas_manager,
                                  Rendering::SpriteAtlasLUT *sprite_lut) {
  if (!initialized_ || !load_queue_ || !pbo_) {
    return 0;
  }

  // Get completed loads
  auto completed = load_queue_->getCompletedSprites();

  // Callback for PBO upload to update LUT
  auto upload_callback = [&](uint32_t sprite_id,
                             const Rendering::AtlasRegion *region) {
    if (region && sprite_lut && sprite_lut->isInitialized()) {
      sprite_lut->update(sprite_id, *region);
    }
  };

  size_t uploaded = 0;
  auto flush_pbo = [&]() {
    uploaded += pbo_->uploadToAtlas(atlas_manager, upload_callback);
  };

  // Logic Fix: Don't drop sprites if PBO is full
  for (auto &result : completed) {
    if (result.success && !result.rgba_data.empty()) {
      // Try to stage the sprite
      bool staged = pbo_->stageSprite(result.sprite_id, result.rgba_data.data());

      if (!staged) {
        // PBO was full, so flush the existing batch.
        flush_pbo();

        // Retry staging the sprite into the now-empty PBO.
        staged = pbo_->stageSprite(result.sprite_id, result.rgba_data.data());

        if (!staged) {
          // This should only happen if a single sprite is too large for the PBO.
          spdlog::error("SpriteAsyncLoader: Failed to stage sprite {} after flush, dropping!", result.sprite_id);
        }
      }

      // If a sprite was successfully staged (either initially or on retry) and the PBO is now full, flush it.
      if (staged && pbo_->isFull()) {
        flush_pbo();
      }
    }

    // Remove from pending loads regardless of outcome (success, failure, or drop)
    pending_loads_.erase(result.sprite_id);
  }

  // Flush remaining
  if (pbo_->getStagedCount() > 0) {
    flush_pbo();
  }

  return uploaded;
}

void SpriteAsyncLoader::request(const std::vector<uint32_t> &sprite_ids) {
  if (!initialized_ || !load_queue_) {
    return;
  }

  std::vector<uint32_t> to_request;
  to_request.reserve(sprite_ids.size());

  for (uint32_t id : sprite_ids) {
    if (id == 0) continue;
    if (pending_loads_.insert(id).second) {
      to_request.push_back(id);
    }
  }

  if (!to_request.empty()) {
    load_queue_->requestSprites(to_request);
  }
}

void SpriteAsyncLoader::request(uint32_t sprite_id) {
  if (!initialized_ || !load_queue_ || sprite_id == 0) {
    return;
  }

  if (pending_loads_.insert(sprite_id).second) {
    load_queue_->requestSprite(sprite_id);
  }
}

bool SpriteAsyncLoader::isPending(uint32_t sprite_id) const {
  return pending_loads_.count(sprite_id) > 0;
}

size_t SpriteAsyncLoader::getPendingCount() const {
  return pending_loads_.size();
}

void SpriteAsyncLoader::clear() {
  pending_loads_.clear();
  if (load_queue_) {
    load_queue_->clearPending();
  }
}

} // namespace Services
} // namespace MapEditor
