#include "CreatureRenderer.h"
#include "IO/Readers/DatReaderBase.h"
#include "Rendering/Overlays/OutfitOverlay.h"
#include "Utils/SpriteUtils.h"
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace Rendering {

CreatureRenderer::CreatureRenderer(
    SpriteEmitter &emitter, Services::SpriteManager &sprites,
    const Services::ClientDataService *client_data)
    : emitter_(emitter), sprite_manager_(sprites), client_data_(client_data) {}

void CreatureRenderer::queue(const Domain::Creature *creature, float screen_x,
                             float screen_y, float size, int tile_x, int tile_y,
                             int tile_z, const AnimationTicks &anim_ticks,
                             const TileColor &ground_color, float alpha,
                             uint8_t direction, int animation_frame,
                             std::vector<uint32_t> &missing_sprites) {
  if (!creature || !client_data_) {
    return;
  }

  // Look up creature type to get outfit
  const Domain::CreatureType *creature_type =
      client_data_->getCreatureType(creature->name);
  if (!creature_type || creature_type->outfit.lookType == 0) {
    // No creature type found - render magenta placeholder
    const AtlasRegion *white_region =
        sprite_manager_.getAtlasManager().getWhitePixel();
    if (white_region) {
      emitter_.emit(screen_x, screen_y, size, size, *white_region,
                    Config::Colors::INVALID_CREATURE_R,
                    Config::Colors::INVALID_CREATURE_G,
                    Config::Colors::INVALID_CREATURE_B, alpha * 0.7f);
    }
    return;
  }

  const Domain::Outfit &outfit = creature_type->outfit;

  // Get outfit data from client
  const IO::ClientItem *outfit_data =
      client_data_->getOutfitData(outfit.lookType);
  if (!outfit_data || outfit_data->sprite_ids.empty()) {
    // No outfit data - render magenta placeholder
    const AtlasRegion *white_region =
        sprite_manager_.getAtlasManager().getWhitePixel();
    if (white_region) {
      emitter_.emit(screen_x, screen_y, size, size, *white_region,
                    Config::Colors::INVALID_CREATURE_R,
                    Config::Colors::INVALID_CREATURE_G,
                    Config::Colors::INVALID_CREATURE_B, alpha * 0.7f);
    }
    return;
  }

  // Calculate color modulation (lighting + selection)
  TileColor creature_color = ground_color;

  // Apply selection dimming
  if (creature->isSelected()) {
    creature_color.r *= 0.5f;
    creature_color.g *= 0.5f;
    creature_color.b *= 0.5f;
  }

  const int width = std::max<int>(1, outfit_data->width);
  const int height = std::max<int>(1, outfit_data->height);
  const int layers = std::max<int>(1, outfit_data->layers);
  const int pattern_x = std::max<int>(1, outfit_data->pattern_x);
  const int frames = std::max<int>(1, outfit_data->frames);

  // Map direction to pattern_x
  int dir = 2 % pattern_x; // Default to South
  if (direction < pattern_x) {
    dir = direction;
  }

  // Clamp animation frame
  int frame = (frames > 1) ? (animation_frame % frames) : 0;

  // For each tile part of a multi-tile creature
  // RME formula: draw at (screenx - cx * TileSize, screeny - cy * TileSize)
  // cx=0,cy=0 is at anchor (screenx, screeny)
  // cx increases leftward, cy increases upward
  for (int cy = 0; cy < height; ++cy) {
    for (int cx = 0; cx < width; ++cx) {
      // Get the base sprite index
      // RME uses: getHardwareID(cx, cy, dir, pattern_y, pattern_z, outfit,
      // frame) Which maps to getSpriteIndex(w, h, layer, pattern_x, pattern_y,
      // pattern_z, frame)
      uint32_t base_sprite_idx = Utils::SpriteUtils::getSpriteIndex(
          outfit_data, cx, cy, // width/height position (NOT reversed!)
          0,                   // layer 0 = base
          dir,                 // pattern_x = direction
          0,                   // pattern_y = addon (0 = base outfit)
          0,                   // pattern_z = mount (0 = no mount)
          frame);

      if (base_sprite_idx >= outfit_data->sprite_ids.size()) {
        continue;
      }

      uint32_t base_sprite_id = outfit_data->sprite_ids[base_sprite_idx];
      if (base_sprite_id == 0) {
        continue;
      }

      // Get template sprite for colorization
      uint32_t template_sprite_id = 0;
      if (layers >= 2) {
        uint32_t template_sprite_idx = Utils::SpriteUtils::getSpriteIndex(
            outfit_data, cx, cy, 1, dir, 0, 0, frame);
        if (template_sprite_idx < outfit_data->sprite_ids.size()) {
          template_sprite_id = outfit_data->sprite_ids[template_sprite_idx];
        }
      }

      // Get the colorized outfit sprite region from the atlas
      const AtlasRegion *region =
          sprite_manager_.getCreatureSpriteService().getColorizedOutfitRegion(
              base_sprite_id, template_sprite_id,
              static_cast<uint8_t>(outfit.lookHead),
              static_cast<uint8_t>(outfit.lookBody),
              static_cast<uint8_t>(outfit.lookLegs),
              static_cast<uint8_t>(outfit.lookFeet));

      if (!region) {
        // Queue for async load
        missing_sprites.push_back(base_sprite_id);
        if (template_sprite_id != 0) {
          missing_sprites.push_back(template_sprite_id);
        }
        continue;
      }

      // Calculate draw position - SIMPLE RME FORMULA + DISPLACEMENT
      // screenx - cx * TileSize - offsetX, screeny - cy * TileSize - offsetY
      // The displacement offset centers creatures on the tile (OTClient
      // parity). This puts cx=0,cy=0 at anchor, and expands left/up for larger
      // creatures
      float displacement_x = outfit_data->has_offset
                                 ? static_cast<float>(outfit_data->offset_x)
                                 : 0.0f;
      float displacement_y = outfit_data->has_offset
                                 ? static_cast<float>(outfit_data->offset_y)
                                 : 0.0f;
      // Scale displacement by (size / TILE_SIZE) for zoom
      float scale = size / TILE_SIZE;
      float draw_x =
          screen_x - static_cast<float>(cx) * size - displacement_x * scale;
      float draw_y =
          screen_y - static_cast<float>(cy) * size - displacement_y * scale;

      // Queue to sprite batch
      emitter_.emit(draw_x, draw_y, size, size, *region, creature_color.r,
                    creature_color.g, creature_color.b, alpha);
    }
  }
}

} // namespace Rendering
} // namespace MapEditor
