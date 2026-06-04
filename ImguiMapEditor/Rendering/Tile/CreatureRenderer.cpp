#include "CreatureRenderer.h"
#include "IO/Readers/DatReaderBase.h"
#include "Rendering/Animation/ItemAnimation.h"
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
  if (!creature || !client_data_)
    return;

  const Domain::CreatureType *creature_type =
      client_data_->getCreatureType(creature->name);
  if (!creature_type || creature_type->outfit.lookType == 0) {
    emitPlaceholder(screen_x, screen_y, size, alpha);
    return;
  }

  const Domain::Outfit &outfit = creature_type->outfit;
  const IO::ClientItem *outfit_data =
      client_data_->getOutfitData(outfit.lookType);
  if (!outfit_data || outfit_data->sprite_ids.empty()) {
    emitPlaceholder(screen_x, screen_y, size, alpha);
    return;
  }

  TileColor creature_color = ground_color;
  if (creature->isSelected()) {
    creature_color.r *= 0.5f;
    creature_color.g *= 0.5f;
    creature_color.b *= 0.5f;
  }

  const int pattern_x = std::max<int>(1, outfit_data->pattern_x);
  int dir = direction % pattern_x;

  // Select sprite source and animation frame
  const std::vector<uint32_t> *sprites = &outfit_data->sprite_ids;
  int frame = 0;
  if (outfit_data->has_frame_groups && animation_frame == 0) {
    if (!outfit_data->idle_sprite_ids.empty()) {
      sprites = &outfit_data->idle_sprite_ids;
      frame = ItemAnimation::getPhaseFromFrames(
          static_cast<int>(outfit_data->idle_frames), anim_ticks.global_ms,
          outfit_data->has_animation_data ? &outfit_data->frame_durations : nullptr,
          outfit_data->total_duration);
    } else {
      frame = ItemAnimation::getPhaseFromFrames(
          static_cast<int>(outfit_data->frames), anim_ticks.global_ms,
          outfit_data->has_animation_data ? &outfit_data->frame_durations : nullptr,
          outfit_data->total_duration);
    }
  } else if (animation_frame > 0 && !outfit_data->walk_sprite_ids.empty()) {
    sprites = &outfit_data->walk_sprite_ids;
    frame = animation_frame % std::max<int>(1, outfit_data->walk_frames);
  } else {
    int f = std::max<int>(1, outfit_data->frames);
    frame = (f > 1) ? (animation_frame % f) : 0;
  }

  // Draw mount before rider
  int z_pattern = 0;
  if (outfit.lookMount > 0 && outfit_data->pattern_z > 1) {
    z_pattern = 1;
    drawMount(outfit, dir, animation_frame, screen_x, screen_y, size,
              creature_color, alpha, anim_ticks, missing_sprites);
  }

  // Draw outfit + addons (yPattern iteration)
  const int width = std::max<int>(1, outfit_data->width);
  const int height = std::max<int>(1, outfit_data->height);
  const int layers = std::max<int>(1, outfit_data->layers);
  const int pattern_y_count = std::max<int>(1, outfit_data->pattern_y);
  uint16_t addons = outfit.lookAddons;

  for (int yPattern = 0; yPattern < pattern_y_count; ++yPattern) {
    if (yPattern > 0 && !(addons & (1 << (yPattern - 1))))
      continue;

    for (int cy = 0; cy < height; ++cy) {
      for (int cx = 0; cx < width; ++cx) {
        uint32_t base_idx = Utils::SpriteUtils::getSpriteIndex(
            outfit_data, cx, cy, 0, dir, yPattern, z_pattern, frame);
        if (base_idx >= sprites->size())
          continue;

        uint32_t base_id = (*sprites)[base_idx];
        if (base_id == 0)
          continue;

        uint32_t template_id = 0;
        if (layers >= 2) {
          uint32_t t_idx = Utils::SpriteUtils::getSpriteIndex(
              outfit_data, cx, cy, 1, dir, yPattern, z_pattern, frame);
          if (t_idx < sprites->size())
            template_id = (*sprites)[t_idx];
        }

        const AtlasRegion *region =
            sprite_manager_.getCreatureSpriteService().getColorizedOutfitRegion(
                base_id, template_id,
                static_cast<uint8_t>(outfit.lookHead),
                static_cast<uint8_t>(outfit.lookBody),
                static_cast<uint8_t>(outfit.lookLegs),
                static_cast<uint8_t>(outfit.lookFeet));
        if (!region) {
          missing_sprites.push_back(base_id);
          if (template_id != 0)
            missing_sprites.push_back(template_id);
          continue;
        }

        float scale = size / TILE_SIZE;
        float dx = screen_x - static_cast<float>(cx) * size -
                   (outfit_data->has_offset ? outfit_data->offset_x : 0) * scale;
        float dy = screen_y - static_cast<float>(cy) * size -
                   (outfit_data->has_offset ? outfit_data->offset_y : 0) * scale;

        emitter_.emit(dx, dy, size, size, *region, creature_color.r,
                      creature_color.g, creature_color.b, alpha);
      }
    }
  }
}

void CreatureRenderer::drawMount(const Domain::Outfit &outfit, int dir,
                                  int animation_frame, float screen_x,
                                  float screen_y, float size,
                                  const TileColor &color, float alpha,
                                  const AnimationTicks &anim_ticks,
                                  std::vector<uint32_t> &missing_sprites) {
  const IO::ClientItem *mount_data =
      client_data_->getOutfitData(outfit.lookMount);
  if (!mount_data || mount_data->sprite_ids.empty())
    return;

  const std::vector<uint32_t> *mount_sprites = &mount_data->sprite_ids;
  int mount_dir = dir % std::max<int>(1, mount_data->pattern_x);

  // Select sprite source based on rider state (idle vs walk)
  if (animation_frame > 0 && !mount_data->walk_sprite_ids.empty()) {
    mount_sprites = &mount_data->walk_sprite_ids;
  } else if (mount_data->has_frame_groups && !mount_data->idle_sprite_ids.empty()) {
    mount_sprites = &mount_data->idle_sprite_ids;
  }

  int mount_frame = selectMountFrame(mount_data, animation_frame,
                                      anim_ticks.global_ms);

  int mw = std::max<int>(1, mount_data->width);
  int mh = std::max<int>(1, mount_data->height);
  int ml = std::max<int>(1, mount_data->layers);
  float scale = size / TILE_SIZE;

  for (int cy = 0; cy < mh; ++cy) {
    for (int cx = 0; cx < mw; ++cx) {
      uint32_t base_idx = Utils::SpriteUtils::getSpriteIndex(
          mount_data, cx, cy, 0, mount_dir, 0, 0, mount_frame);
      if (base_idx >= mount_sprites->size())
        continue;
      uint32_t base_id = (*mount_sprites)[base_idx];
      if (base_id == 0)
        continue;

      uint32_t template_id = 0;
      if (ml >= 2) {
        uint32_t t_idx = Utils::SpriteUtils::getSpriteIndex(
            mount_data, cx, cy, 1, mount_dir, 0, 0, mount_frame);
        if (t_idx < mount_sprites->size())
          template_id = (*mount_sprites)[t_idx];
      }

      const AtlasRegion *region =
          sprite_manager_.getCreatureSpriteService().getColorizedOutfitRegion(
              base_id, template_id,
              static_cast<uint8_t>(outfit.lookMountHead),
              static_cast<uint8_t>(outfit.lookMountBody),
              static_cast<uint8_t>(outfit.lookMountLegs),
              static_cast<uint8_t>(outfit.lookMountFeet));
      if (!region) {
        missing_sprites.push_back(base_id);
        if (template_id != 0)
          missing_sprites.push_back(template_id);
        continue;
      }

      float dx = screen_x - static_cast<float>(cx) * size -
                 (mount_data->has_offset ? mount_data->offset_x : 0) * scale;
      float dy = screen_y - static_cast<float>(cy) * size -
                 (mount_data->has_offset ? mount_data->offset_y : 0) * scale;

      emitter_.emit(dx, dy, size, size, *region, color.r, color.g, color.b, alpha);
    }
  }
}

int CreatureRenderer::selectMountFrame(const IO::ClientItem *mount_data,
                                        int animation_frame, int64_t global_ms) {
  if (animation_frame > 0 && !mount_data->walk_sprite_ids.empty()) {
    return animation_frame % std::max<int>(1, mount_data->walk_frames);
  }
  if (mount_data->has_frame_groups && !mount_data->idle_sprite_ids.empty()) {
    return ItemAnimation::getPhaseFromFrames(
        static_cast<int>(mount_data->idle_frames), global_ms,
        mount_data->has_animation_data ? &mount_data->frame_durations : nullptr,
        mount_data->total_duration);
  }
  if (mount_data->frames > 1) {
    return ItemAnimation::getPhaseFromFrames(
        static_cast<int>(mount_data->frames), global_ms,
        mount_data->has_animation_data ? &mount_data->frame_durations : nullptr,
        mount_data->total_duration);
  }
  return 0;
}

void CreatureRenderer::emitPlaceholder(float screen_x, float screen_y,
                                        float size, float alpha) {
  const AtlasRegion *white = sprite_manager_.getAtlasManager().getWhitePixel();
  if (white) {
    emitter_.emit(screen_x, screen_y, size, size, *white,
                  Config::Colors::INVALID_CREATURE_R,
                  Config::Colors::INVALID_CREATURE_G,
                  Config::Colors::INVALID_CREATURE_B, alpha * 0.7f);
  }
}

} // namespace Rendering
} // namespace MapEditor
