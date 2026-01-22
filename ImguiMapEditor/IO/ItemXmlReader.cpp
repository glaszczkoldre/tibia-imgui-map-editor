#include "ItemXmlReader.h"
#include "XmlUtils.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace MapEditor::IO {

ItemXmlResult ItemXmlReader::load(
    const std::filesystem::path& xml_path,
    std::vector<Domain::ItemType>& items,
    const std::unordered_map<uint16_t, size_t>& server_id_index) {
    
    ItemXmlResult result;
    pugi::xml_document doc;

    pugi::xml_node rootNode = XmlUtils::loadXmlFile(xml_path, "items", doc, result.error);
    if (!rootNode) {
        return result;
    }

    for (pugi::xml_node itemNode : rootNode.children("item")) {
        uint16_t id = 0;
        uint16_t fromId = 0;
        uint16_t toId = 0;

        if (auto attr = itemNode.attribute("id")) {
            id = static_cast<uint16_t>(attr.as_uint());
            if (applyToItem(id, itemNode, items, server_id_index, result.warnings)) {
                result.items_merged++;
            }
            result.items_loaded++;
        } else if (auto fromAttr = itemNode.attribute("fromid")) {
            fromId = static_cast<uint16_t>(fromAttr.as_uint());
            if (auto toAttr = itemNode.attribute("toid")) {
                toId = static_cast<uint16_t>(toAttr.as_uint());
                
                for (uint16_t currentId = fromId; currentId <= toId; ++currentId) {
                    if (applyToItem(currentId, itemNode, items, server_id_index, result.warnings)) {
                        result.items_merged++;
                    }
                    result.items_loaded++;
                }
            } else {
                result.warnings.push_back("Item node with fromid missing toid at offset " + 
                                          std::to_string(itemNode.offset_debug()));
            }
        } else {
            result.warnings.push_back("Item node missing id or fromid at offset " + 
                                      std::to_string(itemNode.offset_debug()));
        }
    }

    result.success = true;
    spdlog::info("[ItemXmlReader] Loaded {} definitions, merged {} with existing types", 
                 result.items_loaded, result.items_merged);
    
    return result;
}

bool ItemXmlReader::applyToItem(
    uint16_t id,
    const pugi::xml_node& itemNode,
    std::vector<Domain::ItemType>& items,
    const std::unordered_map<uint16_t, size_t>& server_id_index,
    std::vector<std::string>& warnings) {
    
    auto it = server_id_index.find(id);
    if (it == server_id_index.end()) {
        return false;
    }

    Domain::ItemType& item = items[it->second];

    // Basic attributes on the item node itself
    if (auto attr = itemNode.attribute("name")) {
        item.name = attr.as_string();
    }
    if (auto attr = itemNode.attribute("article")) {
        item.article = attr.as_string();
    }
    if (auto attr = itemNode.attribute("editorsuffix")) {
        item.editor_suffix = attr.as_string();
    }

    // Parse nested attributes
    parseAttributes(itemNode, item);

    item.xml_loaded = true;
    return true;
}

void ItemXmlReader::parseAttributes(const pugi::xml_node& itemNode, Domain::ItemType& item) {
    for (pugi::xml_node attrNode : itemNode.children("attribute")) {
        pugi::xml_attribute keyAttr = attrNode.attribute("key");
        pugi::xml_attribute valAttr = attrNode.attribute("value");
        if (!keyAttr || !valAttr) continue;

        std::string key = keyAttr.as_string();
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        std::string value = valAttr.as_string();

        if (key == "type") {
            std::transform(value.begin(), value.end(), value.begin(), ::tolower);
            if (value == "depot") item.item_type = Domain::ItemTypeEnum::Depot;
            else if (value == "mailbox") item.item_type = Domain::ItemTypeEnum::Mailbox;
            else if (value == "trashholder") item.item_type = Domain::ItemTypeEnum::TrashHolder;
            else if (value == "container") item.item_type = Domain::ItemTypeEnum::Container;
            else if (value == "door") item.item_type = Domain::ItemTypeEnum::Door;
            else if (value == "magicfield") item.item_type = Domain::ItemTypeEnum::MagicField;
            else if (value == "teleport") item.item_type = Domain::ItemTypeEnum::Teleport;
            else if (value == "bed") item.item_type = Domain::ItemTypeEnum::Bed;
            else if (value == "key") item.item_type = Domain::ItemTypeEnum::Key;
            else if (value == "podium") item.item_type = Domain::ItemTypeEnum::Podium;
        } 
        else if (key == "description") {
            item.description = value;
        } 
        else if (key == "weight") {
            item.weight = std::stof(value) / 100.0f;
        } 
        else if (key == "armor") {
            item.armor = static_cast<int16_t>(std::stoi(value));
        } 
        else if (key == "defense") {
            item.defense = static_cast<int16_t>(std::stoi(value));
        } 
        else if (key == "attack") {
            item.attack = static_cast<int16_t>(std::stoi(value));
        }
        else if (key == "range" || key == "shootrange") {
             item.shootRange = static_cast<uint8_t>(std::stoi(value));
        }
        else if (key == "floorchange") {
            std::transform(value.begin(), value.end(), value.begin(), ::tolower);
            item.floor_change = true;
            if (value == "down") item.floor_change_down = true;
            else if (value == "north") item.floor_change_north = true;
            else if (value == "south") item.floor_change_south = true;
            else if (value == "east") item.floor_change_east = true;
            else if (value == "west") item.floor_change_west = true;
            else if (value == "northex") item.floor_change_north_ex = true;
            else if (value == "southex") item.floor_change_south_ex = true;
            else if (value == "eastex") item.floor_change_east_ex = true;
            else if (value == "westex") item.floor_change_west_ex = true;
        }
        else if (key == "slottype") {
            std::transform(value.begin(), value.end(), value.begin(), ::tolower);
            if (value == "head") item.slot_position = Domain::SlotPosition::Head;
            else if (value == "necklace") item.slot_position = Domain::SlotPosition::Necklace;
            else if (value == "backpack") item.slot_position = Domain::SlotPosition::Backpack;
            else if (value == "body" || value == "armor") item.slot_position = Domain::SlotPosition::Armor;
            else if (value == "hand") item.slot_position = Domain::SlotPosition::Hand;
            else if (value == "legs") item.slot_position = Domain::SlotPosition::Legs;
            else if (value == "feet") item.slot_position = Domain::SlotPosition::Feet;
            else if (value == "ring") item.slot_position = Domain::SlotPosition::Ring;
            else if (value == "ammo") item.slot_position = Domain::SlotPosition::Ammo;
            else if (value == "two-handed") item.slot_position = Domain::SlotPosition::TwoHand;
        }
        else if (key == "weapontype") {
            std::transform(value.begin(), value.end(), value.begin(), ::tolower);
            if (value == "sword") item.weapon_type = Domain::WeaponType::Sword;
            else if (value == "club") item.weapon_type = Domain::WeaponType::Club;
            else if (value == "axe") item.weapon_type = Domain::WeaponType::Axe;
            else if (value == "shield") item.weapon_type = Domain::WeaponType::Shield;
            else if (value == "distance") item.weapon_type = Domain::WeaponType::Distance;
            else if (value == "wand") item.weapon_type = Domain::WeaponType::Wand;
            else if (value == "ammunition") item.weapon_type = Domain::WeaponType::Ammo;
        }
        else if (key == "ammotype") {
            item.ammoType = value; // Store string representation
        }
        else if (key == "containersize") {
            item.volume = static_cast<uint16_t>(std::stoi(value));
        }
        else if (key == "rotateto") {
            item.rotateTo = static_cast<uint16_t>(std::stoul(value));
            item.flags = item.flags | Domain::ItemFlag::Rotatable;
        }
        else if (key == "readable") {
            item.can_read_text = (value == "1" || value == "true");
            if (item.can_read_text) item.flags = item.flags | Domain::ItemFlag::Readable;
        }
        else if (key == "writeable") {
            item.can_write_text = (value == "1" || value == "true");
        }
        else if (key == "maxtextlen") {
            item.maxTextLen = static_cast<uint16_t>(std::stoi(value));
        }
        else if (key == "allowdistread") {
            item.allow_dist_read = (value == "1" || value == "true");
            if (item.allow_dist_read) item.flags = item.flags | Domain::ItemFlag::AllowDistRead;
        }
        else if (key == "lightlevel") {
            item.light_level = static_cast<uint8_t>(std::stoi(value));
        }
        else if (key == "lightcolor") {
            item.light_color = static_cast<uint8_t>(std::stoi(value));
        }
        else if (key == "speed") {
            item.speed = static_cast<uint16_t>(std::stoi(value));
        }
        else if (key == "charges") {
            item.charges = static_cast<uint32_t>(std::stoul(value));
        }
        else if (key == "showcharges") {
            item.extra_chargeable = (value == "1" || value == "true");
        }
        else if (key == "decayto") {
            item.decayTo = static_cast<uint16_t>(std::stoul(value));
        }
        else if (key == "duration" || key == "stopduration") {
            item.stopDuration = static_cast<uint32_t>(std::stoul(value));
        }
        else if (key == "minimapcolor") {
            item.minimap_color = static_cast<uint16_t>(std::stoi(value));
        }
        else if (key == "pickupable") {
            item.is_pickupable = (value == "1" || value == "true");
            if (item.is_pickupable) item.flags = item.flags | Domain::ItemFlag::Pickupable;
        }
        else if (key == "unpassable") {
            bool val = (value == "1" || value == "true");
            if (val) item.flags = item.flags | Domain::ItemFlag::Unpassable;
        }
        else if (key == "blockprojectile") {
            item.blocks_projectile = (value == "1" || value == "true");
            if (item.blocks_projectile) item.flags = item.flags | Domain::ItemFlag::BlockMissiles;
        }
        else if (key == "walkstack") {
            // "walkstack" usually implies blocking pathfinder or order
            bool val = (value == "1" || value == "true");
            if (val) item.flags = item.flags | Domain::ItemFlag::BlockPathfinder;
        }
        else if (key == "alwaysontop") {
            bool val = (value == "1" || value == "true");
            if (val) item.flags = item.flags | Domain::ItemFlag::AlwaysOnTop;
        }
    }
}

} // namespace MapEditor::IO
