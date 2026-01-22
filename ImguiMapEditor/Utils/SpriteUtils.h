#pragma once
#include "IO/Readers/DatReaderBase.h"
#include "IO/SprReader.h"
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
   * Calculate sprite index for given pattern position.
   * RME formula:
   * ((((((frame%frames)*pZ+pZ)*pY+pY)*pX+pX)*layers+layer)*height+h)*width+w
   * @param item ClientItem with sprite dimensions
   * @param w Width index (0 to width-1)
   * @param h Height index (0 to height-1)
   * @param layer Layer index (0 for base, 1 for template)
   * @param pattern_x Pattern X index
   * @param pattern_y Pattern Y index
   * @param pattern_z Pattern Z index
   * @param frame Animation frame
   * @return Sprite index in sprite_ids array
   */
  static uint32_t getSpriteIndex(const IO::ClientItem *item, int w, int h,
                                 int layer, int pattern_x, int pattern_y,
                                 int pattern_z, int frame);

  /**
   * Load and decode a sprite from SprReader.
   * Returns decoded RGBA data, or empty vector on failure.
   * @param spr_reader SprReader to load from
   * @param sprite_id Sprite ID to load
   * @return Decoded RGBA pixel data (32x32x4 bytes), empty on failure
   */
  static std::vector<uint8_t>
  loadDecodedSprite(const std::shared_ptr<IO::SprReader> &spr_reader,
                    uint32_t sprite_id);
};

} // namespace Utils
} // namespace MapEditor
