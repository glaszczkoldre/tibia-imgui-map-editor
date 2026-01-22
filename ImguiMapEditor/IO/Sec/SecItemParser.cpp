#include "SecItemParser.h"
#include "Services/ClientDataService.h"
#include "Domain/ItemType.h"
#include "Domain/Position.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>

namespace MapEditor {
namespace IO {

namespace {

std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

} // anonymous namespace

std::vector<std::unique_ptr<Domain::Item>> SecItemParser::parseItemList(
    ScriptReader& script,
    Services::ClientDataService* client_data) {
    
    std::vector<std::unique_ptr<Domain::Item>> items;
    
    // We're positioned right after the '{' of Content={
    // Format: Content={ItemID1 [Attr=Val], ItemID2 [Attr=Val], ...}
    
    script.nextToken();
    
    while (script.token != TokenType::EndOfFile) {
        // End of content block
        if (script.token == TokenType::Special && script.getSpecial() == '}') {
            break;
        }
        
        // Skip commas between items
        if (script.token == TokenType::Special && script.getSpecial() == ',') {
            script.nextToken();
            continue;
        }
        
        // Parse item by server ID - use shared parseItem function
        if (script.token == TokenType::Number) {
            int server_id = script.getNumber();
            auto item = parseItem(server_id, script, client_data);
            if (item) {
                items.push_back(std::move(item));
            }
            // parseItem leaves script.token at the next separator or }
            continue;
        }
        
        // Skip any unexpected tokens
        script.nextToken();
    }
    
    return items;
}

std::unique_ptr<Domain::Item> SecItemParser::parseItem(
    int server_id,
    ScriptReader& script,
    Services::ClientDataService* client_data) {
    
    // The server_id has already been read from the Number token
    // We need to advance past attributes and containers even for invalid IDs
    // to avoid infinite loops
    
    // Advance to next token (past the Number we just consumed)
    script.nextToken();
    
    if (server_id <= 0 || server_id > 65535) {
        spdlog::warn("SecItemParser: Invalid item ID {}", server_id);
        // Still need to skip any attributes or container that might follow
        // Skip identifiers
        while (script.token == TokenType::Identifier) {
            script.nextToken();
        }
        // Skip container block if present
        if (script.token == TokenType::Special && script.getSpecial() == '{') {
            int depth = 1;
            while (depth > 0 && script.token != TokenType::EndOfFile) {
                script.nextToken();
                if (script.token == TokenType::Special) {
                    if (script.getSpecial() == '{') depth++;
                    else if (script.getSpecial() == '}') depth--;
                }
            }
            if (script.token == TokenType::Special && script.getSpecial() == '}') {
                script.nextToken();
            }
        }
        return nullptr;
    }
    
    auto item = std::make_unique<Domain::Item>(static_cast<uint16_t>(server_id));
    
    // Look up item type from client data (items.srv)
    if (client_data) {
        if (const auto* item_type = client_data->getItemTypeByServerId(static_cast<uint16_t>(server_id))) {
            item->setType(item_type);
            item->setClientId(item_type->client_id);
        }
    }
    
    // Parse optional attributes (token already advanced above)
    // Reference: SEC InstanceAttributeNames from objects.cc
    // Content, ChestQuestNumber, Amount, KeyNumber, KeyholeNumber, Level,
    // DoorQuestNumber, DoorQuestValue, Charges, String, Editor,
    // ContainerLiquidType, PoolLiquidType, AbsTeleportDestination,
    // Responsible, RemainingExpireTime, SavedExpireTime, RemainingUses
    while (script.token == TokenType::Identifier) {
        std::string attr_name = toLower(script.getIdentifier());
        
        // Text attributes (String="...", Editor="...")
        if (attr_name == "string") {
            script.readSymbol('=');
            std::string text = script.readString();
            item->setText(text);
        } else if (attr_name == "editor") {
            script.readSymbol('=');
            std::string desc = script.readString();
            item->setDescription(desc);
        }
        // Numeric attributes - map to Item properties
        else if (attr_name == "amount") {
            script.readSymbol('=');
            int amount = script.readNumber();
            item->setCount(static_cast<uint16_t>(amount));
        } else if (attr_name == "charges" || attr_name == "remaininguses") {
            script.readSymbol('=');
            int charges = script.readNumber();
            item->setCharges(static_cast<uint8_t>(charges));
        } else if (attr_name == "remainingexpiretime" || attr_name == "savedexpiretime") {
            script.readSymbol('=');
            int time = script.readNumber();
            item->setDuration(static_cast<uint16_t>(time));
        } else if (attr_name == "keyholenumber" || attr_name == "keynumber") {
            script.readSymbol('=');
            int door_id = script.readNumber();
            item->setDoorId(static_cast<uint32_t>(door_id));
        } else if (attr_name == "containerliquidtype" || attr_name == "poolliquidtype") {
            script.readSymbol('=');
            int liquid = script.readNumber();
            item->setSubtype(static_cast<uint16_t>(liquid));
        } else if (attr_name == "absteleportdestination") {
            // Format: AbsTeleportDestination=PackedInt32
            // Reference: tibia-game moveuse.cc UnpackAbsoluteCoordinate
            // x = ((Packed >> 18) & 0x3FFF) + 24576
            // y = ((Packed >>  4) & 0x3FFF) + 24576
            // z = ((Packed >>  0) & 0x000F)
            script.readSymbol('=');
            int packed = script.readNumber();
            uint16_t x = static_cast<uint16_t>(((packed >> 18) & 0x3FFF) + 24576);
            uint16_t y = static_cast<uint16_t>(((packed >> 4) & 0x3FFF) + 24576);
            uint8_t z = static_cast<uint8_t>(packed & 0x0F);
            Domain::Position dest{x, y, z};
            item->setTeleportDestination(dest);
        }
        // Content is handled separately (container items)
        else if (attr_name == "content") {
            // Content={...} - will be parsed in container block below
            // Just consume the '=' and let the '{' be handled below
            script.readSymbol('=');
            break; // Exit attribute loop, let container parsing handle it
        }
        // Other numeric attributes we don't use but must consume
        else if (attr_name == "level" || attr_name == "doorquestnumber" || 
                 attr_name == "doorquestvalue" || attr_name == "chestquestnumber" ||
                 attr_name == "responsible") {
            script.readSymbol('=');
            script.readNumber(); // Consume value but don't store
            spdlog::trace("SecItemParser: Skipping unused attribute '{}' for item {}", attr_name, server_id);
        } else {
            // Unknown attribute - try to skip it gracefully
            spdlog::trace("SecItemParser: Unknown attribute '{}' for item {}", attr_name, server_id);
            // Try to consume '=' and value if present
            script.nextToken();
            if (script.token == TokenType::Special && script.getSpecial() == '=') {
                script.nextToken(); // Try to consume the value
            }
            continue; // Don't call nextToken again at end of loop
        }
        
        script.nextToken();
    }
    
    // Check for nested container contents
    if (script.token == TokenType::Special && script.getSpecial() == '{') {
        parseContainerContents(script, *item, client_data);
        script.nextToken();
    }
    
    return item;
}

void SecItemParser::parseContainerContents(
    ScriptReader& script,
    Domain::Item& container,
    Services::ClientDataService* client_data) {
    
    // We're positioned at the opening '{'
    // Parse items until we hit '}'
    
    script.nextToken();
    
    while (script.token != TokenType::EndOfFile) {
        if (script.token == TokenType::Special && script.getSpecial() == '}') {
            break;
        }
        
        if (script.token == TokenType::Special && script.getSpecial() == ',') {
            script.nextToken();
            continue;
        }
        
        if (script.token == TokenType::Number) {
            int server_id = script.getNumber();
            auto item = parseItem(server_id, script, client_data);
            if (item) {
                container.addContainerItem(std::move(item));
            }
            // parseItem leaves script.token at the next separator or }
            continue;
        }
        
        // Skip unexpected tokens
        script.nextToken();
    }
}

} // namespace IO
} // namespace MapEditor
