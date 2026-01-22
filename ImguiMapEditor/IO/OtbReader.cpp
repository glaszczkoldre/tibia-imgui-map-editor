#include "OtbReader.h"
#include "NodeFileReader.h"
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace IO {

// Map OTB group byte to ItemGroup enum
static Domain::ItemGroup mapGroup(uint8_t group) {
    switch (group) {
        case 0: return Domain::ItemGroup::None;
        case 1: return Domain::ItemGroup::Ground;
        case 2: return Domain::ItemGroup::Container;
        case 3: return Domain::ItemGroup::Weapon;
        case 4: return Domain::ItemGroup::Ammunition;
        case 5: return Domain::ItemGroup::Armor;
        case 6: return Domain::ItemGroup::Changes;
        case 7: return Domain::ItemGroup::Teleport;
        case 8: return Domain::ItemGroup::MagicField;
        case 9: return Domain::ItemGroup::Writeable;
        case 10: return Domain::ItemGroup::Key;
        case 11: return Domain::ItemGroup::Splash;
        case 12: return Domain::ItemGroup::Fluid;
        case 13: return Domain::ItemGroup::Door;
        case 14: return Domain::ItemGroup::Deprecated;
        case 15: return Domain::ItemGroup::Podium;
        default: return Domain::ItemGroup::None;
    }
}

// Parse flags from 32-bit value
static Domain::ItemFlag parseFlags(uint32_t flags) {
    Domain::ItemFlag result = Domain::ItemFlag::None;
    
    if (flags & (1 << 0)) result = result | Domain::ItemFlag::Unpassable;
    if (flags & (1 << 1)) result = result | Domain::ItemFlag::BlockMissiles;
    if (flags & (1 << 2)) result = result | Domain::ItemFlag::BlockPathfinder;
    if (flags & (1 << 3)) result = result | Domain::ItemFlag::HasElevation;
    if (flags & (1 << 4)) result = result | Domain::ItemFlag::Useable;
    if (flags & (1 << 5)) result = result | Domain::ItemFlag::Pickupable;
    if (flags & (1 << 6)) result = result | Domain::ItemFlag::Moveable;
    if (flags & (1 << 7)) result = result | Domain::ItemFlag::Stackable;
    if (flags & (1 << 8)) result = result | Domain::ItemFlag::FloorChangeDown;
    if (flags & (1 << 9)) result = result | Domain::ItemFlag::FloorChangeNorth;
    if (flags & (1 << 10)) result = result | Domain::ItemFlag::FloorChangeEast;
    if (flags & (1 << 11)) result = result | Domain::ItemFlag::FloorChangeSouth;
    if (flags & (1 << 12)) result = result | Domain::ItemFlag::FloorChangeWest;
    if (flags & (1 << 13)) result = result | Domain::ItemFlag::AlwaysOnTop;
    if (flags & (1 << 14)) result = result | Domain::ItemFlag::Readable;
    if (flags & (1 << 15)) result = result | Domain::ItemFlag::Rotatable;
    if (flags & (1 << 16)) result = result | Domain::ItemFlag::Hangable;
    if (flags & (1 << 17)) result = result | Domain::ItemFlag::HookEast;
    if (flags & (1 << 18)) result = result | Domain::ItemFlag::HookSouth;
    if (flags & (1 << 19)) result = result | Domain::ItemFlag::CanNotDecay;
    if (flags & (1 << 20)) result = result | Domain::ItemFlag::AllowDistRead;
    if (flags & (1 << 22)) result = result | Domain::ItemFlag::ClientCharges;
    if (flags & (1 << 23)) result = result | Domain::ItemFlag::IgnoreLook;
    if (flags & (1 << 24)) result = result | Domain::ItemFlag::Animation;
    if (flags & (1 << 25)) result = result | Domain::ItemFlag::FullTile;
    if (flags & (1 << 26)) result = result | Domain::ItemFlag::ForceUse;
    
    return result;
}

OtbResult OtbReader::read(const std::filesystem::path& path) {
    OtbResult result;
    
    // Open OTB file - accepts both "OTBI" and null identifier
    DiskNodeFileReadHandle file(path, {"OTBI", std::string(4, '\0')});
    if (!file.isOk()) {
        result.error = "Failed to open file: " + file.getErrorMessage();
        return result;
    }
    
    // Read root node
    BinaryNode* root = file.getRootNode();
    if (!root) {
        result.error = "Failed to read root node";
        return result;
    }
    
    // Skip first byte (0) and unused flags (4 bytes)
    root->skip(1);
    uint32_t unused_flags;
    root->getU32(unused_flags);
    
    // Read version info
    uint8_t attr;
    if (root->getU8(attr) && attr == static_cast<uint8_t>(RootAttribute::Version)) {
        uint16_t data_len;
        if (root->getU16(data_len) && data_len >= 12) {
            root->getU32(result.version.major_version);
            root->getU32(result.version.minor_version);
            root->getU32(result.version.build_number);
            result.version.valid = true;
            
            // Skip remaining header bytes
            if (data_len > 12) {
                root->skip(data_len - 12);
            }
        }
    }
    
    // Read child nodes (items)
    size_t item_count = 0;
    
    for (BinaryNode* node = root->getChild(); node != nullptr; node = node->advance()) {
        try {
            Domain::ItemType item;
            
            // Read group
            uint8_t group;
            if (!node->getU8(group)) continue;
            item.group = mapGroup(group);
            
            // Read flags
            uint32_t flags;
            if (!node->getU32(flags)) continue;
            item.flags = parseFlags(flags);
            
            // Derive properties from flags
            item.is_blocking = Domain::hasFlag(item.flags, Domain::ItemFlag::Unpassable);
            item.is_moveable = !Domain::hasFlag(item.flags, Domain::ItemFlag::Moveable);
            item.is_pickupable = Domain::hasFlag(item.flags, Domain::ItemFlag::Pickupable);
            item.is_stackable = Domain::hasFlag(item.flags, Domain::ItemFlag::Stackable);
            
            // Rendering order flags (note: AlwaysOnTop in OTB actually means "always on bottom")
            item.always_on_bottom = Domain::hasFlag(item.flags, Domain::ItemFlag::AlwaysOnTop);
            
            // Hangable/hook properties (used for wall-mounted items)
            item.is_hangable = Domain::hasFlag(item.flags, Domain::ItemFlag::Hangable);
            item.hook_east = Domain::hasFlag(item.flags, Domain::ItemFlag::HookEast);
            item.hook_south = Domain::hasFlag(item.flags, Domain::ItemFlag::HookSouth);
            
            // Read attributes
            uint8_t attr_type;
            while (node->getU8(attr_type)) {
                uint16_t len;
                if (!node->getU16(len)) break;
                
                switch (static_cast<OtbAttribute>(attr_type)) {
                    case OtbAttribute::ServerId: {
                        uint16_t sid;
                        if (node->getU16(sid)) {
                            item.server_id = sid;
                        }
                        break;
                    }
                    case OtbAttribute::ClientId: {
                        uint16_t cid;
                        if (node->getU16(cid)) {
                            item.client_id = cid;
                        }
                        break;
                    }
                    case OtbAttribute::Name: {
                        std::string name;
                        if (node->getString(name)) {
                            item.name = name;
                        }
                        break;
                    }
                    case OtbAttribute::Speed: {
                        uint16_t speed;
                        if (node->getU16(speed)) {
                            item.speed = speed;
                            item.ground_speed = static_cast<uint8_t>(std::min(speed, uint16_t(255)));
                        }
                        break;
                    }
                    case OtbAttribute::Light: {
                        uint16_t level, color;
                        if (node->getU16(level) && node->getU16(color)) {
                            item.light_level = static_cast<uint8_t>(level);
                            item.light_color = static_cast<uint8_t>(color);
                        }
                        break;
                    }
                    case OtbAttribute::StackOrder: {
                        uint8_t order;
                        if (node->getU8(order)) {
                            item.top_order = static_cast<int8_t>(order);
                        }
                        break;
                    }
                    case OtbAttribute::TradeAs: {
                        uint16_t wareId;
                        if (node->getU16(wareId)) {
                            item.wareId = wareId;
                        }
                        break;
                    }
                    case OtbAttribute::MaxReadWriteChars: {
                        uint16_t maxLen;
                        if (node->getU16(maxLen)) {
                            item.maxTextLen = maxLen;
                        }
                        break;
                    }
                    default:
                        // Skip unknown attributes
                        if (len > 0) {
                            node->skip(len);
                        }
                        break;
                }
            }
            
            // Only add items with valid server IDs
            if (item.server_id > 0) {
                result.items.push_back(std::move(item));
            }
            
            item_count++;
        } catch (const std::exception& e) {
            spdlog::warn("Error reading OTB item {}: {}", item_count, e.what());
        }
    }
    
    spdlog::info("Loaded {} items from OTB", result.items.size());
    result.success = true;
    return result;
}

OtbVersionInfo OtbReader::readVersionInfo(const std::filesystem::path& path) {
    OtbVersionInfo info;
    
    try {
        DiskNodeFileReadHandle file(path, {"OTBI", std::string(4, '\0')});
        if (!file.isOk()) {
            return info;
        }
        
        BinaryNode* root = file.getRootNode();
        if (!root) {
            return info;
        }
        
        root->skip(1);   // Skip 0
        root->skip(4);   // Skip unused flags
        
        uint8_t attr;
        if (root->getU8(attr) && attr == static_cast<uint8_t>(RootAttribute::Version)) {
            uint16_t data_len;
            if (root->getU16(data_len) && data_len >= 12) {
                root->getU32(info.major_version);
                root->getU32(info.minor_version);
                root->getU32(info.build_number);
                info.valid = true;
            }
        }
    } catch (...) {
        info.valid = false;
    }
    
    return info;
}

} // namespace IO
} // namespace MapEditor
