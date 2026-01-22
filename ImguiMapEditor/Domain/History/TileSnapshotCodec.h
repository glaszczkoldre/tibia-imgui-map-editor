#pragma once

#include <vector>
#include <cstdint>

namespace MapEditor::Domain::History {

/**
 * LZ4 compression/decompression for tile snapshot data.
 * Provides fast compression for undo/redo history.
 */
class TileSnapshotCodec {
public:
    /**
     * Compress data using LZ4.
     * @param data Uncompressed data
     * @return Compressed data
     */
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& data);
    
    /**
     * Decompress LZ4 data.
     * @param compressed Compressed data
     * @param original_size Expected size after decompression
     * @return Decompressed data
     */
    static std::vector<uint8_t> decompress(
        const std::vector<uint8_t>& compressed, 
        size_t original_size
    );
    
    /**
     * Check if compression is available.
     */
    static bool isAvailable();
};

} // namespace MapEditor::Domain::History
