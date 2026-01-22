#include "OtbmWriter.h"
#include "../NodeFileWriter.h"
#include "../HouseXmlWriter.h"
#include "../SpawnXmlWriter.h"
#include "Domain/Tile.h"
#include "Domain/Item.h"
#include "Domain/ItemType.h"
#include "Services/ClientDataService.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <map>
#include <tuple>
#include <algorithm>

namespace MapEditor::IO {

namespace {

// Conversion context passed through write functions
struct ConversionContext {
    OtbmConversionMode mode = OtbmConversionMode::None;
    Services::ClientDataService* client_data = nullptr;
    size_t items_converted = 0;
    size_t items_skipped = 0;
};

// Helper to convert ID based on mode
uint16_t convertItemId(uint16_t original_id, ConversionContext& ctx) {
    if (ctx.mode == OtbmConversionMode::None || !ctx.client_data) {
        return original_id;
    }
    
    if (original_id == 0) {
        return 0;
    }
    
    if (ctx.mode == OtbmConversionMode::ToClient) {
        // Server ID -> Client ID
        const auto* item_type = ctx.client_data->getItemTypeByServerId(original_id);
        if (item_type && item_type->client_id > 0) {
            ctx.items_converted++;
            return item_type->client_id;
        }
    } else if (ctx.mode == OtbmConversionMode::ToServer) {
        // Client ID -> Server ID
        const auto* item_type = ctx.client_data->getItemTypeByClientId(original_id);
        if (item_type && item_type->server_id > 0) {
            ctx.items_converted++;
            return item_type->server_id;
        }
    }
    
    // No mapping found, keep original
    ctx.items_skipped++;
    return original_id;
}

bool writeItem(NodeFileWriteHandle& writer, const Domain::Item* item, OtbmVersion version, ConversionContext& ctx) {
    if (!item) return false;
    
    // Convert ID at write time
    uint16_t id_to_write = convertItemId(item->getServerId(), ctx);
    
    writer.startNode(static_cast<uint8_t>(OtbmNode::Item));
    writer.writeU16(id_to_write);
    
    // Subtype/Count - must write for certain item types (matches RME behavior)
    // - Stackable: count of items (only if > 1, default is 1)
    // - Splash/Fluid: fluid type (ALWAYS write, even if 1 = water)
    const auto* item_type = item->getType();
    uint16_t subtype = item->getCount();
    
    bool is_stackable = item_type && item_type->is_stackable;
    bool is_splash = item_type && item_type->isSplash();
    bool is_fluid = item_type && item_type->isFluidContainer();
    
    if (is_splash || is_fluid) {
        // Always write fluid type (even if 1 = water)
        writer.writeU8(static_cast<uint8_t>(OtbmAttribute::Count));
        writer.writeU8(static_cast<uint8_t>(subtype));
    } else if (is_stackable && subtype > 1) {
        // Only write stack count if > 1 (default is 1)
        writer.writeU8(static_cast<uint8_t>(OtbmAttribute::Count));
        writer.writeU8(static_cast<uint8_t>(subtype));
    }
    
    // Action ID
    if (item->getActionId() > 0) {
        writer.writeU8(static_cast<uint8_t>(OtbmAttribute::ActionId));
        writer.writeU16(item->getActionId());
    }
    
    // Unique ID
    if (item->getUniqueId() > 0) {
        writer.writeU8(static_cast<uint8_t>(OtbmAttribute::UniqueId));
        writer.writeU16(item->getUniqueId());
    }
    
    // Text
    std::string text = item->getText();
    if (!text.empty()) {
        writer.writeU8(static_cast<uint8_t>(OtbmAttribute::Text));
        writer.writeString(text);
    }
    
    // Teleport destination
    const Domain::Position* dest = item->getTeleportDestination();
    if (dest) {
        writer.writeU8(static_cast<uint8_t>(OtbmAttribute::TeleportDest));
        writer.writeU16(static_cast<uint16_t>(dest->x));
        writer.writeU16(static_cast<uint16_t>(dest->y));
        writer.writeU8(static_cast<uint8_t>(dest->z));
    }
    
    // Door ID
    uint32_t door_id = item->getDoorId();
    if (door_id > 0) {
        writer.writeU8(static_cast<uint8_t>(OtbmAttribute::HouseDoorId));
        writer.writeU8(static_cast<uint8_t>(door_id));
    }
    
    // Depot ID
    uint32_t depot_id = item->getDepotId();
    if (depot_id > 0) {
        writer.writeU8(static_cast<uint8_t>(OtbmAttribute::DepotId));
        writer.writeU16(static_cast<uint16_t>(depot_id));
    }
    
    // Container items
    const auto& container_items = item->getContainerItems();
    for (const auto& contained : container_items) {
        writeItem(writer, contained.get(), version, ctx);
    }
    
    writer.endNode();
    return true;
}

bool writeTile(
    NodeFileWriteHandle& writer,
    const Domain::Position& pos,
    const Domain::Tile& tile,
    size_t& items_written,
    OtbmVersion version,
    ConversionContext& ctx
) {
    // Calculate local coordinates within 256x256 area
    uint8_t local_x = static_cast<uint8_t>(pos.x & 0xFF);
    uint8_t local_y = static_cast<uint8_t>(pos.y & 0xFF);
    
    bool is_house_tile = tile.getHouseId() > 0;
    
    if (is_house_tile) {
        writer.startNode(static_cast<uint8_t>(OtbmNode::HouseTile));
        writer.writeU8(local_x);
        writer.writeU8(local_y);
        writer.writeU32(tile.getHouseId());
    } else {
        writer.startNode(static_cast<uint8_t>(OtbmNode::Tile));
        writer.writeU8(local_x);
        writer.writeU8(local_y);
    }
    
    // Tile flags - only save map flags, not editor flags (Selected, Modified)
    // Map flags are bits 0-5 (ProtectionZone, NoPvP, NoLogout, PvpZone, Refresh)
    // Editor flags are bits 8+ (Selected=0x100, Modified=0x200)
    constexpr uint32_t MAP_FLAGS_MASK = 0xFF; // Only lower 8 bits are map flags
    uint32_t flags = static_cast<uint32_t>(static_cast<uint16_t>(tile.getFlags())) & MAP_FLAGS_MASK;
    if (flags != 0) {
        writer.writeU8(static_cast<uint8_t>(OtbmAttribute::TileFlags));
        writer.writeU32(flags);
    }
    
    // Ground item - use compact format for simple items, full node for complex
    if (tile.hasGround()) {
        auto* ground = tile.getGround();
        if (ground) {
            uint16_t ground_id = convertItemId(ground->getServerId(), ctx);
            
            // DEBUG: Logging disabled to avoid spamming during save
            // spdlog::info("[DEBUG WRITE] Tile ({},{},{}): Writing ground ID {} -> {}, isComplex={}", 
            //             pos.x, pos.y, pos.z, ground->getServerId(), ground_id, ground->isComplex());
            
            if (ground->isComplex()) {
                // Complex ground item - use full child node
                writeItem(writer, ground, version, ctx);
            } else {
                // Simple ground item - use compact inline format (saves ~2 bytes per item)
                writer.writeU8(static_cast<uint8_t>(OtbmAttribute::Item));
                writer.writeU16(ground_id);
            }
            items_written++;
        }
    }
    
    // Other items - always use full node format (they may have stacking, etc.)
    for (const auto& item : tile.getItems()) {
        if (item) {
            writeItem(writer, item.get(), version, ctx);
            items_written++;
        }
    }
    
    writer.endNode();
    return true;
}

} // anonymous namespace

OtbmWriteResult OtbmWriter::write(
    const std::filesystem::path& path,
    const Domain::ChunkedMap& map,
    OtbmVersion version,
    Services::ClientDataService* client_data,
    OtbmConversionMode conversion_mode,
    OtbmWriteProgressCallback progress
) {
    OtbmWriteResult result;
    
    // Initialize conversion context
    ConversionContext ctx;
    ctx.mode = conversion_mode;
    ctx.client_data = client_data;
    
    // Validate conversion requirements
    if (conversion_mode != OtbmConversionMode::None && !client_data) {
        result.error = "Client data required for ID conversion";
        return result;
    }
    
    if (progress) {
        progress(0, "Opening file...");
    }
    
    NodeFileWriteHandle writer(path, "OTBM");
    if (!writer.isOk()) {
        result.error = "Failed to open file for writing";
        return result;
    }
    
    // Root node
    writer.startNode(0);  // Root node type is 0
    
    // Header
    writer.writeU32(static_cast<uint32_t>(version));
    
    uint16_t map_width = static_cast<uint16_t>(map.getWidth());
    uint16_t map_height = static_cast<uint16_t>(map.getHeight());
    writer.writeU16(map_width);
    writer.writeU16(map_height);
    
    // OTB version info - use values from map (preserved from load)
    const auto& map_version = map.getVersion();
    writer.writeU32(map_version.items_major_version);
    writer.writeU32(map_version.items_minor_version);
    
    // Map data node
    writer.startNode(static_cast<uint8_t>(OtbmNode::MapData));
    
    // Map description
    std::string desc = map.getDescription();
    if (!desc.empty()) {
        writer.writeU8(static_cast<uint8_t>(OtbmAttribute::Description));
        writer.writeString(desc);
    }
    
    // External files
    std::string spawn_file = map.getSpawnFile();
    if (!spawn_file.empty()) {
        writer.writeU8(static_cast<uint8_t>(OtbmAttribute::ExtSpawnFile));
        writer.writeString(spawn_file);
    }
    
    std::string house_file = map.getHouseFile();
    if (!house_file.empty()) {
        writer.writeU8(static_cast<uint8_t>(OtbmAttribute::ExtHouseFile));
        writer.writeString(house_file);
    }
    
    if (progress) {
        progress(10, "Writing tile areas...");
    }
    
    // Group chunks by 256x256 areas (Area = 8x8 chunks)
    // This avoids creating a massive map of all tiles
    std::map<std::tuple<int, int, int>, std::vector<const Domain::Chunk*>> area_chunks;

    map.forEachChunk([&area_chunks](const Domain::Chunk* chunk, int16_t z) {
        if (!chunk || chunk->isEmpty()) return;

        // Use bitwise shift for spatial hashing (floor division by 256)
        // Handles negative coordinates correctly (e.g. -32 >> 8 = -1)
        int area_x = chunk->world_x >> 8;
        int area_y = chunk->world_y >> 8;

        auto key = std::make_tuple(area_x, area_y, static_cast<int>(z));
        area_chunks[key].push_back(chunk);
    });
    
    size_t total_areas = area_chunks.size();
    size_t current_area = 0;
    
    for (const auto& [key, chunks] : area_chunks) {
        auto [area_x, area_y, area_z] = key;
        
        // Tile area node
        writer.startNode(static_cast<uint8_t>(OtbmNode::TileArea));
        writer.writeU16(static_cast<uint16_t>(area_x * 256));
        writer.writeU16(static_cast<uint16_t>(area_y * 256));
        writer.writeU8(static_cast<uint8_t>(area_z));
        
        // Sort chunks to ensure deterministic output (Y then X)
        // Note: chunks is const vector of const Chunk*
        std::vector<const Domain::Chunk*> sorted_chunks = chunks;
        std::sort(sorted_chunks.begin(), sorted_chunks.end(),
            [](const Domain::Chunk* a, const Domain::Chunk* b) {
                return std::tie(a->world_y, a->world_x) < std::tie(b->world_y, b->world_x);
            });

        for (const auto* chunk : sorted_chunks) {
            chunk->forEachTile([&](const Domain::Tile* tile) {
                writeTile(writer, tile->getPosition(), *tile, result.items_written, version, ctx);
                result.tiles_written++;
            });
        }
        
        writer.endNode();  // End tile area
        
        current_area++;
        if (progress && total_areas > 0) {
            int percent = 10 + static_cast<int>(80.0 * current_area / total_areas);
            progress(percent, "Writing tiles...");
        }
    }
    
    // Towns
    const auto& towns = map.getTowns();
    if (!towns.empty()) {
        writer.startNode(static_cast<uint8_t>(OtbmNode::Towns));
        
        for (const auto& town : towns) {
            writer.startNode(static_cast<uint8_t>(OtbmNode::Town));
            writer.writeU32(town.id);
            writer.writeString(town.name);
            writer.writeU16(static_cast<uint16_t>(town.temple_position.x));
            writer.writeU16(static_cast<uint16_t>(town.temple_position.y));
            writer.writeU8(static_cast<uint8_t>(town.temple_position.z));
            writer.endNode();
        }
        
        writer.endNode();  // End towns
    }
    
    // Waypoints
    const auto& waypoints = map.getWaypoints();
    if (!waypoints.empty()) {
        writer.startNode(static_cast<uint8_t>(OtbmNode::Waypoints));
        
        for (const auto& wp : waypoints) {
            writer.startNode(static_cast<uint8_t>(OtbmNode::Waypoint));
            writer.writeString(wp.name);
            writer.writeU16(static_cast<uint16_t>(wp.position.x));
            writer.writeU16(static_cast<uint16_t>(wp.position.y));
            writer.writeU8(static_cast<uint8_t>(wp.position.z));
            writer.endNode();
        }
        
        writer.endNode();  // End waypoints
    }
    
    writer.endNode();  // End map data
    writer.endNode();  // End root
    
    writer.close();
    
    if (progress) {
        progress(100, "Complete");
    }
    
    // Copy conversion statistics
    result.items_converted = ctx.items_converted;
    result.items_skipped = ctx.items_skipped;
    
    result.success = writer.isOk();
    if (!result.success) {
        result.error = "Write error occurred";
    }
    
    return result;
}

bool OtbmWriter::writeHouses(
    const std::filesystem::path& path,
    const Domain::ChunkedMap& map
) {
    return HouseXmlWriter::write(path, map);
}

bool OtbmWriter::writeSpawns(
    const std::filesystem::path& path,
    const Domain::ChunkedMap& map
) {
    return SpawnXmlWriter::write(path, map);
}

bool OtbmWriter::writeWaypoints(
    const std::filesystem::path& path,
    const Domain::ChunkedMap& map
) {
    // Waypoints are stored in OTBM, not separate file
    return true;
}

} // namespace MapEditor::IO
