#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace MapEditor {
namespace Rendering {
struct AtlasRegion;
}
} // namespace MapEditor

namespace MapEditor {
namespace Domain {

/**
 * Item groups from OTB file
 */
enum class ItemGroup : uint8_t {
  None = 0,
  Ground,
  Container,
  Weapon,
  Ammunition,
  Armor,
  Changes, // Deprecated
  Teleport,
  MagicField,
  Writeable,
  Key,
  Splash,
  Fluid,
  Door,
  Deprecated,
  Podium,
  Last
};

/**
 * Item flags from OTB file
 */
enum class ItemFlag : uint32_t {
  None = 0,
  Unpassable = 1 << 0,
  BlockMissiles = 1 << 1,
  BlockPathfinder = 1 << 2,
  HasElevation = 1 << 3,
  Useable = 1 << 4,
  Pickupable = 1 << 5,
  Moveable = 1 << 6,
  Stackable = 1 << 7,
  FloorChangeDown = 1 << 8,
  FloorChangeNorth = 1 << 9,
  FloorChangeEast = 1 << 10,
  FloorChangeSouth = 1 << 11,
  FloorChangeWest = 1 << 12,
  AlwaysOnTop = 1 << 13,
  Readable = 1 << 14,
  Rotatable = 1 << 15,
  Hangable = 1 << 16,
  HookEast = 1 << 17,
  HookSouth = 1 << 18,
  CanNotDecay = 1 << 19,
  AllowDistRead = 1 << 20,
  Unused = 1 << 21,
  ClientCharges = 1 << 22,
  IgnoreLook = 1 << 23,
  Animation = 1 << 24,
  FullTile = 1 << 25,
  ForceUse = 1 << 26
};

inline ItemFlag operator|(ItemFlag a, ItemFlag b) {
  return static_cast<ItemFlag>(static_cast<uint32_t>(a) |
                               static_cast<uint32_t>(b));
}

inline ItemFlag operator&(ItemFlag a, ItemFlag b) {
  return static_cast<ItemFlag>(static_cast<uint32_t>(a) &
                               static_cast<uint32_t>(b));
}

inline bool hasFlag(ItemFlag flags, ItemFlag flag) {
  return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

/**
 * Slot position flags (from items.json slotType)
 */
enum class SlotPosition : uint16_t {
  None = 0,
  Head = 1 << 0,
  Necklace = 1 << 1,
  Backpack = 1 << 2,
  Armor = 1 << 3,
  Right = 1 << 4,
  Left = 1 << 5,
  Legs = 1 << 6,
  Feet = 1 << 7,
  Ring = 1 << 8,
  Ammo = 1 << 9,
  Hand = Right | Left,
  TwoHand = 1 << 10
};

inline SlotPosition operator|(SlotPosition a, SlotPosition b) {
  return static_cast<SlotPosition>(static_cast<uint16_t>(a) |
                                   static_cast<uint16_t>(b));
}

inline SlotPosition operator&(SlotPosition a, SlotPosition b) {
  return static_cast<SlotPosition>(static_cast<uint16_t>(a) &
                                   static_cast<uint16_t>(b));
}

inline SlotPosition operator~(SlotPosition a) {
  return static_cast<SlotPosition>(~static_cast<uint16_t>(a));
}

inline SlotPosition &operator|=(SlotPosition &a, SlotPosition b) {
  a = a | b;
  return a;
}

inline SlotPosition &operator&=(SlotPosition &a, SlotPosition b) {
  a = a & b;
  return a;
}

/**
 * Weapon types (from items.json weaponType)
 */
enum class WeaponType : uint8_t {
  None = 0,
  Sword,
  Club,
  Axe,
  Shield,
  Distance,
  Wand,
  Ammo
};

/**
 * Item types (from items.json type attribute)
 */
enum class ItemTypeEnum : uint8_t {
  None = 0,
  Depot,
  Mailbox,
  TrashHolder,
  Container,
  Door,
  MagicField,
  Teleport,
  Bed,
  Key,
  Podium
};

/**
 * Item type definition - loaded from OTB and DAT files
 * Represents the properties of an item type, not an instance
 */
class ItemType {
public:
  // Identifiers
  uint16_t server_id = 0; // From OTB - used in OTBM maps
  uint16_t client_id = 0; // From DAT - used for rendering

  // OTB properties
  ItemGroup group = ItemGroup::None;
  ItemFlag flags = ItemFlag::None;

  // Basic properties
  std::string name;
  std::string article;
  std::string description;

  // Movement properties
  uint16_t speed = 0; // Ground speed
  bool is_blocking = false;
  bool is_moveable = true;
  bool is_pickupable = false;
  bool is_stackable = false;
  bool is_fluid_container =
      false;              // From DAT - items that hold fluids (buckets, vials)
  bool is_ground = false; // From DAT - ThingAttrGround attribute

  // Rendering properties (from DAT)
  uint8_t width = 1;
  uint8_t height = 1;
  uint8_t layers = 1;
  uint8_t pattern_x = 1;
  uint8_t pattern_y = 1;
  uint8_t pattern_z = 1;
  uint8_t frames = 1;
  uint8_t ground_speed = 0;
  int8_t top_order = 0; // Render order for "always on top" items

  // Light properties
  uint8_t light_level = 0;
  uint8_t light_color = 0;

  // Minimap color (from DAT) - 8-bit index into 256-color palette
  uint16_t minimap_color = 0;

  // Draw offset (from DAT) - sprite visual offset from tile position
  int16_t draw_offset_x = 0;
  int16_t draw_offset_y = 0;

  // Translucency (from DAT)
  bool is_translucent = false;

  // Elevation (from DAT) - items on this raise subsequent items visually
  uint16_t elevation = 0;

  // Stacking order (from OTB FLAG_ALWAYSONTOP actually means bottom)
  bool always_on_bottom = false;

  // Hook/hangable properties (from OTB flags)
  bool is_hangable = false;
  bool hook_east = false;
  bool hook_south = false;

  // Floor visibility flags (from DAT - critical for floor rendering)
  bool is_on_bottom = false; // Wall-like items that block floor view
  bool is_on_top = false;    // Doors, windows (drawn last, priority 3)
  bool is_dont_hide =
      false; // Items that never block floor view (transparent roofs, etc)
  bool blocks_projectile =
      false; // Used in non-free view mode for floor blocking

  // Border/wall detection (used for rendering order)
  bool is_border = false;
  bool is_wall = false;
  bool is_locked = false; // Door key lock status (for highlight_locked_doors)

  // Sprite IDs for rendering
  std::vector<uint32_t> sprite_ids;

  // Market/Store properties
  uint16_t wareId = 0;

  // Writeable properties (RME-compatible from items.json)
  uint16_t maxTextLen = 0; // max_text_len in JSON
  bool can_read_text = false;
  bool can_write_text = false;
  bool allow_dist_read = false;

  // Rotation - target item ID when rotated (0 = not rotatable)
  uint16_t rotateTo = 0;

  // === FROM items.json (RME-compatible) ===

  // Editor display
  std::string editor_suffix; // editorSuffix in JSON

  // Combat stats
  float weight = 0.0f; // Divided by 100 from JSON
  int16_t armor = 0;
  int16_t defense = 0;
  int16_t attack = 0;

  // Equipment
  SlotPosition slot_position = SlotPosition::None;
  WeaponType weapon_type = WeaponType::None;
  ItemTypeEnum item_type = ItemTypeEnum::None;

  // Floor change
  bool floor_change = false;
  bool floor_change_down = false;
  bool floor_change_north = false;
  bool floor_change_south = false;
  bool floor_change_east = false;
  bool floor_change_west = false;
  bool floor_change_north_ex = false;
  bool floor_change_south_ex = false;
  bool floor_change_east_ex = false;
  bool floor_change_west_ex = false;

  // Container
  uint16_t volume = 0; // containerSize in JSON

  // Charges
  uint32_t charges = 0;
  bool extra_chargeable = false;

  // Decay
  bool decays = false;

  // Track if XML was merged
  bool xml_loaded = false;

  // PERFORMANCE: Cached first sprite region (pre-fetched during loading)
  // Eliminates hash lookup in getSpriteRegion() - 30k+ lookups/frame → 0
  const MapEditor::Rendering::AtlasRegion *cached_sprite_region =
      nullptr; // Points to AtlasRegion (avoid circular include)

  // Additional RME-compatible fields
  uint8_t shootRange = 0;
  uint16_t decayTo = 0;
  uint32_t stopDuration = 0;
  std::string ammoType;

  // Disguise - display this item using another item's appearance (from
  // items.srv DisguiseTarget)
  uint16_t disguise_target = 0;

  // Helper methods
  // Type checks - combines OTB group (from items.otb) AND item_type (from
  // items.json) This ensures compatibility regardless of which data source
  // defines the type
  bool isReadable() const {
    return can_read_text || hasFlag(ItemFlag::Readable);
  }
  // isWriteable() is defined below near line 304

  bool isGround() const { return group == ItemGroup::Ground; }
  bool isContainer() const {
    return group == ItemGroup::Container ||
           item_type == ItemTypeEnum::Container;
  }
  bool isSplash() const { return group == ItemGroup::Splash; }
  bool isFluid() const {
    return group == ItemGroup::Fluid || is_fluid_container;
  }
  bool isFluidContainer() const {
    return group == ItemGroup::Fluid || is_fluid_container;
  }
  bool isDoor() const {
    return group == ItemGroup::Door || item_type == ItemTypeEnum::Door;
  }
  bool isTeleport() const {
    return group == ItemGroup::Teleport || item_type == ItemTypeEnum::Teleport;
  }
  bool isMagicField() const {
    return group == ItemGroup::MagicField ||
           item_type == ItemTypeEnum::MagicField;
  }
  bool isWriteable() const {
    return group == ItemGroup::Writeable || can_write_text;
  }
  bool isKey() const {
    return group == ItemGroup::Key || item_type == ItemTypeEnum::Key;
  }
  bool isPodium() const {
    return group == ItemGroup::Podium || item_type == ItemTypeEnum::Podium;
  }
  bool isDepot() const { return item_type == ItemTypeEnum::Depot; }
  bool isMailbox() const { return item_type == ItemTypeEnum::Mailbox; }
  bool isTrashHolder() const { return item_type == ItemTypeEnum::TrashHolder; }
  bool isBed() const { return item_type == ItemTypeEnum::Bed; }

  bool isRotatable() const {
    return Domain::hasFlag(flags, ItemFlag::Rotatable) && rotateTo != 0;
  }

  bool hasFlag(ItemFlag flag) const { return Domain::hasFlag(flags, flag); }

  // Check if this item has elevation (raises items on top of it)
  bool hasElevation() const {
    return Domain::hasFlag(flags, ItemFlag::HasElevation) && elevation > 0;
  }

  // Get the first sprite ID for rendering
  uint32_t getFirstSpriteId() const {
    return sprite_ids.empty() ? 0 : sprite_ids[0];
  }

  // Calculate total sprite count
  size_t getSpriteCount() const {
    return static_cast<size_t>(width) * height * layers * pattern_x *
           pattern_y * pattern_z * frames;
  }

  /**
   * Check if this item type has valid data for rendering.
   * Returns false for "gap" entries in items.otb that have a server_id
   * but no actual item data (no client_id, no sprites).
   */
  bool isValidForRendering() const {
    // Item must have a client_id (DA T mapping) to be renderable
    // Items with only server_id and no client_id are gaps/placeholders
    return client_id > 0 && !sprite_ids.empty();
  }
};

} // namespace Domain
} // namespace MapEditor
