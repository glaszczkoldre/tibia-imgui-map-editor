#pragma once
#include "../ScriptReader.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Position.h"
#include <string>

namespace MapEditor {

namespace Services {
    class ClientDataService;
}

namespace IO {

struct SecResult;

/**
 * Parses tile content from SEC script format.
 * 
 * SEC tile format:
 *   X-Y: [Flags...], Content={item_id1, item_id2 [Attrs], ...}
 * 
 * Flags: Refresh, NoLogout, ProtectionZone
 * Item Attrs: String="text"
 * Containers: {outer_id, {inner_id1, inner_id2}}
 */
class SecTileParser {
public:
    /**
     * Parse a single sector file.
     * @param script Open ScriptReader on the file
     * @param sector_x/y/z Sector coordinates (tiles = sector * 32 + offset)
     * @param map Map to populate
     * @param client_data For item lookup
     * @param result Accumulates stats
     * @return true if parsing completed without fatal errors
     */
    static bool parseSector(
        ScriptReader& script,
        int sector_x, int sector_y, int sector_z,
        Domain::ChunkedMap& map,
        Services::ClientDataService* client_data,
        SecResult& result);

private:
    SecTileParser() = delete;
    
    /**
     * Parse tile flags and return combined flag value.
     * @param identifier Flag name (lowercase)
     * @return Flag bit or 0 if not recognized
     */
    static uint32_t parseTileFlag(const std::string& identifier);
};

} // namespace IO
} // namespace MapEditor
