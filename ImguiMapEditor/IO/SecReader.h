#pragma once
#include "Domain/ChunkedMap.h"
#include <filesystem>
#include <functional>
#include <string>
#include <climits>

namespace MapEditor {

namespace Services {
    class ClientDataService;
}

namespace IO {

/**
 * Result of SEC parsing
 */
struct SecResult {
    bool success = false;
    std::string error;
    
    size_t sector_count = 0;
    size_t tile_count = 0;
    size_t item_count = 0;
    
    // Map bounds (in sector coordinates)
    int sector_x_min = INT_MAX, sector_x_max = INT_MIN;
    int sector_y_min = INT_MAX, sector_y_max = INT_MIN;
    int sector_z_min = INT_MAX, sector_z_max = INT_MIN;
};

/**
 * Progress callback for SEC loading
 */
using SecProgressCallback = std::function<void(int percent, const std::string& status)>;

/**
 * SEC sector map format reader.
 * 
 * Loads all *.sec files from a directory into ChunkedMap.
 * SEC format is the ancient Tibia map format using text-based script files.
 * Each sector file represents a 32x32 tile area.
 * 
 * File naming: XXXX-YYYY-ZZ.sec where X,Y are sector coords, Z is floor.
 * 
 * IMPORTANT: SEC maps use SERVER IDs, requiring items.srv (not items.otb).
 */
class SecReader {
public:
    /**
     * Read all .sec files from directory into map.
     * @param directory Folder containing XXXX-YYYY-ZZ.sec files
     * @param map Target map to populate
     * @param client_data Required (must have items.srv loaded)
     * @param progress Optional progress callback
     * @return Result with success/error and statistics
     */
    static SecResult read(
        const std::filesystem::path& directory,
        Domain::ChunkedMap& map,
        Services::ClientDataService* client_data,
        SecProgressCallback progress = nullptr);

    /**
     * Scan directory to determine map bounds without loading.
     * Useful for UI preview before full load.
     * @param directory Folder containing *.sec files
     * @return Result with sector count and bounds
     */
    static SecResult scanBounds(const std::filesystem::path& directory);

private:
    SecReader() = delete;
    
    /**
     * Load a single sector file.
     */
    static bool readSector(
        const std::filesystem::path& file,
        int sector_x, int sector_y, int sector_z,
        Domain::ChunkedMap& map,
        Services::ClientDataService* client_data,
        SecResult& result);
        
    /**
     * Parse sector coordinates from filename.
     * @param filename e.g. "1015-0996-03.sec"
     * @param out_x, out_y, out_z Output parameters
     * @return true if parsing succeeded
     */
    static bool parseFilename(
        const std::string& filename,
        int& out_x, int& out_y, int& out_z);
};

} // namespace IO
} // namespace MapEditor
