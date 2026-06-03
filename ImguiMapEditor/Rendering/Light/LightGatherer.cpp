#include "LightGatherer.h"

#include <algorithm>

#include "Core/Config.h"
#include "Rendering/Light/LightProjection.h"
#include "Services/ClientDataService.h"
#include "Domain/Tile.h"
#include "Domain/Item.h"
#include "Domain/ItemType.h"
#include "Domain/Creature.h"
#include "Domain/CreatureType.h"

namespace MapEditor {
namespace Rendering {

namespace {

constexpr int LIGHT_COLLECTION_MARGIN_TILES = 16;

bool blocksLightFromBelow(const Domain::ItemType* item_type) {
    if (!item_type) return false;
    const bool is_ground_tile =
        item_type->is_ground || item_type->group == Domain::ItemGroup::Ground;
    return is_ground_tile && !item_type->is_translucent;
}

bool carriesTranslucentLight(const Domain::Tile* tile,
                             Services::ClientDataService* client_data) {
    if (!tile || !client_data) return false;

    const auto check_item = [&](const Domain::Item* item) {
        if (!item) return false;
        const Domain::ItemType* item_type = item->getType();
        if (!item_type) {
            item_type = client_data->getItemTypeByServerId(item->getServerId());
        }
        return item_type &&
               (item_type->is_translucent || item_type->lens_help > 0);
    };

    if (check_item(tile->getGround())) return true;

    for (const auto& item : tile->getItems()) {
        if (check_item(item.get())) return true;
    }

    return false;
}

} // namespace

void LightGatherer::addLight(ViewportLightBuffer& light_buffer,
                             int32_t x, int32_t y,
                             uint8_t color, uint8_t intensity,
                             int16_t floor) {
    if (intensity == 0) return;

    if (!light_buffer.lights.empty()) {
        auto& prev = light_buffer.lights.back();
        if (prev.x == x && prev.y == y && prev.color == color && prev.source_floor == floor) {
            prev.intensity = std::max(prev.intensity, intensity);
            return;
        }
    }

    light_buffer.lights.emplace_back(Domain::LightSource{
        .x = x,
        .y = y,
        .color = color,
        .intensity = intensity,
        .source_floor = floor
    });
}

void LightGatherer::gatherViewportLightBuffer(
    const Domain::ChunkedMap& map,
    Services::ClientDataService* client_data,
    int16_t current_floor,
    int16_t start_floor,
    int16_t end_floor,
    ViewportLightBuffer& light_buffer)
{
    if (!client_data) return;

    // RME processes start_z down to superend_z. The floor_light_start snapshot
    // must be taken before collecting each floor's lights, in that same order.
    for (int16_t floor = start_floor; floor >= end_floor; --floor) {
        const uint32_t floor_light_start =
            static_cast<uint32_t>(light_buffer.lights.size());

        if (floor >= current_floor) {
            registerGroundOcclusionForViewport(
                map, client_data, current_floor, floor, floor_light_start, light_buffer);
        }

        gatherLightsForViewportFloor(
            map, client_data, current_floor, floor, light_buffer);
    }
}

void LightGatherer::registerGroundOcclusionForViewport(
    const Domain::ChunkedMap& map,
    Services::ClientDataService* client_data,
    int16_t current_floor,
    int16_t floor,
    uint32_t floor_light_start,
    ViewportLightBuffer& light_buffer)
{
    const int32_t floor_offset = projectedFloorOffsetTiles(current_floor, floor);
    const int32_t min_x = light_buffer.origin_x + floor_offset;
    const int32_t min_y = light_buffer.origin_y + floor_offset;
    const int32_t max_x = light_buffer.origin_x + light_buffer.width - 1 + floor_offset;
    const int32_t max_y = light_buffer.origin_y + light_buffer.height - 1 + floor_offset;

    std::vector<Domain::Chunk*> chunks;
    map.getVisibleChunks(min_x, min_y, max_x, max_y, floor, chunks);

    for (Domain::Chunk* chunk : chunks) {
        if (!chunk) continue;

        chunk->forEachTile([&](const Domain::Tile* tile) {
            if (!tile || tile->getZ() != floor) return;

            const Domain::Item* ground = tile->getGround();
            if (!ground) return;

            const Domain::ItemType* ground_type = ground->getType();
            if (!ground_type) {
                ground_type = client_data->getItemTypeByServerId(ground->getServerId());
            }
            if (!blocksLightFromBelow(ground_type)) return;

            const int32_t projected_x = tile->getX() - floor_offset;
            const int32_t projected_y = tile->getY() - floor_offset;
            light_buffer.setTileStart(projected_x, projected_y, floor_light_start);
        });
    }
}

void LightGatherer::gatherLightsForViewportFloor(
    const Domain::ChunkedMap& map,
    Services::ClientDataService* client_data,
    int16_t current_floor,
    int16_t floor,
    ViewportLightBuffer& light_buffer)
{
    const int32_t floor_offset = projectedFloorOffsetTiles(current_floor, floor);
    const int32_t min_x =
        light_buffer.origin_x + floor_offset - LIGHT_COLLECTION_MARGIN_TILES;
    const int32_t min_y =
        light_buffer.origin_y + floor_offset - LIGHT_COLLECTION_MARGIN_TILES;
    const int32_t max_x =
        light_buffer.origin_x + light_buffer.width - 1 + floor_offset +
        LIGHT_COLLECTION_MARGIN_TILES;
    const int32_t max_y =
        light_buffer.origin_y + light_buffer.height - 1 + floor_offset +
        LIGHT_COLLECTION_MARGIN_TILES;

    std::vector<Domain::Chunk*> chunks;
    map.getVisibleChunks(min_x, min_y, max_x, max_y, floor, chunks);

    const auto add_item_light = [&](const Domain::Item* item,
                                    int32_t projected_x,
                                    int32_t projected_y) {
        if (!item) return;
        const Domain::ItemType* item_type = item->getType();
        if (!item_type) {
            item_type = client_data->getItemTypeByServerId(item->getServerId());
        }
        if (item_type && item_type->light_level > 0) {
            addLight(light_buffer, projected_x, projected_y,
                     item_type->light_color, item_type->light_level, floor);
        }
    };

    for (Domain::Chunk* chunk : chunks) {
        if (!chunk) continue;

        chunk->forEachTile([&](const Domain::Tile* tile) {
            if (!tile || tile->getZ() != floor) return;

            const int32_t projected_x = tile->getX() - floor_offset;
            const int32_t projected_y = tile->getY() - floor_offset;

            add_item_light(tile->getGround(), projected_x, projected_y);

            for (const auto& item : tile->getItems()) {
                add_item_light(item.get(), projected_x, projected_y);
            }

            const Domain::Creature* creature = tile->getCreature();
            if (creature) {
                const Domain::CreatureType* creature_type =
                    client_data->getCreatureType(creature->name);
                if (creature_type && creature_type->light_level > 0) {
                    addLight(light_buffer, projected_x, projected_y,
                             creature_type->light_color,
                             creature_type->light_level, floor);
                }
            }
        });
    }

    if (floor == Config::Map::GROUND_LAYER + 1) {
        const int16_t above_floor = Config::Map::GROUND_LAYER;
        const int32_t above_offset = projectedFloorOffsetTiles(current_floor, above_floor);
        const int32_t above_min_x =
            light_buffer.origin_x + above_offset - LIGHT_COLLECTION_MARGIN_TILES;
        const int32_t above_min_y =
            light_buffer.origin_y + above_offset - LIGHT_COLLECTION_MARGIN_TILES;
        const int32_t above_max_x =
            light_buffer.origin_x + light_buffer.width - 1 + above_offset +
            LIGHT_COLLECTION_MARGIN_TILES;
        const int32_t above_max_y =
            light_buffer.origin_y + light_buffer.height - 1 + above_offset +
            LIGHT_COLLECTION_MARGIN_TILES;

        std::vector<Domain::Chunk*> above_chunks;
        map.getVisibleChunks(above_min_x, above_min_y, above_max_x, above_max_y,
                             above_floor, above_chunks);

        for (Domain::Chunk* chunk : above_chunks) {
            if (!chunk) continue;

            chunk->forEachTile([&](const Domain::Tile* tile) {
                if (!tile || tile->getZ() != above_floor) return;
                if (!carriesTranslucentLight(tile, client_data)) return;

                const int32_t projected_x = tile->getX() - floor_offset;
                const int32_t projected_y = tile->getY() - floor_offset;
                addLight(light_buffer, projected_x, projected_y,
                         Config::Lighting::DEFAULT_SERVER_LIGHT_COLOR,
                         Config::Lighting::TRANSLUCENT_PROPAGATION_INTENSITY,
                         floor);
            });
        }
    }
}

} // namespace Rendering
} // namespace MapEditor
