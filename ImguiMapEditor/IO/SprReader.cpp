#include "SprReader.h"
#include <fstream>
#include <cstring>
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace IO {

bool SpriteData::decode(bool use_transparency) {
    // [UB FIX] Double-Checked Locking Pattern (Thread-Safe)
    // First check with Acquire semantics to ensure we see writes to rgba_data
    if (is_decoded.load(std::memory_order_acquire)) {
        return true;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    // Second check under lock (Relaxed is fine as we hold the lock)
    if (is_decoded.load(std::memory_order_relaxed)) {
        return true;
    }
    
    if (is_empty || compressed_pixels.empty()) {
        // Create transparent sprite
        rgba_data.resize(RGBA_SIZE, 0);
        is_decoded.store(true, std::memory_order_release);
        return true;
    }
    
    // Initialize with transparent pixels
    rgba_data.resize(RGBA_SIZE, 0);
    
    // RLE decode
    size_t read_pos = 0;
    size_t pixel_index = 0;
    
    while (read_pos < compressed_pixels.size() && pixel_index < SPRITE_PIXELS) {
        // Read transparent pixel count (little-endian 16-bit)
        if (read_pos + 2 > compressed_pixels.size()) break;
        
        uint16_t transparent_count = compressed_pixels[read_pos] | 
                                     (compressed_pixels[read_pos + 1] << 8);
        read_pos += 2;
        
        pixel_index += transparent_count;
        
        if (pixel_index >= SPRITE_PIXELS) break;
        
        // Read colored pixel count
        if (read_pos + 2 > compressed_pixels.size()) break;
        
        uint16_t colored_count = compressed_pixels[read_pos] | 
                                 (compressed_pixels[read_pos + 1] << 8);
        read_pos += 2;
        
        // Read colored pixels (RGB)
        for (uint16_t i = 0; i < colored_count && pixel_index < SPRITE_PIXELS; ++i) {
            if (read_pos + 3 > compressed_pixels.size()) break;
            
            uint8_t r = compressed_pixels[read_pos++];
            uint8_t g = compressed_pixels[read_pos++];
            uint8_t b = compressed_pixels[read_pos++];
            
            size_t rgba_index = pixel_index * 4;
            rgba_data[rgba_index + 0] = r;
            rgba_data[rgba_index + 1] = g;
            rgba_data[rgba_index + 2] = b;
            rgba_data[rgba_index + 3] = 255;  // Opaque
            
            pixel_index++;
        }
    }
    
    // Publish results with Release semantics so other threads see rgba_data writes
    is_decoded.store(true, std::memory_order_release);
    return true;
}

SprResult SprReader::open(const std::filesystem::path& path,
                           uint32_t expected_signature,
                           bool extended) {
    // [UB FIX] Critical race condition fix:
    // Multiple threads (worker pool) read from file_ via loadSprite().
    // If open() is called (main thread), it closes/reopens file_ without lock.
    // This causes crashes/UB in workers. We MUST lock here.
    std::lock_guard lock(mutex_);

    SprResult result;
    extended_ = extended;
    cache_.clear();
    offsets_.clear();
    
    if (file_.is_open()) {
        file_.close();
    }
    
    file_.open(path, std::ios::binary);
    if (!file_) {
        result.error = "Failed to open file: " + path.string();
        return result;
    }
    
    // Read signature
    uint32_t signature = 0;
    file_.read(reinterpret_cast<char*>(&signature), 4);
    if (!file_) {
        result.error = "Failed to read signature";
        return result;
    }
    
    result.signature = signature;
    signature_ = signature;
    
    // Validate signature if provided
    if (expected_signature != 0 && signature != expected_signature) {
        result.error = "Signature mismatch. Expected: " + 
                       std::to_string(expected_signature) + 
                       ", Got: " + std::to_string(signature);
        return result;
    }
    
    // Read sprite count
    if (extended_) {
        file_.read(reinterpret_cast<char*>(&sprite_count_), 4);
    } else {
        uint16_t count16 = 0;
        file_.read(reinterpret_cast<char*>(&count16), 2);
        sprite_count_ = count16;
    }
    
    if (!file_) {
        result.error = "Failed to read sprite count";
        return result;
    }
    
    result.sprite_count = sprite_count_;
    
    // Read all offsets
    offsets_.resize(sprite_count_);
    for (uint32_t i = 0; i < sprite_count_; ++i) {
        file_.read(reinterpret_cast<char*>(&offsets_[i]), 4);
    }
    
    if (!file_) {
        result.error = "Failed to read sprite offsets";
        return result;
    }
    
    spdlog::info("Opened SPR with {} sprites", sprite_count_);
    result.success = true;
    return result;
}

std::shared_ptr<SpriteData> SprReader::loadSprite(uint32_t sprite_id) {
    // ID 0 is always empty/blank
    if (sprite_id == 0) {
        auto sprite = std::make_shared<SpriteData>();
        sprite->id = 0;
        sprite->is_empty = true;
        return sprite;
    }
    
    // Lock for thread-safety (file access and cache)
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check cache
    auto it = cache_.find(sprite_id);
    if (it != cache_.end()) {
        return it->second;
    }
    
    // Validate ID (1-based in file)
    if (sprite_id > sprite_count_) {
        return nullptr;
    }
    
    // Get offset (0-indexed in our array)
    uint32_t offset = offsets_[sprite_id - 1];
    
    if (offset == 0) {
        // Empty sprite
        auto sprite = std::make_shared<SpriteData>();
        sprite->id = sprite_id;
        sprite->is_empty = true;
        cache_[sprite_id] = sprite;
        return sprite;
    }
    
    // Seek to sprite data
    file_.seekg(offset, std::ios::beg);
    if (!file_) {
        return nullptr;
    }
    
    // Skip RGB transparent color (3 bytes)
    file_.seekg(3, std::ios::cur);
    
    // Read compressed size
    uint16_t size = 0;
    file_.read(reinterpret_cast<char*>(&size), 2);
    if (!file_) {
        return nullptr;
    }
    
    auto sprite = std::make_shared<SpriteData>();
    sprite->id = sprite_id;
    sprite->compressed_size = size;
    
    if (size > 0) {
        sprite->compressed_pixels.resize(size);
        file_.read(reinterpret_cast<char*>(sprite->compressed_pixels.data()), size);
        if (!file_) {
            return nullptr;
        }
        sprite->is_empty = false;
    } else {
        sprite->is_empty = true;
    }
    
    cache_[sprite_id] = sprite;
    return sprite;
}

} // namespace IO
} // namespace MapEditor
