#pragma once

#include "../../Core/Config.h"
#include <cstdint>
#include <functional>
#include <glad/glad.h>
#include <utility>
#include <vector>
#include "SyncHandle.h"

namespace MapEditor {
namespace Rendering {

class AtlasManager;
struct AtlasRegion;

/**
 * Double-buffered Pixel Buffer Object for async texture uploads.
 *
 * PROBLEM SOLVED:
 * glTexSubImage2D on the main thread stalls waiting for GPU.
 *
 * SOLUTION:
 * 1. Stage sprite data into a PBO (CPU-visible buffer)
 * 2. Copy from PBO to texture (DMA transfer, non-blocking)
 * 3. Double-buffer PBOs so we write to one while GPU reads the other
 *
 * This allows texture uploads to happen in the background via DMA.
 */
class PixelBufferObject {
public:
  static constexpr int SPRITE_SIZE = Config::Rendering::SPRITE_SIZE;
  static constexpr int SPRITE_BYTES = Config::Rendering::SPRITE_BYTES;
  static constexpr int MAX_SPRITES_PER_UPLOAD =
      Config::Performance::MAX_SPRITES_PER_UPLOAD;
  static constexpr size_t PBO_SIZE = Config::Performance::PBO_SIZE;
  static constexpr size_t PBO_COUNT = Config::Performance::PBO_COUNT;

  PixelBufferObject() = default;
  ~PixelBufferObject();

  // Non-copyable
  PixelBufferObject(const PixelBufferObject &) = delete;
  PixelBufferObject &operator=(const PixelBufferObject &) = delete;

  /**
   * Initialize PBOs.
   * @return true if successful
   */
  bool initialize();

  /**
   * Cleanup GPU resources.
   */
  void cleanup();

  /**
   * Stage a sprite for upload.
   * Must call uploadToAtlas() to complete the transfer.
   * @param sprite_id The sprite ID (for atlas lookup)
   * @param rgba_data Pointer to 32x32 RGBA data (4096 bytes)
   * @return true if staged, false if PBO is full
   */
  bool stageSprite(uint32_t sprite_id, const uint8_t *rgba_data);

  /**
   * Upload all staged sprites to the atlas.
   * Swaps PBOs for double-buffering.
   * @param atlas_manager The atlas to upload to
   * @return Number of sprites uploaded
   */
  size_t uploadToAtlas(AtlasManager &atlas_manager);

  /**
   * Callback for each successfully uploaded sprite.
   * Parameters: sprite_id, atlas region pointer
   */
  using UploadCallback = std::function<void(uint32_t, const AtlasRegion *)>;

  /**
   * Upload all staged sprites to the atlas with callback for each upload.
   * Use this to update LUT or other sprite tracking structures.
   * @param atlas_manager The atlas to upload to
   * @param on_upload Called for each successful upload with sprite_id and
   * region
   * @return Number of sprites uploaded
   */
  size_t uploadToAtlas(AtlasManager &atlas_manager, UploadCallback on_upload);

  /**
   * Get number of sprites currently staged.
   */
  size_t getStagedCount() const { return staged_sprites_.size(); }

  /**
   * Check if PBO is full and needs flush.
   */
  bool isFull() const {
    return staged_sprites_.size() >= MAX_SPRITES_PER_UPLOAD;
  }

private:
  GLuint pbos_[PBO_COUNT] = {};
  uint8_t *mapped_[PBO_COUNT] = {};
  size_t current_pbo_ = 0;
  SyncHandle fences_[PBO_COUNT];

  // Staged sprite IDs (paired with offset in PBO)
  std::vector<std::pair<uint32_t, size_t>> staged_sprites_;

  size_t write_offset_ = 0;
  bool initialized_ = false;

  void finalizeUpload();
};

} // namespace Rendering
} // namespace MapEditor
