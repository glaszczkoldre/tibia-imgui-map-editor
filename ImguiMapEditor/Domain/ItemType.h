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
 * Item flags resolved from OTB, DAT, SRV, and XML
 */
enum class ItemFlag : uint64_t {
  None = 0,
  Unpassable = 1ULL << 0,
  BlockMissiles = 1ULL << 1,
  BlockPathfinder = 1ULL << 2,
  HasElevation = 1ULL << 3,
  Useable = 1ULL << 4,
  Pickupable = 1ULL << 5,
  Moveable = 1ULL << 6,
  Stackable = 1ULL << 7,
  FloorChangeDown = 1ULL << 8,
  FloorChangeNorth = 1ULL << 9,
  FloorChangeEast = 1ULL << 10,
  FloorChangeSouth = 1ULL << 11,
  FloorChangeWest = 1ULL << 12,
  AlwaysOnBottom = 1ULL << 13,
  CanReadText = 1ULL << 14,
  Rotatable = 1ULL << 15,
  IsHangable = 1ULL << 16,
  HookEast = 1ULL << 17,
  HookSouth = 1ULL << 18,
  CanNotDecay = 1ULL << 19,
  AllowDistRead = 1ULL << 20,
  CanWriteText = 1ULL << 21,
  ClientCharges = 1ULL << 22,
  IgnoreLook = 1ULL << 23,
  Animation = 1ULL << 24,
  FullTile = 1ULL << 25,
  ForceUse = 1ULL << 26,
  
  // Visual/gameplay flags from DAT
  Translucent = 1ULL << 27,
  DontHide = 1ULL << 28,
  OnTop = 1ULL << 29,
  NoMoveAnimation = 1ULL << 30,
  Wrappable = 1ULL << 31,
  Unwrappable = 1ULL << 32,
  TopEffect = 1ULL << 33,
  Cloth = 1ULL << 34,
  MarketItem = 1ULL << 35,
  LensHelp = 1ULL << 36,
  Decays = 1ULL << 37,
  FloorChange = 1ULL << 38
};

inline ItemFlag operator|(ItemFlag a, ItemFlag b) {
  return static_cast<ItemFlag>(static_cast<uint64_t>(a) |
                               static_cast<uint64_t>(b));
}

inline ItemFlag operator&(ItemFlag a, ItemFlag b) {
  return static_cast<ItemFlag>(static_cast<uint64_t>(a) &
                               static_cast<uint64_t>(b));
}

inline ItemFlag operator~(ItemFlag a) {
  return static_cast<ItemFlag>(~static_cast<uint64_t>(a));
}

inline bool hasFlag(ItemFlag flags, ItemFlag flag) {
  return (static_cast<uint64_t>(flags) & static_cast<uint64_t>(flag)) != 0;
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
 * Centralized, read-only item definition store structure.
 * Generated from multiple raw source fragments via ItemDefinitionResolver.
 */
class ItemDefinition {
public:
  // Identifiers
  uint16_t server_id = 0; // From OTB/SRV - used in maps
  uint16_t client_id = 0; // From DAT - used for rendering

  ItemGroup group = ItemGroup::None;
  ItemFlag flags = ItemFlag::None;

  // Basic properties
  std::string name;
  std::string article;
  std::string description;

  // Movement/attributes properties
  uint16_t speed = 0; // Ground speed
  uint8_t ground_speed = 0;
  int8_t always_on_top_order = 0; // Render order (previously top_order)

  // Rendering properties (from DAT)
  uint8_t width = 1;
  uint8_t height = 1;
  uint8_t layers = 1;
  uint8_t pattern_x = 1;
  uint8_t pattern_y = 1;
  uint8_t pattern_z = 1;
  uint8_t frames = 1;

  // Animation data (from DAT)
  bool animate_always = false;
  uint8_t animation_mode = 0;
  int32_t loop_count = 0;
  uint8_t start_frame = 0;
  std::vector<std::pair<uint32_t, uint32_t>> frame_durations;
  uint32_t total_duration = 0;       // Pre-calculated animation sum

  // Frame groups (creatures with idle/walking animations)
  std::vector<uint32_t> idle_sprite_ids;
  std::vector<uint32_t> walk_sprite_ids;
  uint8_t idle_frames = 1;
  uint8_t walk_frames = 1;
  bool has_frame_groups = false;

  // Light properties
  uint8_t light_level = 0;
  uint8_t light_color = 0;

  // Minimap color (from DAT/XML)
  uint16_t minimap_color = 0;

  // Draw offset (from DAT)
  int16_t draw_offset_x = 0;
  int16_t draw_offset_y = 0;

  // Elevation (from DAT)
  uint16_t elevation = 0;

  // Sprite IDs for rendering
  std::vector<uint32_t> sprite_ids;

  // Market/Store properties
  uint16_t wareId = 0;

  // Writeable properties (RME-compatible from items.json)
  uint16_t maxTextLen = 0; 
  uint16_t rotateTo = 0;

  // Display and Stats
  std::string editor_suffix;
  float weight = 0.0f;
  int16_t armor = 0;
  int16_t defense = 0;
  int16_t attack = 0;

  SlotPosition slot_position = SlotPosition::None;
  WeaponType weapon_type = WeaponType::None;
  ItemTypeEnum item_type = ItemTypeEnum::None;

  // Floor change details
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

  // Attributes
  uint16_t volume = 0;
  uint32_t charges = 0;
  bool extra_chargeable = false;
  uint8_t shootRange = 0;
  uint16_t decayTo = 0;
  uint32_t stopDuration = 0;
  std::string ammoType;

  // Disguise target
  uint16_t disguise_target = 0;

  // Track if XML was merged
  bool xml_loaded = false;

  // PERFORMANCE: Cached first sprite region
  const MapEditor::Rendering::AtlasRegion *cached_sprite_region = nullptr;

  // Helper methods over the final resolved item definition.
  bool hasFlag(ItemFlag flag) const { return Domain::hasFlag(flags, flag); }

  bool isReadable() const {
    return hasFlag(ItemFlag::CanReadText);
  }

  bool isGround() const { return group == ItemGroup::Ground; }
  bool isContainer() const {
    return item_type == ItemTypeEnum::Container;
  }
  bool isSplash() const { return group == ItemGroup::Splash; }
  bool isFluid() const { return group == ItemGroup::Fluid; }
  bool isFluidContainer() const { return group == ItemGroup::Fluid; }
  bool isDoor() const {
    return item_type == ItemTypeEnum::Door;
  }
  bool isTeleport() const {
    return item_type == ItemTypeEnum::Teleport;
  }
  bool isMagicField() const {
    return item_type == ItemTypeEnum::MagicField;
  }
  bool isWriteable() const {
    return hasFlag(ItemFlag::CanWriteText);
  }
  bool isKey() const {
    return item_type == ItemTypeEnum::Key;
  }
  bool isPodium() const {
    return item_type == ItemTypeEnum::Podium;
  }
  bool isDepot() const { return item_type == ItemTypeEnum::Depot; }
  bool isMailbox() const { return item_type == ItemTypeEnum::Mailbox; }
  bool isTrashHolder() const { return item_type == ItemTypeEnum::TrashHolder; }
  bool isBed() const { return item_type == ItemTypeEnum::Bed; }

  bool isRotatable() const {
    return hasFlag(ItemFlag::Rotatable) && rotateTo != 0;
  }

  // Check if this item has elevation (raises items on top of it)
  bool hasElevation() const {
    return hasFlag(ItemFlag::HasElevation) && elevation > 0;
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

  bool isValidForRendering() const {
    return client_id > 0 && !sprite_ids.empty();
  }

  // Unified helpers to replace old boolean fields
  bool isBlocking() const { return hasFlag(ItemFlag::Unpassable); }
  bool isMoveable() const { return hasFlag(ItemFlag::Moveable); }
  bool isPickupable() const { return hasFlag(ItemFlag::Pickupable); }
  bool isStackable() const { return hasFlag(ItemFlag::Stackable); }
  bool isOnBottom() const { return hasFlag(ItemFlag::AlwaysOnBottom); }
  bool isOnTop() const { return hasFlag(ItemFlag::OnTop); }
  bool isDontHide() const { return hasFlag(ItemFlag::DontHide); }
  bool blocksProjectile() const { return hasFlag(ItemFlag::BlockMissiles); }
  bool canReadText() const { return hasFlag(ItemFlag::CanReadText); }
  bool canWriteText() const { return hasFlag(ItemFlag::CanWriteText); }
  bool allowDistRead() const { return hasFlag(ItemFlag::AllowDistRead); }
  bool isBorder() const { return false; }
  bool isLocked() const { return false; }
  bool isWall() const { return false; }
  bool hookSouth() const { return hasFlag(ItemFlag::HookSouth); }
  bool hookEast() const { return hasFlag(ItemFlag::HookEast); }
  bool isHangable() const { return hasFlag(ItemFlag::IsHangable); }
  bool decays() const { return hasFlag(ItemFlag::Decays); }
  int8_t alwaysOnTopOrder() const { return always_on_top_order; }
};

// Aliasing ItemType to ItemDefinition for codebase compatibility
using ItemType = ItemDefinition;

} // namespace Domain
} // namespace MapEditor
