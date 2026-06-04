#pragma once
#include "IO/Readers/DatReaderBase.h"
#include "IO/SprReader.h"
#include "Domain/ItemType.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace MapEditor {
namespace Utils {

/**
 * Sprite utility functions shared across rendering and services.
 * Provides common sprite index calculation and data loading.
 */
class SpriteUtils {
public:
  /**
   * Calculate sprite index for given pattern position (ClientItem).
   * RME formula:
   * ((((((frame%frames)*pZ+pZ)*pY+pY)*pX+pX)*layers+layer)*height+h)*width+w
   */
  static uint32_t getSpriteIndex(const IO::ClientItem *item, int w, int h,
                                 int layer, int pattern_x, int pattern_y,
                                 int pattern_z, int frame);

  /**
   * Calculate sprite index for given pattern position (ItemType).
   * Same formula as ClientItem overload.
   */
  static uint32_t getSpriteIndex(const Domain::ItemType *item, int w, int h,
                                 int layer, int pattern_x, int pattern_y,
                                 int pattern_z, int frame);

  /**
   * Load and decode a sprite from SprReader.
   * Returns decoded RGBA data, or empty vector on failure.
   */
  static std::vector<uint8_t>
  loadDecodedSprite(const std::shared_ptr<IO::SprReader> &spr_reader,
                    uint32_t sprite_id);
};

} // namespace Utils
} // namespace MapEditor
