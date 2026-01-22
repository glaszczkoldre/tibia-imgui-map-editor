#include "SrvReader.h"
#include "ScriptReader.h"
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <memory>

namespace MapEditor {
namespace IO {

SrvResult SrvReader::read(const std::filesystem::path& path) {
    SrvResult result;
    
    ScriptReader script;
    if (!script.open(path.string())) {
        result.error = "Failed to open file: " + path.string();
        return result;
    }
    
    std::unique_ptr<Domain::ItemType> currentItem = nullptr;
    uint16_t maxItemId = 0;
    
    while (true) {
        script.nextToken();
        
        if (script.token == TokenType::EndOfFile) {
            break;
        }
        
        if (script.token != TokenType::Identifier) {
            script.error("Identifier expected");
            result.error = "Parse error: identifier expected";
            return result;
        }
        
        std::string identifier = script.getIdentifier();
        script.readSymbol('=');
        
        if (identifier == "typeid") {
            // Save previous item if exists
            if (currentItem && currentItem->server_id > 0) {
                result.items.push_back(std::move(*currentItem));
            }
            
            // Create new item
            currentItem = std::make_unique<Domain::ItemType>();
            uint16_t id = static_cast<uint16_t>(script.readNumber());
            currentItem->server_id = id;
            currentItem->client_id = id;  // SRV format: server_id == client_id
            
            if (id > maxItemId) {
                maxItemId = id;
            }
        }
        else if (identifier == "name") {
            if (currentItem) {
                currentItem->name = script.readString();
            }
        }
        else if (identifier == "description") {
            if (currentItem) {
                currentItem->description = script.readString();
            }
        }
        else if (identifier == "flags") {
            if (!currentItem) continue;
            
            script.readSymbol('{');
            
            while (true) {
                script.nextToken();
                
                if (script.token == TokenType::Special && script.getSpecial() == '}') {
                    break;
                }
                
                if (script.token == TokenType::Identifier) {
                    std::string flag = script.getIdentifier();
                    
                    // Map flags to ItemType properties
                    if (flag == "bank") {
                        currentItem->group = Domain::ItemGroup::Ground;
                    }
                    else if (flag == "clip") {
                        currentItem->always_on_bottom = true;
                        currentItem->top_order = 1;
                    }
                    else if (flag == "bottom") {
                        currentItem->always_on_bottom = true;
                        currentItem->top_order = 2;
                    }
                    else if (flag == "top") {
                        currentItem->always_on_bottom = true;
                        currentItem->top_order = 3;
                    }
                    else if (flag == "container" || flag == "chest") {
                        currentItem->group = Domain::ItemGroup::Container;
                        if (flag == "chest") {
                            currentItem->volume = 1;
                        }
                    }
                    else if (flag == "cumulative") {
                        currentItem->is_stackable = true;
                        currentItem->flags = currentItem->flags | Domain::ItemFlag::Stackable;
                    }
                    else if (flag == "key") {
                        currentItem->group = Domain::ItemGroup::Key;
                    }
                    else if (flag == "door") {
                        currentItem->group = Domain::ItemGroup::Door;
                    }
                    else if (flag == "bed") {
                        currentItem->item_type = Domain::ItemTypeEnum::Bed;
                    }
                    else if (flag == "rune") {
                        currentItem->flags = currentItem->flags | Domain::ItemFlag::ClientCharges;
                    }
                    else if (flag == "depotlocker") {
                        currentItem->item_type = Domain::ItemTypeEnum::Depot;
                    }
                    else if (flag == "mailbox") {
                        currentItem->item_type = Domain::ItemTypeEnum::Mailbox;
                    }
                    else if (flag == "allowdistread") {
                        currentItem->allow_dist_read = true;
                    }
                    else if (flag == "text") {
                        currentItem->can_read_text = true;
                    }
                    else if (flag == "write" || flag == "writeonce") {
                        currentItem->can_write_text = true;
                        currentItem->can_read_text = true;
                    }
                    else if (flag == "fluidcontainer") {
                        currentItem->group = Domain::ItemGroup::Fluid;
                        currentItem->is_fluid_container = true;
                    }
                    else if (flag == "splash") {
                        currentItem->group = Domain::ItemGroup::Splash;
                    }
                    else if (flag == "unpass") {
                        currentItem->is_blocking = true;
                        currentItem->flags = currentItem->flags | Domain::ItemFlag::Unpassable;
                    }
                    else if (flag == "unmove") {
                        currentItem->is_moveable = false;
                    }
                    else if (flag == "unthrow") {
                        currentItem->flags = currentItem->flags | Domain::ItemFlag::BlockMissiles;
                    }
                    else if (flag == "avoid") {
                        currentItem->flags = currentItem->flags | Domain::ItemFlag::BlockPathfinder;
                    }
                    else if (flag == "magicfield") {
                        currentItem->group = Domain::ItemGroup::MagicField;
                    }
                    else if (flag == "take") {
                        currentItem->is_pickupable = true;
                        currentItem->flags = currentItem->flags | Domain::ItemFlag::Pickupable;
                    }
                    else if (flag == "hang") {
                        currentItem->is_hangable = true;
                        currentItem->flags = currentItem->flags | Domain::ItemFlag::Hangable;
                    }
                    else if (flag == "hooksouth") {
                        currentItem->hook_south = true;
                        currentItem->flags = currentItem->flags | Domain::ItemFlag::HookSouth;
                    }
                    else if (flag == "hookeast") {
                        currentItem->hook_east = true;
                        currentItem->flags = currentItem->flags | Domain::ItemFlag::HookEast;
                    }
                    else if (flag == "rotate") {
                        currentItem->flags = currentItem->flags | Domain::ItemFlag::Rotatable;
                    }
                    else if (flag == "weapon") {
                        currentItem->group = Domain::ItemGroup::Weapon;
                    }
                    else if (flag == "armor") {
                        currentItem->group = Domain::ItemGroup::Armor;
                    }
                    else if (flag == "disguise") {
                        // Will be handled with disguisetarget attribute
                    }
                    // Skip unknown flags silently
                }
                
                // Skip commas between flags
                if (script.token == TokenType::Special && script.getSpecial() == ',') {
                    continue;
                }
            }
        }
        else if (identifier == "attributes") {
            if (!currentItem) continue;
            
            script.readSymbol('{');
            
            while (true) {
                script.nextToken();
                
                if (script.token == TokenType::Special && script.getSpecial() == '}') {
                    break;
                }
                
                if (script.token == TokenType::Identifier) {
                    std::string attr = script.getIdentifier();
                    script.readSymbol('=');
                    
                    if (attr == "capacity") {
                        currentItem->volume = static_cast<uint16_t>(script.readNumber());
                    }
                    else if (attr == "weight") {
                        currentItem->weight = static_cast<float>(script.readNumber());
                    }
                    else if (attr == "rotatetarget") {
                        currentItem->rotateTo = static_cast<uint16_t>(script.readNumber());
                    }
                    else if (attr == "maxlength") {
                        currentItem->maxTextLen = static_cast<uint16_t>(script.readNumber());
                    }
                    else if (attr == "attack") {
                        currentItem->attack = static_cast<int16_t>(script.readNumber());
                    }
                    else if (attr == "defense") {
                        currentItem->defense = static_cast<int16_t>(script.readNumber());
                    }
                    else if (attr == "armorvalue") {
                        currentItem->armor = static_cast<int16_t>(script.readNumber());
                    }
                    else if (attr == "totaluses") {
                        currentItem->charges = static_cast<uint32_t>(script.readNumber());
                    }
                    else if (attr == "disguisetarget") {
                        currentItem->disguise_target = static_cast<uint16_t>(script.readNumber());
                    }
                    else {
                        // Skip unknown attributes - read value
                        script.nextToken();
                        // Could be number, string, or identifier
                    }
                }
                
                // Skip commas between attributes
                if (script.token == TokenType::Special && script.getSpecial() == ',') {
                    continue;
                }
            }
        }
        else if (identifier == "magicfield") {
            // Skip magicfield blocks - not needed for editor
            script.readSymbol('{');
            int depth = 1;
            while (depth > 0) {
                script.nextToken();
                if (script.token == TokenType::Special) {
                    if (script.getSpecial() == '{') depth++;
                    else if (script.getSpecial() == '}') depth--;
                }
                if (script.token == TokenType::EndOfFile) break;
            }
        }
    }
    
    // Save last item
    if (currentItem && currentItem->server_id > 0) {
        result.items.push_back(std::move(*currentItem));
    }
    
    script.close();
    
    spdlog::info("SrvReader: Loaded {} items from SRV (max id: {})", 
                 result.items.size(), maxItemId);
    
    // Post-process: Apply disguise by copying target's client_id
    // Build a map of server_id -> index for quick lookup
    std::unordered_map<uint16_t, size_t> id_to_index;
    id_to_index.reserve(result.items.size());
    for (size_t index = 0; const auto& item : result.items) {
        id_to_index[item.server_id] = index++;
    }
    
    int disguise_count = 0;
    for (auto& item : result.items) {
        if (item.disguise_target > 0) {
            if (auto it = id_to_index.find(item.disguise_target); it != id_to_index.end()) {
                // Copy client_id from target item to use its appearance
                item.client_id = result.items[it->second].client_id;
                ++disguise_count;
            } else {
                spdlog::warn("SrvReader: Item {} has DisguiseTarget={} but target not found",
                             item.server_id, item.disguise_target);
            }
        }
    }
    
    if (disguise_count > 0) {
        spdlog::info("SrvReader: Applied {} disguise mappings", disguise_count);
    }
    
    result.success = true;
    return result;
}

} // namespace IO
} // namespace MapEditor
