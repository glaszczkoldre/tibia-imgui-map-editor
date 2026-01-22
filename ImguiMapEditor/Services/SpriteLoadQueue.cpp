#include "SpriteLoadQueue.h"
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace Services {

SpriteLoadQueue::SpriteLoadQueue(size_t thread_count) {
    // Start worker threads
    workers_.reserve(thread_count);
    for (size_t i = 0; i < thread_count; ++i) {
        workers_.emplace_back(&SpriteLoadQueue::workerLoop, this);
    }
    spdlog::info("SpriteLoadQueue: Started {} worker threads", thread_count);
}

SpriteLoadQueue::~SpriteLoadQueue() {
    shutdown();
}

void SpriteLoadQueue::setLoader(SpriteLoader loader) {
    loader_ = std::move(loader);
}

void SpriteLoadQueue::requestSprite(uint32_t sprite_id) {
    if (sprite_id == 0) return;

    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        request_queue_.push(sprite_id);
    }
    request_cv_.notify_one();
}

void SpriteLoadQueue::requestSprites(const std::vector<uint32_t>& sprite_ids) {
    if (sprite_ids.empty()) return;

    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        for (uint32_t id : sprite_ids) {
            if (id != 0) {
                request_queue_.push(id);
            }
        }
    }
    request_cv_.notify_all();
}

std::vector<SpriteLoadQueue::LoadResult> SpriteLoadQueue::getCompletedSprites() {
    std::vector<LoadResult> result;
    {
        std::lock_guard<std::mutex> lock(completed_mutex_);
        result = std::move(completed_);
        completed_.clear();
    }
    return result;
}

void SpriteLoadQueue::clearPending() {
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        std::queue<uint32_t> empty;
        std::swap(request_queue_, empty);
    }
}

void SpriteLoadQueue::shutdown() {
    if (shutdown_.exchange(true)) {
        return;  // Already shut down
    }

    // Wake up all workers
    request_cv_.notify_all();

    // Wait for all workers to finish
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();

    spdlog::debug("SpriteLoadQueue: Shutdown complete");
}

void SpriteLoadQueue::workerLoop() {
    while (!shutdown_) {
        uint32_t sprite_id = 0;

        // Wait for work
        {
            std::unique_lock<std::mutex> lock(request_mutex_);
            request_cv_.wait(lock, [this] {
                return shutdown_ || !request_queue_.empty();
            });

            if (shutdown_ && request_queue_.empty()) {
                return;
            }

            if (!request_queue_.empty()) {
                sprite_id = request_queue_.front();
                request_queue_.pop();
            }
        }

        if (sprite_id == 0) continue;

        // Load sprite (outside of lock!)
        LoadResult result;
        result.sprite_id = sprite_id;

        if (loader_) {
            try {
                result.rgba_data = loader_(sprite_id);
                result.success = !result.rgba_data.empty();
            } catch (const std::exception& e) {
                spdlog::error("SpriteLoadQueue: Exception loading sprite {}: {}", 
                             sprite_id, e.what());
                result.success = false;
            }
        }

        // Store result
        {
            std::lock_guard<std::mutex> lock(completed_mutex_);
            completed_.push_back(std::move(result));
        }
    }
}

} // namespace Services
} // namespace MapEditor
