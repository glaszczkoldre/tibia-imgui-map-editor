#pragma once

#include "../Backend/SpriteBatch.h"
#include "../Backend/TileInstance.h"
#include "../Resources/AtlasManager.h"
#include <vector>

namespace MapEditor {
namespace Rendering {

/**
 * Unified sprite emission logic.
 * Handles switching between immediate rendering (SpriteBatch) and
 * deferred caching (SpriteInstance or TileInstance vectors).
 */
class SpriteEmitter {
public:
  explicit SpriteEmitter(SpriteBatch &batch) : batch_(batch) {}

  void setCache(std::vector<SpriteInstance> *cache) { cache_ = cache; }
  void setTileCache(std::vector<TileInstance> *cache) { tile_cache_ = cache; }

  bool hasTileCache() const { return tile_cache_ != nullptr; }
  bool hasCache() const { return cache_ != nullptr; }

  // Emit sprite using UV coordinates (AtlasRegion)
  // Used for immediate rendering or legacy caching
  inline void emit(float x, float y, float w, float h, const AtlasRegion &region,
                   float r, float g, float b, float a) {
    if (cache_) {
      cache_->push_back({.x = x,
                         .y = y,
                         .w = w,
                         .h = h,
                         .u_min = region.u_min,
                         .v_min = region.v_min,
                         .u_max = region.u_max,
                         .v_max = region.v_max,
                         .r = r,
                         .g = g,
                         .b = b,
                         .a = a,
                         .atlas_layer = static_cast<float>(region.atlas_index)});
    } else {
      batch_.draw(x, y, w, h, region, r, g, b, a);
    }
  }

  // Emit sprite using Sprite ID
  // Used for ID-based caching (new architecture)
  // Note: SpriteBatch doesn't support direct ID drawing yet (needs AtlasManager
  // lookup) So for immediate mode, callers might still need to look up region.
  // However, ItemRenderer logic usually handles the lookup if no tile cache is
  // present.
  inline void emitById(float x, float y, float w, float h, uint32_t sprite_id,
                       float r, float g, float b, float a) {
    if (tile_cache_) {
      tile_cache_->push_back({.x = x,
                              .y = y,
                              .w = w,
                              .h = h,
                              .sprite_id = sprite_id,
                              .flags = 0,
                              .r = r,
                              .g = g,
                              .b = b,
                              .a = a});
    }
  }

private:
  SpriteBatch &batch_;
  std::vector<SpriteInstance> *cache_ = nullptr;
  std::vector<TileInstance> *tile_cache_ = nullptr;
};

} // namespace Rendering
} // namespace MapEditor
