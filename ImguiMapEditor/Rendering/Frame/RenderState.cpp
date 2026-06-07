#include "Rendering/Frame/RenderState.h"
#include "Domain/ChunkedMap.h"
#include <spdlog/spdlog.h>
#include <unordered_set>

namespace MapEditor::Rendering {

namespace {

int32_t floorDiv(int32_t value, int32_t divisor) {
    if (value >= 0) {
        return value / divisor;
    }
    return -static_cast<int32_t>((-static_cast<int64_t>(value) + divisor - 1) /
                                 divisor);
}

struct ChunkKey {
    int32_t x = 0;
    int32_t y = 0;
    int8_t z = 0;

    bool operator==(const ChunkKey& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct ChunkKeyHash {
    size_t operator()(const ChunkKey& key) const {
        size_t seed = 0;
        auto mix = [&seed](auto value) {
            seed ^= std::hash<decltype(value)>{}(value) + 0x9e3779b9 +
                    (seed << 6) + (seed >> 2);
        };
        mix(key.x);
        mix(key.y);
        mix(key.z);
        return seed;
    }
};

} // namespace

RenderState::RenderState(Services::ClientDataService* client_data) {
    light_manager = std::make_unique<LightManager>(client_data);
    if (!light_manager->initialize()) {
        spdlog::warn("Failed to initialize LightManager for RenderState");
    }
}

RenderState::~RenderState() = default;

void RenderState::invalidateAll() {
    chunk_cache.invalidateAll();
    if (light_manager) {
        light_manager->invalidateAll();
    }
    overlay_collector.clear();
}

void RenderState::invalidateChunk(int32_t chunk_x, int32_t chunk_y, int8_t floor) {
    chunk_cache.invalidate(chunk_x, chunk_y, floor);
}

void RenderState::invalidateLight(int32_t x, int32_t y) {
    if (light_manager) {
        light_manager->invalidateTile(x, y);
    }
}

void RenderState::invalidateTiles(std::span<const Domain::Position> positions) {
    std::unordered_set<ChunkKey, ChunkKeyHash> chunks;
    chunks.reserve(positions.size());

    for (const auto& position : positions) {
        chunks.insert({.x = floorDiv(position.x, Domain::Chunk::SIZE),
                       .y = floorDiv(position.y, Domain::Chunk::SIZE),
                       .z = static_cast<int8_t>(position.z)});
        invalidateLight(position.x, position.y);
    }

    for (const auto& chunk : chunks) {
        invalidateChunk(chunk.x, chunk.y, chunk.z);
    }
    overlay_collector.clear();
}

} // namespace MapEditor::Rendering
