#pragma once

#include <vector>
#include <unordered_map>
#include <array>
#include <cstdint>
#include <mutex>

namespace MapEditor {
namespace Rendering {

struct CachedLightGrid {
    static constexpr int SIZE = 32;
    std::array<uint32_t, SIZE * SIZE> pixels; // RGBA packed
    bool is_valid = false;
};

class LightCache {
public:
    LightCache();
    ~LightCache();

    // Non-copyable
    LightCache(const LightCache&) = delete;
    LightCache& operator=(const LightCache&) = delete;

    /**
     * Get the cached grid for a specific chunk.
     * Returns nullptr if not currently cached (caller should compute and set).
     */
    CachedLightGrid* getGrid(int32_t chunk_x, int32_t chunk_y, int16_t chunk_z);

    /**
     * Get or create a grid slot. If it was invalid/missing, is_valid will be false.
     * Caller is responsible for filling it and setting is_valid = true.
     */
    CachedLightGrid& getOrCreateGrid(int32_t chunk_x, int32_t chunk_y, int16_t chunk_z);

    /**
     * Invalidate a specific chunk cache.
     */
    void invalidate(int32_t chunk_x, int32_t chunk_y, int16_t chunk_z);

    /**
     * Invalidate a region of chunks (e.g. 3x3 around a change).
     */
    void invalidateRegion(int32_t min_chunk_x, int32_t min_chunk_y,
                          int32_t max_chunk_x, int32_t max_chunk_y, int16_t chunk_z);

    /**
     * Clear the entire cache.
     */
    void clear();

private:
    static uint64_t chunkKey(int32_t x, int32_t y, int16_t z);

    std::unordered_map<uint64_t, CachedLightGrid> cache_;
    mutable std::mutex cache_mutex_;
};

} // namespace Rendering
} // namespace MapEditor
