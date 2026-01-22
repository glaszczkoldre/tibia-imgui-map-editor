#include "ItemRenderer.h"
#include "../../Core/Config.h"
#include "../ColorFilter.h"
#include "Services/ClientDataService.h"
#include "Services/SecondaryClientConstants.h"
#include "Services/SecondaryClientData.h"
#include "Services/SpriteManager.h"
#include <algorithm>
#include <cmath>

namespace MapEditor {
namespace Rendering {

ItemRenderer::ItemRenderer(SpriteEmitter &emitter,
                           Services::SpriteManager &sprites,
                           const Services::ClientDataService *client_data)
    : emitter_(emitter), sprite_manager_(sprites), client_data_(client_data) {}

void ItemRenderer::queueInvalidPlaceholder(float screen_x, float screen_y,
                                           float size, float alpha, float r,
                                           float g, float b) {
  const auto *region = sprite_manager_.getAtlasManager().getWhitePixel();
  if (!region)
    return;

  emitter_.emit(std::round(screen_x), std::round(screen_y), size, size, *region,
                r, g, b, alpha * 0.7f);
}

void ItemRenderer::queueWithColor(
    const Domain::ItemType *item_type, float screen_x, float screen_y,
    float size, int tile_x, int tile_y, int tile_z,
    const AnimationTicks &anim_ticks, std::vector<uint32_t> &missing_sprites,
    float r, float g, float b, float alpha, float *accumulated_elevation,
    const Domain::Item *item_inst, uint32_t sprite_id_offset,
    bool tile_has_hook_south, bool tile_has_hook_east) {
  if (!item_type || item_type->sprite_ids.empty())
    return;

  // Calculate scaling factor for zoom
  float scale = size / TILE_SIZE;

  // Start with screen coordinates
  float adjusted_x = screen_x;
  float adjusted_y = screen_y;

  // Apply accumulated elevation from previous items in stack
  if (accumulated_elevation) {
    adjusted_x -= *accumulated_elevation;
    adjusted_y -= *accumulated_elevation;
  }

  // Apply draw offset / displacement
  if (item_type->draw_offset_x != 0 || item_type->draw_offset_y != 0) {
    adjusted_x -= item_type->draw_offset_x * scale;
    adjusted_y -= item_type->draw_offset_y * scale;
  }

  // Update accumulated elevation for next item
  if (accumulated_elevation && item_type->hasElevation()) {
    *accumulated_elevation += static_cast<float>(item_type->elevation) * scale;
  }

  // Check if patterns are used
  int pat_x_count = std::max<int>(1, item_type->pattern_x);
  int pat_y_count = std::max<int>(1, item_type->pattern_y);
  int pat_z_count = std::max<int>(1, item_type->pattern_z);
  bool has_patterns = (pat_x_count > 1 || pat_y_count > 1 || pat_z_count > 1);

  // Stackable/Fluid Subtype Logic
  int subtype_index = -1;
  if (item_inst) {
    if (item_type->is_stackable) {
      int count = item_inst->getSubtype();
      if (count <= 1)
        subtype_index = 0;
      else if (count <= 2)
        subtype_index = 1;
      else if (count <= 3)
        subtype_index = 2;
      else if (count <= 4)
        subtype_index = 3;
      else if (count < 10)
        subtype_index = 4;
      else if (count < 25)
        subtype_index = 5;
      else if (count < 50)
        subtype_index = 6;
      else
        subtype_index = 7;
    }
  }

  // FAST PATH: Simple single-sprite items
  bool can_use_subtype =
      (subtype_index >= 0 && item_type->width == 1 && item_type->height == 1);

  if ((item_type->width == 1 && item_type->height == 1 &&
       item_type->layers == 1 && item_type->frames == 1 && !has_patterns) ||
      can_use_subtype) {

    uint32_t sprite_id = 0;
    if (can_use_subtype &&
        subtype_index < static_cast<int>(item_type->sprite_ids.size())) {
      sprite_id = item_type->sprite_ids[subtype_index];
    } else if (!item_type->sprite_ids.empty()) {
      sprite_id = item_type->sprite_ids[0];
    }

    if (sprite_id > 0 && sprite_id_offset > 0) {
      sprite_id += sprite_id_offset;
    }

    if (sprite_id > 0) {
      // BUG #2 FIX: Ensure sprite is loaded before caching
      // getSpriteRegion triggers async load if not present and returns null
      // We only cache the ID if the sprite is actually in the atlas
      const AtlasRegion *region = sprite_manager_.getSpriteRegion(sprite_id);

      if (region) {
        // Sprite is loaded - use ID-based rendering
        if (emitter_.hasTileCache()) {
          emitter_.emitById(std::round(adjusted_x), std::round(adjusted_y),
                            size, size, sprite_id, r, g, b, alpha);
        } else {
          // Direct rendering (non-cached path) or UV-based caching
          emitter_.emit(std::round(adjusted_x), std::round(adjusted_y), size,
                        size, *region, r, g, b, alpha);
        }
      } else {
        // Sprite not yet loaded - track as missing for async loader
        missing_sprites.push_back(sprite_id);
      }
    }
    return;
  }

  // SLOW PATH: Multi-tile, animated, or multi-layer items
  int width = std::max<int>(1, item_type->width);
  int height = std::max<int>(1, item_type->height);
  int layers = std::max<int>(1, item_type->layers);
  int pat_x = std::max<int>(1, item_type->pattern_x);
  int pat_y = std::max<int>(1, item_type->pattern_y);
  int pat_z = std::max<int>(1, item_type->pattern_z);
  int frames = std::max<int>(1, item_type->frames);

  // Initialize patterns from tile position (like RME does)
  int pattern_x = tile_x % pat_x;
  int pattern_y = tile_y % pat_y;
  int pattern_z = tile_z % pat_z;

  if (item_type->is_hangable) {
    // RME checks TILE properties (wall items on the tile), not item type flags.
    // pattern_x determines which sprite variant:
    //   0 = free-standing (no wall to hang on)
    //   1 = hanging on south wall (HOOK_SOUTH)
    //   2 = hanging on east wall (HOOK_EAST)
    // NOTE: pattern_y and pattern_z are PRESERVED from the position-based calc.
    if (tile_has_hook_south)
      pattern_x = 1;
    else if (tile_has_hook_east)
      pattern_x = 2;
    else
      pattern_x = 0;
  } else if ((item_type->isFluidContainer() || item_type->isSplash()) &&
             item_inst) {
    int fluid_subtype = item_inst->getSubtype();
    pattern_x = (fluid_subtype % 4) % pat_x;
    pattern_y = (fluid_subtype / 4) % pat_y;
    pattern_z = 0;
  }

  int frame = 0;
  if (frames > 1) {
    frame = anim_ticks.tick_500ms % frames;
  }

  float draw_adjusted_x = std::round(adjusted_x);
  float draw_adjusted_y = std::round(adjusted_y);

  // Optimization: Pre-calculate constant pattern offset to avoid recomputing in inner loops
  // Formula structure: [Frame -> Pattern Z -> Pattern Y -> Pattern X] -> Layers -> Height -> Width
  // Note: 'frame' is already computed as (tick % frames) above, so frame % frames is redundant but safe
  const size_t pattern_offset = static_cast<size_t>(
      ((((frame % frames) * pat_z + pattern_z) * pat_y + pattern_y) * pat_x + pattern_x) * layers);

  // Optimization: Cache sprite vector reference and size to avoid indirection in hot loop
  const auto& sprite_ids = item_type->sprite_ids;
  const size_t num_sprites = sprite_ids.size();

  if (num_sprites == 0) return;

  // Optimization: Generic render loop to avoid code duplication.
  // This helper handles coordinate calculation and emission logic, while delegating
  // the sprite ID lookup strategy to the caller.
  auto renderGrid = [&](auto getSpriteInfoFunc) {
    for (int cy = 0; cy < height; cy++) {
      float draw_y = draw_adjusted_y - cy * size;
      for (int cx = 0; cx < width; cx++) {
        for (int layer = 0; layer < layers; layer++) {
          float draw_x = draw_adjusted_x - cx * size;

          // Delegate sprite info retrieval (ID and AtlasRegion)
          auto [sprite_id, region] = getSpriteInfoFunc(cx, cy, layer);

          if (sprite_id > 0) {
            if (region) {
              if (emitter_.hasTileCache()) {
                emitter_.emitById(draw_x, draw_y, size, size, sprite_id, r, g,
                                  b, alpha);
              } else {
                emitter_.emit(draw_x, draw_y, size, size, *region, r, g, b,
                              alpha);
              }
            } else {
              missing_sprites.push_back(sprite_id);
            }
          }
        }
      }
    }
  };

  // Optimization: Split loop for single-sprite case (90% of static items)
  // to avoid modulo operations and repeated lookups in the hot path.
  if (num_sprites == 1) {
    // SINGLE SPRITE PATH: Always index 0
    uint32_t sprite_id = sprite_ids[0];
    if (sprite_id > 0 && sprite_id_offset > 0) {
      sprite_id += sprite_id_offset;
    }

    if (sprite_id > 0) {
      // Pre-fetch region once outside the loop
      const auto *region = sprite_manager_.getSpriteRegion(sprite_id);

      if (region) {
        // FAST PATH: Region is available, emit directly without redundant checks
        renderGrid([&](int, int, int) { return std::pair{sprite_id, region}; });
      } else {
        // FALLBACK: Region missing, record as missing for all tiles
        size_t total_iterations = static_cast<size_t>(width) * height * layers;
        missing_sprites.insert(missing_sprites.end(), total_iterations, sprite_id);
      }
    }
    return;
  }

  // MULTI-SPRITE PATH: Full logic with modulo and dynamic lookup
  renderGrid([&](int cx, int cy, int layer) -> std::pair<uint32_t, const AtlasRegion*> {
    // Calculate final index using pre-calculated pattern offset
    size_t sprite_index = ((pattern_offset + layer) * height + cy) * width + cx;

    if (sprite_index >= num_sprites) {
      sprite_index %= num_sprites;
    }

    uint32_t sprite_id = sprite_ids[sprite_index];
    if (sprite_id > 0 && sprite_id_offset > 0) {
      sprite_id += sprite_id_offset;
    }

    // Dynamic lookup per sprite
    const AtlasRegion *region = (sprite_id > 0) ? sprite_manager_.getSpriteRegion(sprite_id) : nullptr;
    return {sprite_id, region};
  });
}

} // namespace Rendering
} // namespace MapEditor
