#pragma once
#include "Domain/ChunkedMap.h"
#include <filesystem>

namespace MapEditor::IO {

/**
 * Writes spawn data to XML format.
 */
class SpawnXmlWriter {
public:
    /**
     * Write spawns.xml file.
     * @param path Output file path
     * @param map Map containing spawn data
     * @return true on success
     */
    static bool write(
        const std::filesystem::path& path,
        const Domain::ChunkedMap& map
    );

private:
    SpawnXmlWriter() = delete;
};

} // namespace MapEditor::IO
