#include "SpriteUtils.h"
#include <algorithm>

namespace MapEditor {
namespace Utils {

uint32_t SpriteUtils::getSpriteIndex(const IO::ClientItem *item, int w, int h,
                                     int layer, int pattern_x, int pattern_y,
                                     int pattern_z, int frame) {
  if (!item)
    return 0;

  const int width = std::max<int>(1, item->width);
  const int height = std::max<int>(1, item->height);
  const int layers = std::max<int>(1, item->layers);
  const int pattern_x_count = std::max<int>(1, item->pattern_x);
  const int pattern_y_count = std::max<int>(1, item->pattern_y);
  const int pattern_z_count = std::max<int>(1, item->pattern_z);
  const int frame_count = std::max<int>(1, item->frames);

  // RME sprite index formula (no modulo on pattern inputs - they are already in
  // range)
  uint32_t index = 0;
  index = (frame % frame_count);
  index = index * pattern_z_count + pattern_z;
  index = index * pattern_y_count + pattern_y;
  index = index * pattern_x_count + pattern_x;
  index = index * layers + layer;
  index = index * height + h;
  index = index * width + w;

  return index;
}

std::vector<uint8_t>
SpriteUtils::loadDecodedSprite(const std::shared_ptr<IO::SprReader> &spr_reader,
                               uint32_t sprite_id) {

  if (!spr_reader || sprite_id == 0) {
    return {};
  }

  auto sprite = spr_reader->loadSprite(sprite_id);
  if (!sprite) {
    return {};
  }

  if (!sprite->is_decoded) {
    sprite->decode();
  }

  if (sprite->rgba_data.empty()) {
    return {};
  }

  return sprite->rgba_data;
}

} // namespace Utils
} // namespace MapEditor
