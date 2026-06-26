#include "ItemDefinitionResolver.h"

#include <algorithm>
#include <format>
#include <unordered_map>

namespace MapEditor::Services {
namespace {

using Domain::ItemFlag;
using Domain::ItemGroup;

void setFlag(Domain::ItemType &item, ItemFlag flag, bool enabled) {
  if (!enabled) {
    return;
  }
  item.flags = item.flags | flag;
}

ItemGroup datGroup(const IO::ClientItem &dat) {
  if (dat.is_ground) {
    return ItemGroup::Ground;
  }
  if (dat.is_container) {
    return ItemGroup::Container;
  }
  if (dat.is_fluid_container) {
    return ItemGroup::Fluid;
  }
  if (dat.is_fluid) {
    return ItemGroup::Splash;
  }
  return ItemGroup::None;
}

} // namespace

std::vector<Domain::ItemType> ItemDefinitionResolver::resolve(
    Domain::ItemDataSource source,
    const std::vector<Domain::ItemType> &server_items,
    const IO::DatResult &dat_result) {
  std::unordered_map<uint16_t, const IO::ClientItem *> dat_by_client_id;
  dat_by_client_id.reserve(dat_result.items.size());
  for (const auto &dat : dat_result.items) {
    dat_by_client_id[dat.id] = &dat;
  }

  std::vector<Domain::ItemType> resolved;

  if (source == Domain::ItemDataSource::DAT) {
    resolved.reserve(dat_result.items.size());
    for (const auto &dat : dat_result.items) {
      resolved.push_back(createDatOnlyItem(dat));
    }
    normalizeResolvedItemTypes(resolved);
    return resolved;
  }

  resolved.reserve(server_items.size());
  for (const auto &server_item : server_items) {
    Domain::ItemType item = server_item;
    const auto dat_it = dat_by_client_id.find(server_item.client_id);
    if (dat_it != dat_by_client_id.end()) {
      applyDatFragment(item, *dat_it->second, false);
    }
    normalizeResolvedItemType(item);
    resolved.push_back(std::move(item));
  }

  return resolved;
}

void ItemDefinitionResolver::normalizeResolvedItemTypes(
    std::vector<Domain::ItemType> &items) {
  for (auto &item : items) {
    normalizeResolvedItemType(item);
  }
}

Domain::ItemType
ItemDefinitionResolver::createDatOnlyItem(const IO::ClientItem &dat) {
  Domain::ItemType item;
  item.server_id = dat.id;
  item.client_id = dat.id;
  item.name = std::format("Item {}", dat.id);
  applyDatFragment(item, dat, true);
  normalizeResolvedItemType(item);
  return item;
}

void ItemDefinitionResolver::applyDatFragment(Domain::ItemType &item,
                                              const IO::ClientItem &dat,
                                              bool datOwnsClassification) {
  item.width = dat.width;
  item.height = dat.height;
  item.layers = dat.layers;
  item.pattern_x = dat.pattern_x;
  item.pattern_y = dat.pattern_y;
  item.pattern_z = dat.pattern_z;
  item.frames = dat.frames;
  item.sprite_ids = dat.sprite_ids;

  item.idle_sprite_ids = dat.idle_sprite_ids;
  item.walk_sprite_ids = dat.walk_sprite_ids;
  item.idle_frames = dat.idle_frames;
  item.walk_frames = dat.walk_frames;
  item.has_frame_groups = dat.has_frame_groups;

  const ItemGroup group_from_dat = datGroup(dat);
  if (datOwnsClassification || item.group == ItemGroup::None) {
    item.group = group_from_dat;
  }

  if (dat.is_ground && dat.ground_speed > 0 &&
      (datOwnsClassification || item.speed == 0)) {
    item.speed = dat.ground_speed;
  }
  item.ground_speed = static_cast<uint8_t>(
      std::min<uint16_t>(dat.ground_speed, uint16_t{255}));

  if (dat.has_light) {
    item.light_level = static_cast<uint8_t>(
        std::min<uint16_t>(dat.light_level, uint16_t{255}));
    item.light_color = static_cast<uint8_t>(
        std::min<uint16_t>(dat.light_color, uint16_t{255}));
  }

  item.draw_offset_x = dat.offset_x;
  item.draw_offset_y = dat.offset_y;
  item.is_translucent = dat.is_translucent;
  item.elevation = dat.elevation;

  if (dat.has_minimap_color) {
    item.minimap_color = dat.minimap_color;
  }

  item.is_on_bottom = dat.is_on_bottom;
  item.is_on_top = dat.is_on_top;
  item.is_dont_hide = dat.dont_hide;
  item.blocks_projectile = dat.blocks_missiles;
  item.is_fluid_container =
      item.group == ItemGroup::Fluid || dat.is_fluid_container;

  item.animate_always = dat.animate_always;
  item.animation_mode = dat.animation_mode;
  item.loop_count = dat.loop_count;
  item.start_frame = dat.start_frame;
  item.frame_durations = dat.frame_durations;
  item.total_duration = dat.total_duration;
  item.is_lying_object = dat.is_lying_object;

  if (dat.is_writable) {
    item.can_write_text = true;
    item.can_read_text = true;
    item.maxTextLen = dat.max_text_length;
  }

  setFlag(item, ItemFlag::Unpassable, dat.is_unpassable);
  setFlag(item, ItemFlag::BlockMissiles, dat.blocks_missiles);
  setFlag(item, ItemFlag::BlockPathfinder, dat.blocks_pathfinder);
  setFlag(item, ItemFlag::Pickupable, dat.is_pickupable);
  setFlag(item, ItemFlag::Stackable, dat.is_stackable);
  setFlag(item, ItemFlag::Useable, dat.is_useable || dat.usable);
  setFlag(item, ItemFlag::Hangable, dat.is_hangable);
  setFlag(item, ItemFlag::HookEast, dat.is_horizontal);
  setFlag(item, ItemFlag::HookSouth, dat.is_vertical);
  setFlag(item, ItemFlag::Rotatable, dat.is_rotatable);
  setFlag(item, ItemFlag::HasElevation, dat.has_elevation && dat.elevation > 0);
  setFlag(item, ItemFlag::IgnoreLook, dat.ignore_look);
  setFlag(item, ItemFlag::FullTile, dat.full_ground);
  setFlag(item, ItemFlag::Animation,
          dat.animate_always || dat.has_animation_data || dat.frames > 1);
  setFlag(item, ItemFlag::ClientCharges, dat.has_default_action);
  setFlag(item, ItemFlag::ForceUse, dat.usable);

  if (dat.floor_change) {
    item.floor_change = true;
  }
}

void ItemDefinitionResolver::normalizeResolvedItemType(Domain::ItemType &item) {
  item.is_ground = item.group == ItemGroup::Ground;
  item.is_blocking = item.hasFlag(ItemFlag::Unpassable);
  item.is_moveable = !item.hasFlag(ItemFlag::Moveable);
  item.is_pickupable = item.hasFlag(ItemFlag::Pickupable);
  item.is_stackable = item.hasFlag(ItemFlag::Stackable);
  item.is_hangable = item.hasFlag(ItemFlag::Hangable);
  item.hook_east = item.hasFlag(ItemFlag::HookEast);
  item.hook_south = item.hasFlag(ItemFlag::HookSouth);
  item.blocks_projectile = item.hasFlag(ItemFlag::BlockMissiles);
  item.is_fluid_container = item.group == ItemGroup::Fluid;

  if (item.elevation > 0) {
    item.flags = item.flags | ItemFlag::HasElevation;
  }
  if (item.maxTextLen > 0) {
    item.can_read_text = true;
  }
  if (item.can_read_text) {
    item.flags = item.flags | ItemFlag::Readable;
  }
  if (item.allow_dist_read) {
    item.flags = item.flags | ItemFlag::AllowDistRead;
  }
  if (item.rotateTo != 0) {
    item.flags = item.flags | ItemFlag::Rotatable;
  }
  if (item.group == ItemGroup::Container) {
    item.item_type = Domain::ItemTypeEnum::Container;
  } else if (item.group == ItemGroup::Door) {
    item.item_type = Domain::ItemTypeEnum::Door;
  } else if (item.group == ItemGroup::Teleport) {
    item.item_type = Domain::ItemTypeEnum::Teleport;
  } else if (item.group == ItemGroup::MagicField) {
    item.item_type = Domain::ItemTypeEnum::MagicField;
  } else if (item.group == ItemGroup::Key) {
    item.item_type = Domain::ItemTypeEnum::Key;
  } else if (item.group == ItemGroup::Podium) {
    item.item_type = Domain::ItemTypeEnum::Podium;
  }
}

} // namespace MapEditor::Services
