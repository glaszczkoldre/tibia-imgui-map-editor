#include "TileSnapshotCodec.h"
#include <lz4.h>

namespace MapEditor::Domain::History {

std::vector<uint8_t> TileSnapshotCodec::compress(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return {};
    }
    
    // LZ4 worst case: slightly larger than input
    int max_compressed = LZ4_compressBound(static_cast<int>(data.size()));
    std::vector<uint8_t> compressed(max_compressed);
    
    int compressed_size = LZ4_compress_default(
        reinterpret_cast<const char*>(data.data()),
        reinterpret_cast<char*>(compressed.data()),
        static_cast<int>(data.size()),
        max_compressed
    );
    
    if (compressed_size <= 0) {
        // Compression failed - return copy of original
        return data;
    }
    
    compressed.resize(compressed_size);
    compressed.shrink_to_fit();
    return compressed;
}

std::vector<uint8_t> TileSnapshotCodec::decompress(
    const std::vector<uint8_t>& compressed, 
    size_t original_size
) {
    if (compressed.empty() || original_size == 0) {
        return {};
    }
    
    std::vector<uint8_t> decompressed(original_size);
    
    int result = LZ4_decompress_safe(
        reinterpret_cast<const char*>(compressed.data()),
        reinterpret_cast<char*>(decompressed.data()),
        static_cast<int>(compressed.size()),
        static_cast<int>(original_size)
    );
    
    if (result < 0) {
        // Decompression failed
        return {};
    }
    
    return decompressed;
}

bool TileSnapshotCodec::isAvailable() {
    return true;  // LZ4 is always available when linked
}

} // namespace MapEditor::Domain::History
