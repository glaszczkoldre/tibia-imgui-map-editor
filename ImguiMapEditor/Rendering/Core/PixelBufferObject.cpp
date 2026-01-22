#include "PixelBufferObject.h"
#include "Rendering/Resources/AtlasManager.h"
#include <cstring>
#include <spdlog/spdlog.h>


namespace MapEditor {
namespace Rendering {

PixelBufferObject::~PixelBufferObject() { cleanup(); }

bool PixelBufferObject::initialize() {
  if (initialized_) {
    return true;
  }

  glGenBuffers(PBO_COUNT, pbos_);

  for (size_t i = 0; i < PBO_COUNT; ++i) {
    if (pbos_[i] == 0) {
      spdlog::error("PixelBufferObject: Failed to generate PBO {}", i);
      cleanup();
      return false;
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos_[i]);

    // Check for persistent mapping support
    bool has_buffer_storage = false;
#ifdef GLAD_GL_VERSION_4_4
    has_buffer_storage = (GLAD_GL_VERSION_4_4 != 0);
#endif
#ifdef GLAD_GL_ARB_buffer_storage
    has_buffer_storage =
        has_buffer_storage || (GLAD_GL_ARB_buffer_storage != 0);
#endif
    has_buffer_storage = has_buffer_storage && (glBufferStorage != nullptr);

    if (has_buffer_storage) {
      GLbitfield flags =
          GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
      glBufferStorage(GL_PIXEL_UNPACK_BUFFER, PBO_SIZE, nullptr, flags);
      mapped_[i] = static_cast<uint8_t *>(
          glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, PBO_SIZE, flags));
    } else {
      // Fallback for GL 3.3
      glBufferData(GL_PIXEL_UNPACK_BUFFER, PBO_SIZE, nullptr, GL_STREAM_DRAW);
      mapped_[i] = nullptr; // Will map on-demand
    }
  }

  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  staged_sprites_.reserve(MAX_SPRITES_PER_UPLOAD);
  initialized_ = true;

  spdlog::info("PixelBufferObject: Initialized {} PBOs of {} KB each",
               PBO_COUNT, PBO_SIZE / 1024);
  return true;
}

void PixelBufferObject::cleanup() {
  if (!initialized_)
    return;

  for (size_t i = 0; i < PBO_COUNT; ++i) {
    if (mapped_[i]) {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos_[i]);
      glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
      mapped_[i] = nullptr;
    }
  }
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  glDeleteBuffers(PBO_COUNT, pbos_);
  for (size_t i = 0; i < PBO_COUNT; ++i) {
    pbos_[i] = 0;
    fences_[i].reset();
  }

  staged_sprites_.clear();
  write_offset_ = 0;
  initialized_ = false;
}

bool PixelBufferObject::stageSprite(uint32_t sprite_id,
                                    const uint8_t *rgba_data) {
  if (!initialized_ || !rgba_data) {
    return false;
  }

  // If this is the first sprite in a new batch, ensure the PBO is not in use
  // This prevents Write-After-Read hazards where GPU is still reading PBO
  if (staged_sprites_.empty() && mapped_[current_pbo_]) {
    // Only needed for persistent mapping where we write directly to mapped memory
    if (fences_[current_pbo_].isValid()) {
      // [UB FIX] Don't ignore timeout. Loop and handle wait failures.
      bool fence_signaled = false;
      for (int32_t i = 0; i < Config::Performance::MAX_FENCE_WAIT_RETRIES; ++i) { // Safety limit to prevent deadlock
          // 1ms timeout. GL_TIMEOUT_IGNORED could hang forever if driver bugs out.
          const GLenum waitReturn = fences_[current_pbo_].clientWait(
              GL_SYNC_FLUSH_COMMANDS_BIT, Config::Performance::FENCE_WAIT_TIMEOUT_NS);

          if (waitReturn == GL_ALREADY_SIGNALED ||
              waitReturn == GL_CONDITION_SATISFIED) {
              fence_signaled = true;
              break; // Success
          }

          if (waitReturn == GL_WAIT_FAILED) {
              spdlog::error("PixelBufferObject: Fence wait failed unexpectedly.");
              return false; // Prevent UB by not writing
          }

          // On GL_TIMEOUT_EXPIRED, the loop continues to retry.
      }

      if (!fence_signaled) {
           spdlog::error("PixelBufferObject: Fence wait timed out after {} retries.", Config::Performance::MAX_FENCE_WAIT_RETRIES);
           return false; // Safety fallback
      }
    }
  }

  if (staged_sprites_.size() >= MAX_SPRITES_PER_UPLOAD) {
    return false; // Full, need to flush
  }

  if (write_offset_ + SPRITE_BYTES > PBO_SIZE) {
    return false; // Shouldn't happen if MAX_SPRITES is correct
  }

  // Get write pointer
  uint8_t *write_ptr = nullptr;

  if (mapped_[current_pbo_]) {
    // Persistent mapping
    write_ptr = mapped_[current_pbo_] + write_offset_;
  } else {
    // Fallback: map the buffer
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos_[current_pbo_]);
    uint8_t *full_ptr = static_cast<uint8_t *>(
        glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, write_offset_, SPRITE_BYTES,
                         GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT));
    if (!full_ptr) {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
      return false;
    }
    write_ptr = full_ptr;
  }

  // Copy sprite data to PBO
  memcpy(write_ptr, rgba_data, SPRITE_BYTES);

  // If using non-persistent mapping, unmap
  if (!mapped_[current_pbo_]) {
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  }

  // Record this sprite
  staged_sprites_.emplace_back(sprite_id, write_offset_);
  write_offset_ += SPRITE_BYTES;

  return true;
}

size_t PixelBufferObject::uploadToAtlas(AtlasManager &atlas_manager) {
  if (!initialized_ || staged_sprites_.empty()) {
    return 0;
  }

  spdlog::debug("PBO::uploadToAtlas: uploading {} sprites from PBO {}",
                staged_sprites_.size(), current_pbo_);

  // Bind the PBO we just wrote to
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos_[current_pbo_]);

  size_t uploaded = 0;

  // Upload each staged sprite to atlas
  for (const auto &[sprite_id, pbo_offset] : staged_sprites_) {
    spdlog::trace("PBO: Uploading sprite {} from offset {}", sprite_id,
                  pbo_offset);

    // Add sprite to atlas, reading from PBO
    // The offset becomes the "data" pointer when PBO is bound
    const AtlasRegion *region = atlas_manager.addSpriteFromPBO(
        sprite_id, reinterpret_cast<const uint8_t *>(pbo_offset));

    if (region) {
      uploaded++;
    }
  }

  spdlog::debug("PBO::uploadToAtlas: uploaded {} sprites", uploaded);

  finalizeUpload();

  return uploaded;
}

size_t PixelBufferObject::uploadToAtlas(AtlasManager &atlas_manager,
                                        UploadCallback on_upload) {
  if (!initialized_ || staged_sprites_.empty()) {
    return 0;
  }

  spdlog::debug(
      "PBO::uploadToAtlas: uploading {} sprites from PBO {} (with callback)",
      staged_sprites_.size(), current_pbo_);

  // Bind the PBO we just wrote to
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos_[current_pbo_]);

  size_t uploaded = 0;

  // Upload each staged sprite to atlas
  for (const auto &[sprite_id, pbo_offset] : staged_sprites_) {
    spdlog::trace("PBO: Uploading sprite {} from offset {}", sprite_id,
                  pbo_offset);

    // Add sprite to atlas, reading from PBO
    const AtlasRegion *region = atlas_manager.addSpriteFromPBO(
        sprite_id, reinterpret_cast<const uint8_t *>(pbo_offset));

    if (region) {
      uploaded++;
      // Notify caller of successful upload for LUT update
      if (on_upload) {
        on_upload(sprite_id, region);
      }
    }
  }

  spdlog::debug("PBO::uploadToAtlas: uploaded {} sprites", uploaded);

  finalizeUpload();

  return uploaded;
}

void PixelBufferObject::finalizeUpload() {
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  // Place a fence so we know when GPU is done reading this PBO
  fences_[current_pbo_].reset(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));

  // Swap to other PBO for next batch
  current_pbo_ = (current_pbo_ + 1) % PBO_COUNT;
  staged_sprites_.clear();
  write_offset_ = 0;
}

} // namespace Rendering
} // namespace MapEditor
