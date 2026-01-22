#pragma once

#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>
#include <cstdint>

namespace MapEditor {
namespace Services {

/**
 * Thread-safe queue for async sprite loading.
 * 
 * ARCHITECTURE:
 * - Main thread calls requestSprite() to queue loads
 * - Worker threads read sprite data from disk and decode
 * - Main thread polls getCompletedSprites() each frame
 * - Completed sprites are uploaded to GPU via PBO (in SpriteManager)
 * 
 * This eliminates disk I/O and decode from the render thread.
 *
 * Simplified to remove internal pending tracking - caller is responsible
 * for filtering duplicate requests.
 */
class SpriteLoadQueue {
public:
    /**
     * Result of a sprite load operation.
     */
    struct LoadResult {
        uint32_t sprite_id = 0;
        std::vector<uint8_t> rgba_data;  // 32x32x4 = 4096 bytes
        bool success = false;
    };

    /**
     * Function signature for the sprite loader callback.
     * Takes sprite_id, returns rgba_data or empty vector on failure.
     */
    using SpriteLoader = std::function<std::vector<uint8_t>(uint32_t)>;

    /**
     * Create load queue with specified thread count.
     * @param thread_count Number of worker threads (default: 4)
     */
    explicit SpriteLoadQueue(size_t thread_count = 4);
    ~SpriteLoadQueue();

    // Non-copyable
    SpriteLoadQueue(const SpriteLoadQueue&) = delete;
    SpriteLoadQueue& operator=(const SpriteLoadQueue&) = delete;

    /**
     * Set the sprite loader callback.
     * Must be called before requestSprite().
     */
    void setLoader(SpriteLoader loader);

    /**
     * Request a sprite to be loaded asynchronously.
     * Caller must ensure no duplicates are requested if strict uniqueness is required,
     * though duplicate processing is harmless but wasteful.
     * @param sprite_id The sprite ID to load
     */
    void requestSprite(uint32_t sprite_id);

    /**
     * Request multiple sprites to be loaded.
     * @param sprite_ids Vector of sprite IDs
     */
    void requestSprites(const std::vector<uint32_t>& sprite_ids);

    /**
     * Get all sprites that have completed loading since last call.
     * Non-blocking - returns immediately with whatever is ready.
     * @return Vector of completed load results
     */
    std::vector<LoadResult> getCompletedSprites();

    /**
     * Clear all pending requests (does not affect in-flight loads).
     */
    void clearPending();

    /**
     * Shutdown the queue and wait for workers to finish.
     */
    void shutdown();

private:
    void workerLoop();

    SpriteLoader loader_;

    // Request queue (main thread -> workers)
    std::queue<uint32_t> request_queue_;
    mutable std::mutex request_mutex_;
    std::condition_variable request_cv_;

    // Completed results (workers -> main thread)
    std::vector<LoadResult> completed_;
    std::mutex completed_mutex_;

    // Worker threads
    std::vector<std::thread> workers_;
    std::atomic<bool> shutdown_{false};
};

} // namespace Services
} // namespace MapEditor
