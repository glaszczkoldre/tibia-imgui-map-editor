#pragma once

#include <cstdint>
#include <cstddef>

namespace MapEditor::Domain::History {

/**
 * Configuration for the history/undo-redo system.
 */
struct HistoryConfig {
    // Maximum number of history entries
    size_t max_entries = 500;
    
    // Maximum memory usage in bytes (256 MB default)
    size_t max_memory_bytes = 256 * 1024 * 1024;
    
    // Enable LZ4 compression for tile snapshots
    bool enable_compression = true;
    
    // Minimum size to compress (skip compression for tiny data)
    size_t min_compress_size = 64;
};

} // namespace MapEditor::Domain::History
