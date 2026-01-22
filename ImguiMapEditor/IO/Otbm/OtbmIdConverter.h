#pragma once

#include <filesystem>
#include <string>

namespace MapEditor {
namespace Services {
    class ClientDataService;
}
namespace IO {

/**
 * ID conversion mode
 */
enum class ConversionDirection {
    ServerToClient,  // Server ID -> Client ID
    ClientToServer   // Client ID -> Server ID
};

/**
 * Result of binary OTBM conversion
 */
struct OtbmConvertResult {
    bool success = false;
    std::string error;
    
    size_t items_converted = 0;
    size_t items_skipped = 0;
};

/**
 * Binary-level OTBM ID converter.
 * 
 * Reads an OTBM file, swaps item IDs in-place, writes to output.
 * Does NOT load items into domain objects - just swaps the raw bytes.
 * Similar to the JavaScript OTBM2JSON converter approach.
 */
class OtbmIdConverter {
public:
    /**
     * Convert item IDs in an OTBM file.
     * 
     * @param input_path Source OTBM file
     * @param output_path Destination OTBM file
     * @param direction Conversion direction (server->client or client->server)
     * @param client_data Client data for ID lookups
     * @return Conversion result with statistics
     */
    static OtbmConvertResult convert(
        const std::filesystem::path& input_path,
        const std::filesystem::path& output_path,
        ConversionDirection direction,
        Services::ClientDataService* client_data
    );
};

} // namespace IO
} // namespace MapEditor
