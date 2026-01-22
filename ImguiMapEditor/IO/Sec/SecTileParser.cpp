#include "SecTileParser.h"
#include "SecItemParser.h"
#include "../SecReader.h"
#include "Domain/Tile.h"
#include "Services/ClientDataService.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>

namespace MapEditor {
namespace IO {

namespace {

// Convert string to lowercase for case-insensitive comparison
std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

} // anonymous namespace

bool SecTileParser::parseSector(
    ScriptReader& script,
    int sector_x, int sector_y, int sector_z,
    Domain::ChunkedMap& map,
    Services::ClientDataService* client_data,
    SecResult& result) {
    
    int current_offset_x = -1;
    int current_offset_y = -1;
    Domain::Tile* current_tile = nullptr;
    
    while (true) {
        script.nextToken();
        
        if (script.token == TokenType::EndOfFile) {
            break;
        }
        
        // Skip commas between elements
        if (script.token == TokenType::Special && script.getSpecial() == ',') {
            continue;
        }
        
        // Parse tile coordinate: X-Y: (now parsed as BYTES token like [X, Y])
        // Reference: tibia-game map.cc lines 969-976
        if (script.token == TokenType::Bytes) {
            const auto& bytes = script.getBytes();
            if (bytes.size() >= 2) {
                current_offset_x = bytes[0];
                current_offset_y = bytes[1];
            } else {
                spdlog::warn("SecTileParser: Invalid coordinate byte sequence size: {}", bytes.size());
                continue;
            }
            
            script.readSymbol(':');
            
            // Calculate world position
            int world_x = sector_x * 32 + current_offset_x;
            int world_y = sector_y * 32 + current_offset_y;
            
            Domain::Position pos(world_x, world_y, sector_z);
            current_tile = map.getOrCreateTile(pos);
            
            if (current_tile) {
                result.tile_count++;
            }
            
            continue;
        }
        
        // Parse tile flags or content
        if (script.token == TokenType::Identifier && current_tile) {
            std::string id = toLower(script.getIdentifier());
            
            // Check for tile flags
            uint32_t flag = parseTileFlag(id);
            if (flag != 0) {
                current_tile->setFlags(static_cast<uint32_t>(current_tile->getFlags()) | flag);
                continue;
            }
            
            // Check for content block
            if (id == "content") {
                script.readSymbol('=');
                script.readSymbol('{');
                
                auto items = SecItemParser::parseItemList(script, client_data);
                
                // Add items to tile (ground first, then stacked)
                for (auto& item : items) {
                    if (item) {
                        current_tile->addItem(std::move(item));
                        result.item_count++;
                    }
                }
                
                continue;
            }
            
            spdlog::trace("SecTileParser: Unknown identifier '{}' at {}-{}", 
                         id, current_offset_x, current_offset_y);
        }
    }
    
    return true;
}

uint32_t SecTileParser::parseTileFlag(const std::string& identifier) {
    // Map SEC flag names to Domain::TileFlag enum values
    
    if (identifier == "refresh") {
        return static_cast<uint32_t>(Domain::TileFlag::Refresh);
    }
    if (identifier == "nologout") {
        return static_cast<uint32_t>(Domain::TileFlag::NoLogout);
    }
    if (identifier == "protectionzone") {
        return static_cast<uint32_t>(Domain::TileFlag::ProtectionZone);
    }
    
    return 0;
}

} // namespace IO
} // namespace MapEditor
