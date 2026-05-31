#include "LightGatherer.h"
#include "Services/ClientDataService.h"
#include "Domain/Tile.h"
#include "Domain/Item.h"
#include "Domain/ItemType.h"
#include "Domain/Creature.h"
#include "Domain/CreatureType.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace MapEditor {
namespace Rendering {

void LightGatherer::clear() {
    lights_.clear();
    ground_brightness_ = GroundBrightness{};
}

void LightGatherer::addLight(int32_t x, int32_t y, uint8_t color, uint8_t intensity, int16_t floor) {
    // Dedup: merge consecutive lights with same pos, color, and floor
    if (!lights_.empty()) {
        auto& prev = lights_.back();
        if (prev.x == x && prev.y == y && prev.color == color && prev.source_floor == floor) {
            prev.intensity = std::max(prev.intensity, intensity);
            return;
        }
    }
    lights_.emplace_back(Domain::LightSource{
        .x = x,
        .y = y,
        .color = color,
        .intensity = intensity,
        .source_floor = floor
    });
}

void LightGatherer::gatherForChunk(
    const Domain::ChunkedMap& map,
    int32_t chunk_x, int32_t chunk_y,
    Services::ClientDataService* client_data,
    int16_t floor)
{
    if (!client_data) return;
    
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int32_t target_cx = chunk_x + dx;
            int32_t target_cy = chunk_y + dy;
            
            int32_t tile_start_x = target_cx * Domain::Chunk::SIZE;
            int32_t tile_start_y = target_cy * Domain::Chunk::SIZE;
            int32_t tile_end_x = tile_start_x + Domain::Chunk::SIZE;
            int32_t tile_end_y = tile_start_y + Domain::Chunk::SIZE;
            
            std::vector<Domain::Chunk*> chunks;
            map.getVisibleChunks(tile_start_x, tile_start_y, tile_end_x - 1, tile_end_y - 1, floor, chunks);
            
            for (Domain::Chunk* chunk : chunks) {
                if (!chunk) continue;
                
                chunk->forEachTile([&](const Domain::Tile* tile) {
                    if (!tile || tile->getZ() != floor) return;
                    
                    int32_t tile_x = tile->getX();
                    int32_t tile_y = tile->getY();
                    
                    // Check ground item for light
                    const Domain::Item* ground = tile->getGround();
                    if (ground) {
                        const Domain::ItemType* item_type = client_data->getItemTypeByServerId(ground->getServerId());
                        if (item_type && item_type->light_level > 0) {
                            addLight(tile_x, tile_y, item_type->light_color, item_type->light_level, floor);
                        }
                    }
                    
                    // Check all items on the tile
                    for (const auto& item_ptr : tile->getItems()) {
                        if (!item_ptr) continue;
                        const Domain::ItemType* item_type = client_data->getItemTypeByServerId(item_ptr->getServerId());
                        if (item_type && item_type->light_level > 0) {
                            addLight(tile_x, tile_y, item_type->light_color, item_type->light_level, floor);
                        }
                    }
                    
                    // Check creature for light
                    const Domain::Creature* creature = tile->getCreature();
                    if (creature) {
                        const Domain::CreatureType* creature_type = client_data->getCreatureType(creature->name);
                        if (creature_type && creature_type->light_level > 0) {
                            addLight(tile_x, tile_y, creature_type->light_color, creature_type->light_level, floor);
                        }
                    }
                });
            }
        }
    }
}

void LightGatherer::gatherForChunkMultiFloor(
    const Domain::ChunkedMap& map,
    int32_t chunk_x, int32_t chunk_y,
    Services::ClientDataService* client_data,
    int16_t start_floor,
    int16_t end_floor)
{
    if (!client_data) return;
    
    constexpr int GROUND_LAYER = 7;
    
    // Reset ground brightness for the target chunk
    ground_brightness_ = GroundBrightness{};
    
    for (int16_t floor = start_floor; floor >= end_floor; --floor) {
        int32_t floor_offset = 0;
        if (floor <= GROUND_LAYER) {
            floor_offset = GROUND_LAYER - floor;
        }
        
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                bool is_target = (dx == 0 && dy == 0);
                gatherLightsFromNeighborChunk(
                    map, chunk_x + dx, chunk_y + dy, 
                    client_data, floor, floor_offset, is_target);
            }
        }
    }
}

void LightGatherer::gatherLightsFromNeighborChunk(
    const Domain::ChunkedMap& map,
    int32_t target_cx, int32_t target_cy,
    Services::ClientDataService* client_data,
    int16_t floor, int32_t floor_offset,
    bool is_target_chunk)
{
    int32_t tile_start_x = target_cx * Domain::Chunk::SIZE;
    int32_t tile_start_y = target_cy * Domain::Chunk::SIZE;
    int32_t tile_end_x = tile_start_x + Domain::Chunk::SIZE;
    int32_t tile_end_y = tile_start_y + Domain::Chunk::SIZE;
    
    int32_t target_chunk_start_x = target_cx * Domain::Chunk::SIZE;
    int32_t target_chunk_start_y = target_cy * Domain::Chunk::SIZE;
    
    constexpr int GROUND_LAYER = 7;
    
    std::vector<Domain::Chunk*> chunks;
    map.getVisibleChunks(tile_start_x, tile_start_y, tile_end_x - 1, tile_end_y - 1, floor, chunks);
    
    for (Domain::Chunk* chunk : chunks) {
        if (!chunk) continue;
        
        chunk->forEachTile([&](const Domain::Tile* tile) {
            if (!tile || tile->getZ() != floor) return;
            
            int32_t tile_x = tile->getX();
            int32_t tile_y = tile->getY();
            
            int32_t adjusted_x = tile_x - floor_offset;
            int32_t adjusted_y = tile_y - floor_offset;
            
            // Track ground solidity for blocking (only for the target chunk)
            if (is_target_chunk) {
                const Domain::Item* ground = tile->getGround();
                if (ground) {
                    const Domain::ItemType* ground_type = client_data->getItemTypeByServerId(ground->getServerId());
                    if (ground_type && ground_type->is_ground && !ground_type->is_translucent) {
                        int32_t local_x = tile_x - target_chunk_start_x;
                        int32_t local_y = tile_y - target_chunk_start_y;
                        if (local_x >= 0 && local_x < 32 && local_y >= 0 && local_y < 32) {
                            int idx = local_y * 32 + local_x;
                            if (ground_brightness_.blocking_floor[idx] < 0 || floor < ground_brightness_.blocking_floor[idx]) {
                                ground_brightness_.blocking_floor[idx] = floor;
                            }
                        }
                    }
                }
            }
            
            // Add light sources from items
            auto addLightFromItem = [&](const Domain::Item* item) {
                if (!item) return;
                const Domain::ItemType* item_type = client_data->getItemTypeByServerId(item->getServerId());
                if (item_type && item_type->light_level > 0) {
                    addLight(adjusted_x, adjusted_y, item_type->light_color, item_type->light_level, floor);
                }
            };
            
            addLightFromItem(tile->getGround());
            
            for (const auto& item_ptr : tile->getItems()) {
                addLightFromItem(item_ptr.get());
            }
            
            // Check creature for light (Phase 4)
            const Domain::Creature* creature = tile->getCreature();
            if (creature) {
                const Domain::CreatureType* creature_type = client_data->getCreatureType(creature->name);
                if (creature_type && creature_type->light_level > 0) {
                    addLight(adjusted_x, adjusted_y, creature_type->light_color, creature_type->light_level, floor);
                }
            }
            
            // Translucent ground propagation (Phase 5)
            if (floor == GROUND_LAYER) {
                bool has_translucent = false;
                
                const Domain::Item* ground_check = tile->getGround();
                if (ground_check) {
                    const Domain::ItemType* ground_type = client_data->getItemTypeByServerId(ground_check->getServerId());
                    if (ground_type && (ground_type->is_translucent || ground_type->lens_help > 0)) {
                        has_translucent = true;
                    }
                }
                
                if (!has_translucent) {
                    for (const auto& item_ptr : tile->getItems()) {
                        const Domain::ItemType* item_type = client_data->getItemTypeByServerId(item_ptr->getServerId());
                        if (item_type && (item_type->is_translucent || item_type->lens_help > 0)) {
                            has_translucent = true;
                            break;
                        }
                    }
                }
                
                if (has_translucent) {
                    // Emit dim white light at z=8 (no isometric offset for underground)
                    addLight(tile_x, tile_y, 215, 1, static_cast<int16_t>(GROUND_LAYER + 1));
                }
            }
        });
    }
}

} // namespace Rendering
} // namespace MapEditor
