#include "DatReaderBase.h"
#include "../Flags/CanonicalFlags.h"
#include <spdlog/spdlog.h>
#include <ranges>
#include <algorithm>
#include <iterator>

namespace MapEditor {
namespace IO {

using namespace CanonicalFlags;

DatResult DatReaderBase::read(const std::filesystem::path& path, uint32_t expected_signature) {
    DatResult result;
    
    BinaryReader reader(path);
    if (!reader.isOpen()) {
        result.error = "Failed to open DAT file: " + path.string();
        return result;
    }
    
    // Read and validate signature
    result.signature = reader.readU32();
    if (expected_signature != 0 && result.signature != expected_signature) {
        result.error = "DAT signature mismatch. Expected: " + 
                       std::to_string(expected_signature) +
                       ", Got: " + std::to_string(result.signature);
        return result;
    }
    
    // Read max IDs for each category
    result.max_item_id = reader.readU16();
    result.max_outfit_id = reader.readU16();
    result.max_effect_id = reader.readU16();
    result.max_missile_id = reader.readU16();
    
    spdlog::debug("DAT header: items={}, outfits={}, effects={}, missiles={}",
                  result.max_item_id, result.max_outfit_id, 
                  result.max_effect_id, result.max_missile_id);
    
    try {
        // Items start at ID 100
        readCategory(reader, result.items, DatCategory::Item, 100, result.max_item_id);
        
        // Outfits start at ID 1
        readCategory(reader, result.outfits, DatCategory::Outfit, 1, result.max_outfit_id);
        
        // Effects start at ID 1
        readCategory(reader, result.effects, DatCategory::Effect, 1, result.max_effect_id);
        
        // Missiles start at ID 1
        readCategory(reader, result.missiles, DatCategory::Missile, 1, result.max_missile_id);
        
    } catch (const std::exception& e) {
        result.error = std::string("Error reading DAT: ") + e.what();
        return result;
    }
    
    spdlog::info("Loaded DAT using {}: {} items, {} outfits, {} effects, {} missiles",
                 getName(), result.items.size(), result.outfits.size(),
                 result.effects.size(), result.missiles.size());
    
    result.success = true;
    return result;
}

void DatReaderBase::readCategory(BinaryReader& reader, std::vector<ClientItem>& items,
                                  DatCategory category, uint16_t min_id, uint16_t max_id) {
    items.reserve(max_id - min_id + 1);
    
    // Use iota view to safely iterate over ID range
    // Casting to int prevents infinite loop if max_id is UINT16_MAX
    auto ids = std::views::iota(static_cast<int>(min_id), static_cast<int>(max_id) + 1);

    for (int id : ids) {
        ClientItem item;
        item.id = static_cast<uint16_t>(id);
        item.category = category;
        
        // Read version-specific flags
        readItemFlags(item, reader);
        
        // Read sprite data (common across versions)
        readSpriteData(item, reader);
        
        items.push_back(std::move(item));
    }
}

void DatReaderBase::readItemFlags(ClientItem& item, BinaryReader& reader) {
    uint8_t raw_flag;

    while (true) {
        raw_flag = reader.readU8();

        // [PHANTOM FIX] Check for read errors to prevent infinite loop
        // If readU8 fails (EOF), it returns 0. If 0 is a valid flag (GROUND),
        // we might enter an infinite loop of processing GROUND forever.
        if (!reader.good()) {
            break;
        }

        if (raw_flag == LAST) {
            break;
        }

        uint8_t flag = transformFlag(raw_flag);

        // Allow child classes to intercept/override specific flag handling
        if (handleSpecificFlag(flag, item, reader)) {
            continue;
        }

        switch (flag) {
            case GROUND:
                item.is_ground = true;
                item.ground_speed = reader.readU16();
                break;

            case GROUND_BORDER:
                item.is_on_bottom = true;
                break;

            case ON_BOTTOM:
                item.is_on_bottom = true;
                break;

            case ON_TOP:
                item.is_on_top = true;
                break;

            case CONTAINER:
                item.is_container = true;
                break;

            case STACKABLE:
                item.is_stackable = true;
                break;

            case FORCE_USE:
                break;

            case MULTI_USE:
                item.is_useable = true;
                break;

            case WRITABLE:
                item.is_writable = true;
                item.max_text_length = reader.readU16();
                break;

            case WRITABLE_ONCE:
                item.is_writable = true;
                item.max_text_length = reader.readU16();
                break;

            case FLUID_CONTAINER:
                item.is_fluid_container = true;
                break;

            case FLUID:
                item.is_fluid = true;
                break;

            case UNPASSABLE:
                item.is_unpassable = true;
                break;

            case UNMOVEABLE:
                item.is_unmoveable = true;
                break;

            case BLOCK_MISSILE:
                item.blocks_missiles = true;
                break;

            case BLOCK_PATHFINDER:
                item.blocks_pathfinder = true;
                break;

            case PICKUPABLE:
                item.is_pickupable = true;
                break;

            case HANGABLE:
                item.is_hangable = true;
                break;

            case HOOK_SOUTH:
                item.is_vertical = true;
                break;

            case HOOK_EAST:
                item.is_horizontal = true;
                break;

            case ROTATABLE:
                item.is_rotatable = true;
                break;

            case HAS_LIGHT:
                item.has_light = true;
                item.light_level = reader.readU16();
                item.light_color = reader.readU16();
                break;

            case DONT_HIDE:
                item.dont_hide = true;
                break;

            case TRANSLUCENT:
                item.is_translucent = true;
                break;

            case HAS_OFFSET:
                item.has_offset = true;
                item.offset_x = static_cast<int16_t>(reader.readU16());
                item.offset_y = static_cast<int16_t>(reader.readU16());
                break;

            case HAS_ELEVATION:
                item.has_elevation = true;
                item.elevation = reader.readU16();
                break;

            case LYING_OBJECT:
                item.is_lying_object = true;
                break;

            case ANIMATE_ALWAYS:
                item.animate_always = true;
                break;

            case MINI_MAP:
                item.has_minimap_color = true;
                item.minimap_color = reader.readU16();
                break;

            case LENS_HELP:
                item.lens_help = reader.readU16();
                break;

            case FULL_GROUND:
                item.full_ground = true;
                break;

            case IGNORE_LOOK:
                item.ignore_look = true;
                break;

            case CLOTH:
                item.is_cloth = true;
                item.cloth_slot = reader.readU16();
                break;

            case MARKET_ITEM:
                item.has_market_data = true;
                item.market_category = reader.readU16();
                item.trade_as = reader.readU16();
                item.show_as = reader.readU16();
                {
                    uint16_t name_len = reader.readU16();
                    item.market_name = reader.readString(name_len);
                }
                item.market_profession = reader.readU16();
                item.market_level = reader.readU16();
                break;

            case DEFAULT_ACTION:
                item.has_default_action = true;
                item.default_action = reader.readU16();
                break;

            case WRAPPABLE:
                item.wrappable = true;
                break;

            case UNWRAPPABLE:
                item.unwrappable = true;
                break;

            case TOP_EFFECT:
                item.top_effect = true;
                break;

            case NPC_SALE_DATA:
                // Skip NPC sale data (3 uint16_t values)
                reader.readU16();
                reader.readU16();
                reader.readU16();
                break;

            case CHANGER:
                // Changer flag, no data
                break;

            case PODIUM:
                // Podium flag, no data
                break;

            case USABLE:
                item.usable = true;
                break;

            case NO_MOVE_ANIMATION:
                item.no_move_animation = true;
                break;

            case FLOOR_CHANGE:
                item.floor_change = true;
                break;

            case CHARGEABLE:
                // Item is chargeable (no additional data)
                break;

            default:
                spdlog::warn("Unknown flag 0x{:02X} (raw: 0x{:02X}) for item {}",
                            flag, raw_flag, item.id);
                break;
        }
    }
}

void DatReaderBase::readSpriteData(ClientItem& item, BinaryReader& reader) {
    // Frame groups (10.50+ for outfits only)
    uint8_t group_count = 1;
    if (hasFrameGroups() && item.category == DatCategory::Outfit) {
        group_count = reader.readU8();
    }
    
    for (uint8_t g = 0; g < group_count; ++g) {
        uint8_t group_type = 0;
        if (hasFrameGroups() && item.category == DatCategory::Outfit) {
            group_type = reader.readU8();
        }

        // Store frame group data for each group
        uint8_t gw = reader.readU8();
        uint8_t gh = reader.readU8();
        
        if (gw > 1 || gh > 1) {
            reader.readU8();  // exact size
        }
        
        uint8_t gl = reader.readU8();
        uint8_t gpx = reader.readU8();
        uint8_t gpy = reader.readU8();
        uint8_t gpz = shouldReadPatternZ() ? reader.readU8() : 1;
        uint8_t gf = reader.readU8();
        
        if (gf > 1 && hasFrameDurations()) {
            uint8_t anim_mode = reader.readU8();
            int32_t loop = static_cast<int32_t>(reader.readU32());
            uint8_t start = reader.readU8();
            
            if (g == 0) {
                item.has_animation_data = true;
                item.animation_mode = anim_mode;
                item.loop_count = loop;
                item.start_frame = start;
            }
            
             for (uint8_t f = 0; f < gf; ++f) {
                uint32_t min_duration = reader.readU32();
                uint32_t max_duration = reader.readU32();
                if (g == 0) {
                    item.frame_durations.emplace_back(min_duration, max_duration);
                    item.total_duration += (min_duration + max_duration) / 2;
                }
            }
        }
        
        uint32_t sprite_count = static_cast<uint32_t>(gw) * gh * gl * gpx * gpy * gpz * gf;
        std::vector<uint32_t> group_sprites;
        if (sprite_count > 100000) {
            for (uint32_t i = 0; i < sprite_count; ++i)
                if (usesExtendedSprites()) reader.readU32(); else reader.readU16();
        } else {
            group_sprites.reserve(sprite_count);
            for (uint32_t i = 0; i < sprite_count; ++i)
                group_sprites.push_back(usesExtendedSprites() ? reader.readU32() : reader.readU16());
        }
        
        if (g == 0) {
            item.width = gw;
            item.height = gh;
            item.layers = gl;
            item.pattern_x = gpx;
            item.pattern_y = gpy;
            item.pattern_z = gpz;
            item.frames = gf;
            item.sprite_ids = std::move(group_sprites);
        } else {
            item.has_frame_groups = true;
            if (group_type == 0) {
                item.idle_sprite_ids = std::move(group_sprites);
                item.idle_frames = gf;
            } else {
                item.walk_sprite_ids = std::move(group_sprites);
                item.walk_frames = gf;
            }
        }
    }
}

} // namespace IO
} // namespace MapEditor
