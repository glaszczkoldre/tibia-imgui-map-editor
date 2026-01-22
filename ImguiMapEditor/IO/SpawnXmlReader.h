#pragma once
#include "Domain/Spawn.h"
#include "Domain/ChunkedMap.h"
#include <filesystem>
#include <string>

namespace MapEditor {
namespace IO {

class SpawnXmlReader {
public:
    struct Result {
        bool success = false;
        std::string error;
        int spawns_loaded = 0;
        int creatures_loaded = 0;
    };

    /**
     * Reads spawns from XML and populates the map.
     * @param path Path to the spawn.xml file
     * @param map Target map to populate
     * @return Result object
     */
    static Result read(const std::filesystem::path& path, Domain::ChunkedMap& map);
};

} // namespace IO
} // namespace MapEditor
