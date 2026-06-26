#pragma once
#include "IO/Loader/SourceFragments.h"
#include <filesystem>
#include <vector>
#include <string>

namespace pugi {
    class xml_node;
}

namespace MapEditor::IO {

/**
 * Result of loading items.xml
 */
struct ItemXmlResult {
    bool success = false;
    std::string error;
    std::vector<std::string> warnings;
    std::vector<XmlItemFragment> items;
    size_t items_loaded = 0;
};

/**
 * Reads items.xml and returns game attributes as XmlItemFragments.
 */
class ItemXmlReader {
public:
    ItemXmlReader() = delete;  // Static-only class
    
    /**
     * Load items.xml into override fragments.
     * @param xml_path Path to items.xml
     * @return Result with success status, warnings, and loaded fragments
     */
    [[nodiscard]] static ItemXmlResult load(const std::filesystem::path& xml_path);

private:
    static void parseItemNode(
        const pugi::xml_node& itemNode,
        uint16_t id,
        std::vector<XmlItemFragment>& items);
        
    static void parseAttributes(
        const pugi::xml_node& itemNode,
        XmlItemFragment& item);
};

} // namespace MapEditor::IO
