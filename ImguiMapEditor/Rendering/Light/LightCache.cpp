#include "LightCache.h"
namespace MapEditor {
namespace Rendering {

LightCache::LightCache() = default;
LightCache::~LightCache() = default;

uint64_t LightCache::chunkKey(int32_t x, int32_t y, int16_t z) {
    // Pack x (20), y (20), z (8) into 64-bit key
    // Supports 1,048,576 chunks (33 million tiles) per axis
    // X at 0-19, Y at 20-39, Z at 40-47
    return (static_cast<uint64_t>(x & 0xFFFFF)) | 
           (static_cast<uint64_t>(y & 0xFFFFF) << 20) | 
           (static_cast<uint64_t>(z & 0xFF) << 40);
}

CachedLightGrid* LightCache::getGrid(int32_t chunk_x, int32_t chunk_y, int16_t chunk_z) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = cache_.find(chunkKey(chunk_x, chunk_y, chunk_z));
    if (it != cache_.end() && it->second.is_valid) {
        return &it->second;
    }
    return nullptr;
}

CachedLightGrid& LightCache::getOrCreateGrid(int32_t chunk_x, int32_t chunk_y, int16_t chunk_z) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_[chunkKey(chunk_x, chunk_y, chunk_z)];
}

void LightCache::invalidate(int32_t chunk_x, int32_t chunk_y, int16_t chunk_z) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = cache_.find(chunkKey(chunk_x, chunk_y, chunk_z));
    if (it != cache_.end()) {
        it->second.is_valid = false;
    }
}

void LightCache::invalidateRegion(int32_t min_x, int32_t min_y,
                                  int32_t max_x, int32_t max_y, int16_t chunk_z)
{
    std::lock_guard<std::mutex> lock(cache_mutex_);
    for (int32_t y = min_y; y <= max_y; ++y) {
        for (int32_t x = min_x; x <= max_x; ++x) {
            auto it = cache_.find(chunkKey(x, y, chunk_z));
            if (it != cache_.end()) {
                it->second.is_valid = false;
            }
        }
    }
}

void LightCache::clear() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.clear();
}

} // namespace Rendering
} // namespace MapEditor
