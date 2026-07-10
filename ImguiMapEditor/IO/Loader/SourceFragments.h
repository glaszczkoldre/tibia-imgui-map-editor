#pragma once

#include "Domain/ItemType.h"
#include <string>
#include <vector>
#include <optional>

namespace MapEditor::IO {

enum class DatCategory : uint8_t {
    Item = 0,
    Outfit = 1,
    Effect = 2,
    Missile = 3
};

struct DatItemFragment {
    uint16_t id = 0;
    DatCategory category = DatCategory::Item;
    
    // Sprite dimensions
    uint8_t width = 1;
    uint8_t height = 1;
    uint8_t layers = 1;
    uint8_t pattern_x = 1;
    uint8_t pattern_y = 1;
    uint8_t pattern_z = 1;
    uint8_t frames = 1;
    
    std::vector<uint32_t> sprite_ids;

    // Frame groups (10.57+)
    std::vector<uint32_t> idle_sprite_ids;
    std::vector<uint32_t> walk_sprite_ids;
    uint8_t idle_frames = 1;
    uint8_t walk_frames = 1;
    bool has_frame_groups = false;
    
    // Properties from flags
    bool is_ground = false;
    uint16_t ground_speed = 0;
    bool is_on_bottom = false;
    bool is_on_top = false;
    bool is_container = false;
    bool is_stackable = false;
    bool is_useable = false;
    bool is_writable = false;
    uint16_t max_text_length = 0;
    bool is_fluid_container = false;
    bool is_fluid = false;
    bool is_unpassable = false;
    bool is_unmoveable = false;
    bool blocks_missiles = false;
    bool blocks_pathfinder = false;
    bool is_pickupable = false;
    bool is_hangable = false;
    bool is_horizontal = false;
    bool is_vertical = false;
    bool is_rotatable = false;
    bool has_light = false;
    uint16_t light_level = 0;
    uint16_t light_color = 0;
    bool dont_hide = false;
    bool is_translucent = false;
    bool has_offset = false;
    int16_t offset_x = 0;
    int16_t offset_y = 0;
    bool has_elevation = false;
    uint16_t elevation = 0;
    bool is_lying_object = false;
    bool animate_always = false;
    bool has_minimap_color = false;
    uint16_t minimap_color = 0;
    bool full_ground = false;
    bool ignore_look = false;
    bool is_cloth = false;
    uint16_t cloth_slot = 0;
    bool has_market_data = false;
    uint16_t market_category = 0;
    uint16_t trade_as = 0;
    uint16_t show_as = 0;
    std::string market_name;
    uint16_t market_profession = 0;
    uint16_t market_level = 0;
    bool has_default_action = false;
    uint16_t default_action = 0;
    bool floor_change = false;
    uint16_t lens_help = 0;
    bool wrappable = false;
    bool unwrappable = false;
    bool top_effect = false;
    bool no_move_animation = false;
    bool usable = false;
    
    // Animation data
    bool has_animation_data = false;
    uint8_t animation_mode = 0;
    int32_t loop_count = 0;
    uint8_t start_frame = 0;
    std::vector<std::pair<uint32_t, uint32_t>> frame_durations;
    uint32_t total_duration = 0;
    
    uint32_t getTotalSprites() const {
        return static_cast<uint32_t>(width) * height * layers *
               pattern_x * pattern_y * pattern_z * frames;
    }
};

struct ServerItemFragment {
    uint16_t server_id = 0;
    uint16_t client_id = 0;
    
    Domain::ItemGroup group = Domain::ItemGroup::None;
    Domain::ItemFlag flags = Domain::ItemFlag::None;
    
    std::string name;
    std::string description;
    
    uint16_t speed = 0;
    uint8_t light_level = 0;
    uint8_t light_color = 0;
    int8_t top_order = 0;
    uint16_t wareId = 0;
    uint16_t maxTextLen = 0;
    uint16_t disguise_target = 0;
    
    // SRV-specific metadata attributes
    Domain::ItemTypeEnum item_type = Domain::ItemTypeEnum::None;
    uint16_t volume = 0;
    float weight = 0.0f;
    int16_t attack = 0;
    int16_t defense = 0;
    int16_t armor = 0;
    uint32_t charges = 0;
    uint16_t rotateTo = 0;
};

struct XmlItemFragment {
    uint16_t server_id = 0;
    std::optional<uint16_t> client_id;
    std::optional<Domain::ItemGroup> group;
    std::optional<Domain::ItemTypeEnum> item_type;
    
    std::optional<std::string> name;
    std::optional<std::string> article;
    std::optional<std::string> description;
    std::optional<std::string> editor_suffix;
    
    std::optional<float> weight;
    std::optional<int16_t> armor;
    std::optional<int16_t> defense;
    std::optional<int16_t> attack;
    std::optional<uint8_t> shootRange;
    std::optional<std::string> ammoType;
    std::optional<uint16_t> volume;
    std::optional<uint16_t> rotateTo;
    
    std::optional<bool> can_read_text;
    std::optional<bool> can_write_text;
    std::optional<uint16_t> maxTextLen;
    std::optional<bool> allow_dist_read;
    
    std::optional<uint8_t> light_level;
    std::optional<uint8_t> light_color;
    std::optional<uint16_t> speed;
    std::optional<uint32_t> charges;
    std::optional<bool> extra_chargeable;
    std::optional<uint16_t> decayTo;
    std::optional<uint32_t> stopDuration;
    std::optional<uint16_t> minimap_color;
    
    std::optional<bool> is_pickupable;
    std::optional<bool> is_unpassable;
    std::optional<bool> blocks_projectile;
    std::optional<bool> is_walkstack;
    std::optional<bool> is_alwaysontop;
    
    // Floor change flags
    std::optional<bool> floor_change;
    std::optional<bool> floor_change_down;
    std::optional<bool> floor_change_north;
    std::optional<bool> floor_change_south;
    std::optional<bool> floor_change_east;
    std::optional<bool> floor_change_west;
    std::optional<bool> floor_change_north_ex;
    std::optional<bool> floor_change_south_ex;
    std::optional<bool> floor_change_east_ex;
    std::optional<bool> floor_change_west_ex;
    
    std::optional<Domain::SlotPosition> slot_position;
    std::optional<Domain::WeaponType> weapon_type;
};

// Aliases for compatibility/readability in readers
using ClientItem = DatItemFragment;

} // namespace MapEditor::IO
