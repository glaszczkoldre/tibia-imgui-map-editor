#include "ItemDefinitionResolver.h"
#include "IO/Readers/DatReaderBase.h"

#include <algorithm>
#include <format>
#include <unordered_map>
#include <spdlog/spdlog.h>

namespace MapEditor::Services {
namespace {

using Domain::ItemFlag;
using Domain::ItemGroup;
using Domain::ItemTypeEnum;

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
    const std::vector<IO::ServerItemFragment> &server_items,
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
    Domain::ItemType item;
    
    // 1. Server Metadata Precedence (OTB/SRV)
    item.server_id = server_item.server_id;
    item.client_id = server_item.client_id;
    item.group = server_item.group;
    item.name = server_item.name;
    item.description = server_item.description;
    item.speed = server_item.speed;
    item.light_level = server_item.light_level;
    item.light_color = server_item.light_color;
    item.always_on_top_order = server_item.top_order;
    item.wareId = server_item.wareId;
    item.maxTextLen = server_item.maxTextLen;
    item.disguise_target = server_item.disguise_target;
    
    // Copy flags
    item.flags = server_item.flags;
    
    // Map OTB flags (e.g. AlwaysOnTop is OTB bit 13 which actually means bottom)
    if (hasFlag(server_item.flags, ItemFlag::AlwaysOnBottom)) {
      setFlag(item, ItemFlag::AlwaysOnBottom, true);
    }
    
    // Translate some SRV/OTB types if they have explicit groups/types
    if (server_item.item_type != ItemTypeEnum::None) {
        item.item_type = server_item.item_type;
    } else {
        if (server_item.group == ItemGroup::Container) item.item_type = ItemTypeEnum::Container;
        else if (server_item.group == ItemGroup::Door) item.item_type = ItemTypeEnum::Door;
        else if (server_item.group == ItemGroup::Teleport) item.item_type = ItemTypeEnum::Teleport;
        else if (server_item.group == ItemGroup::MagicField) item.item_type = ItemTypeEnum::MagicField;
        else if (server_item.group == ItemGroup::Key) item.item_type = ItemTypeEnum::Key;
        else if (server_item.group == ItemGroup::Podium) item.item_type = ItemTypeEnum::Podium;
    }

    // 2. DAT Visuals Precedence
    // Resolver picks one final client id before DAT lookup
    const auto dat_it = dat_by_client_id.find(item.client_id);
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
  // Visual dimensions
  item.width = dat.width;
  item.height = dat.height;
  item.layers = dat.layers;
  item.pattern_x = dat.pattern_x;
  item.pattern_y = dat.pattern_y;
  item.pattern_z = dat.pattern_z;
  item.frames = dat.frames;
  item.sprite_ids = dat.sprite_ids;

  // Frame groups
  item.idle_sprite_ids = dat.idle_sprite_ids;
  item.walk_sprite_ids = dat.walk_sprite_ids;
  item.idle_frames = dat.idle_frames;
  item.walk_frames = dat.walk_frames;
  item.has_frame_groups = dat.has_frame_groups;

  // Classification
  const ItemGroup group_from_dat = datGroup(dat);
  if (datOwnsClassification) {
    item.group = group_from_dat;
  }

  // Speed
  if (dat.is_ground && dat.ground_speed > 0 &&
      (datOwnsClassification || item.speed == 0)) {
    item.speed = dat.ground_speed;
  }
  item.ground_speed = static_cast<uint8_t>(
      std::min<uint16_t>(dat.ground_speed, uint16_t{255}));

  // Light
  if (dat.has_light && (datOwnsClassification || item.light_level == 0)) {
    item.light_level = static_cast<uint8_t>(
        std::min<uint16_t>(dat.light_level, uint16_t{255}));
    item.light_color = static_cast<uint8_t>(
        std::min<uint16_t>(dat.light_color, uint16_t{255}));
  }

  // Offset
  item.draw_offset_x = dat.offset_x;
  item.draw_offset_y = dat.offset_y;
  
  // Elevation
  item.elevation = dat.elevation;

  // Minimap
  if (dat.has_minimap_color && (datOwnsClassification || item.minimap_color == 0)) {
    item.minimap_color = dat.minimap_color;
  }

  // Visual animation details
  item.animate_always = dat.animate_always;
  item.animation_mode = dat.animation_mode;
  item.loop_count = dat.loop_count;
  item.start_frame = dat.start_frame;
  item.frame_durations = dat.frame_durations;
  item.total_duration = dat.total_duration;
  
  // Stacking order (AlwaysOnTopOrder)
  if (item.always_on_top_order == 0) {
      if (dat.is_on_bottom) {
          item.always_on_top_order = 1; // Default border order if not OTB stackorder
      } else if (dat.is_on_top) {
          item.always_on_top_order = 3;
      }
  }

  // Final visual/gameplay flags from DAT OR'ed into final flags
  setFlag(item, ItemFlag::Unpassable, dat.is_unpassable);
  setFlag(item, ItemFlag::BlockMissiles, dat.blocks_missiles);
  setFlag(item, ItemFlag::BlockPathfinder, dat.blocks_pathfinder);
  setFlag(item, ItemFlag::Pickupable, dat.is_pickupable);
  setFlag(item, ItemFlag::Stackable, dat.is_stackable);
  setFlag(item, ItemFlag::Useable, dat.is_useable || dat.usable);
  setFlag(item, ItemFlag::IsHangable, dat.is_hangable);
  setFlag(item, ItemFlag::HookEast, dat.is_horizontal);
  setFlag(item, ItemFlag::HookSouth, dat.is_vertical);
  setFlag(item, ItemFlag::Rotatable, dat.is_rotatable);
  setFlag(item, ItemFlag::HasElevation, dat.has_elevation && dat.elevation > 0);
  setFlag(item, ItemFlag::IgnoreLook, dat.ignore_look);
  setFlag(item, ItemFlag::FullTile, dat.full_ground);
  setFlag(item, ItemFlag::Animation,
          dat.animate_always || dat.has_animation_data || dat.frames > 1);
  setFlag(item, ItemFlag::ClientCharges, dat.has_default_action || dat.default_action > 0);
  setFlag(item, ItemFlag::ForceUse, dat.usable);
  
  // Custom DAT flags
  setFlag(item, ItemFlag::AlwaysOnBottom, dat.is_on_bottom);
  setFlag(item, ItemFlag::OnTop, dat.is_on_top);
  setFlag(item, ItemFlag::DontHide, dat.dont_hide);
  setFlag(item, ItemFlag::Translucent, dat.is_translucent);
  setFlag(item, ItemFlag::NoMoveAnimation, dat.no_move_animation);
  setFlag(item, ItemFlag::Wrappable, dat.wrappable);
  setFlag(item, ItemFlag::Unwrappable, dat.unwrappable);
  setFlag(item, ItemFlag::TopEffect, dat.top_effect);
  setFlag(item, ItemFlag::Cloth, dat.is_cloth);
  setFlag(item, ItemFlag::MarketItem, dat.has_market_data);
  setFlag(item, ItemFlag::LensHelp, dat.lens_help > 0);

  if (dat.floor_change) {
    setFlag(item, ItemFlag::FloorChange, true);
  }

  // Handle moveability in DatOnly (where no OTB metadata exists)
  if (datOwnsClassification) {
    setFlag(item, ItemFlag::Moveable, !dat.is_unmoveable);
  }
}

void ItemDefinitionResolver::mergeXmlOverride(Domain::ItemType &item, const IO::XmlItemFragment &xml) {
    if (xml.client_id.has_value()) {
        item.client_id = xml.client_id.value();
    }
    if (xml.group.has_value()) {
        item.group = xml.group.value();
    }
    if (xml.item_type.has_value()) {
        item.item_type = xml.item_type.value();
    }
    if (xml.name.has_value()) {
        item.name = xml.name.value();
    }
    if (xml.article.has_value()) {
        item.article = xml.article.value();
    }
    if (xml.description.has_value()) {
        item.description = xml.description.value();
    }
    if (xml.editor_suffix.has_value()) {
        item.editor_suffix = xml.editor_suffix.value();
    }
    if (xml.weight.has_value()) {
        item.weight = xml.weight.value();
    }
    if (xml.armor.has_value()) {
        item.armor = xml.armor.value();
    }
    if (xml.defense.has_value()) {
        item.defense = xml.defense.value();
    }
    if (xml.attack.has_value()) {
        item.attack = xml.attack.value();
    }
    if (xml.shootRange.has_value()) {
        item.shootRange = xml.shootRange.value();
    }
    if (xml.ammoType.has_value()) {
        item.ammoType = xml.ammoType.value();
    }
    if (xml.volume.has_value()) {
        item.volume = xml.volume.value();
    }
    if (xml.rotateTo.has_value()) {
        item.rotateTo = xml.rotateTo.value();
        setFlag(item, ItemFlag::Rotatable, true);
    }
    if (xml.can_read_text.has_value()) {
        setFlag(item, ItemFlag::CanReadText, xml.can_read_text.value());
    }
    if (xml.can_write_text.has_value()) {
        setFlag(item, ItemFlag::CanWriteText, xml.can_write_text.value());
    }
    if (xml.maxTextLen.has_value()) {
        item.maxTextLen = xml.maxTextLen.value();
        setFlag(item, ItemFlag::CanReadText, true);
    }
    if (xml.allow_dist_read.has_value()) {
        setFlag(item, ItemFlag::AllowDistRead, xml.allow_dist_read.value());
    }
    if (xml.light_level.has_value()) {
        item.light_level = xml.light_level.value();
    }
    if (xml.light_color.has_value()) {
        item.light_color = xml.light_color.value();
    }
    if (xml.speed.has_value()) {
        item.speed = xml.speed.value();
    }
    if (xml.charges.has_value()) {
        item.charges = xml.charges.value();
    }
    if (xml.extra_chargeable.has_value()) {
        item.extra_chargeable = xml.extra_chargeable.value();
    }
    if (xml.decayTo.has_value()) {
        item.decayTo = xml.decayTo.value();
    }
    if (xml.stopDuration.has_value()) {
        item.stopDuration = xml.stopDuration.value();
    }
    if (xml.minimap_color.has_value()) {
        item.minimap_color = xml.minimap_color.value();
    }
    if (xml.is_pickupable.has_value()) {
        setFlag(item, ItemFlag::Pickupable, xml.is_pickupable.value());
    }
    if (xml.is_unpassable.has_value()) {
        setFlag(item, ItemFlag::Unpassable, xml.is_unpassable.value());
    }
    if (xml.blocks_projectile.has_value()) {
        setFlag(item, ItemFlag::BlockMissiles, xml.blocks_projectile.value());
    }
    if (xml.is_walkstack.has_value()) {
        setFlag(item, ItemFlag::BlockPathfinder, xml.is_walkstack.value());
    }
    if (xml.is_alwaysontop.has_value()) {
        setFlag(item, ItemFlag::AlwaysOnBottom, xml.is_alwaysontop.value());
    }
    
    // Floor change overrides
    if (xml.floor_change.has_value() && xml.floor_change.value()) {
        item.floor_change = true;
        setFlag(item, ItemFlag::FloorChange, true);
    }
    if (xml.floor_change_down.has_value()) item.floor_change_down = xml.floor_change_down.value();
    if (xml.floor_change_north.has_value()) item.floor_change_north = xml.floor_change_north.value();
    if (xml.floor_change_south.has_value()) item.floor_change_south = xml.floor_change_south.value();
    if (xml.floor_change_east.has_value()) item.floor_change_east = xml.floor_change_east.value();
    if (xml.floor_change_west.has_value()) item.floor_change_west = xml.floor_change_west.value();
    if (xml.floor_change_north_ex.has_value()) item.floor_change_north_ex = xml.floor_change_north_ex.value();
    if (xml.floor_change_south_ex.has_value()) item.floor_change_south_ex = xml.floor_change_south_ex.value();
    if (xml.floor_change_east_ex.has_value()) item.floor_change_east_ex = xml.floor_change_east_ex.value();
    if (xml.floor_change_west_ex.has_value()) item.floor_change_west_ex = xml.floor_change_west_ex.value();
    
    if (xml.slot_position.has_value()) {
        item.slot_position = xml.slot_position.value();
    }
    if (xml.weapon_type.has_value()) {
        item.weapon_type = xml.weapon_type.value();
    }
}

void ItemDefinitionResolver::normalizeResolvedItemType(Domain::ItemType &item) {
  // Translate groups to classification types if not overridden
  if (item.group == ItemGroup::Container) {
    item.item_type = ItemTypeEnum::Container;
  } else if (item.group == ItemGroup::Door) {
    item.item_type = ItemTypeEnum::Door;
  } else if (item.group == ItemGroup::Teleport) {
    item.item_type = ItemTypeEnum::Teleport;
  } else if (item.group == ItemGroup::MagicField) {
    item.item_type = ItemTypeEnum::MagicField;
  } else if (item.group == ItemGroup::Key) {
    item.item_type = ItemTypeEnum::Key;
  } else if (item.group == ItemGroup::Podium) {
    item.item_type = ItemTypeEnum::Podium;
  }

  // Ensure logical dependencies are set correctly
  if (item.elevation > 0) {
    item.flags = item.flags | ItemFlag::HasElevation;
  }
  if (item.maxTextLen > 0) {
    item.flags = item.flags | ItemFlag::CanReadText;
  }
  if (item.rotateTo != 0) {
    item.flags = item.flags | ItemFlag::Rotatable;
  }
  if (item.charges > 0) {
    item.flags = item.flags | ItemFlag::ClientCharges;
  }
  
  // Floor change flag consistency
  if (item.floor_change || item.floor_change_down || item.floor_change_north ||
      item.floor_change_south || item.floor_change_east || item.floor_change_west ||
      item.floor_change_north_ex || item.floor_change_south_ex ||
      item.floor_change_east_ex || item.floor_change_west_ex) {
      item.floor_change = true;
      item.flags = item.flags | ItemFlag::FloorChange;
  }
}

} // namespace MapEditor::Services
