#pragma once
#include "Domain/ChunkedMap.h"
#include <filesystem>

namespace MapEditor::IO {

/**
 * Writes house data to XML format.
 */
class HouseXmlWriter {
public:
    /**
     * Write houses.xml file.
     * @param path Output file path
     * @param map Map containing house data
     * @return true on success
     */
    static bool write(
        const std::filesystem::path& path,
        const Domain::ChunkedMap& map
    );

private:
    HouseXmlWriter() = delete;
};

} // namespace MapEditor::IO
