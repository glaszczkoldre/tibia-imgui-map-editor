#pragma once
#include "Domain/ItemType.h"
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>

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
    size_t items_loaded = 0;
    size_t items_merged = 0;
};

/**
 * Reads items.xml and merges game attributes into ItemType objects.
 * Follows RME's loadFromGameXml pattern exactly.
 * Supports single item IDs and ID ranges (fromid/toid).
 * 
 * Stateless reader - all methods are static.
 */
class ItemXmlReader {
public:
    ItemXmlReader() = delete;  // Static-only class
    
    /**
     * Load and merge items.xml into existing ItemTypes (loaded from OTB).
     * @param xml_path Path to items.xml
     * @param items Vector of ItemTypes (from OTB) to augment
     * @param server_id_index Map of server_id → index for fast lookup
     * @return Result with success status and statistics
     */
    [[nodiscard]] static ItemXmlResult load(
        const std::filesystem::path& xml_path,
        std::vector<Domain::ItemType>& items,
        const std::unordered_map<uint16_t, size_t>& server_id_index);

private:
    /**
     * Apply properties to a single ItemType by ID.
     */
    static bool applyToItem(
        uint16_t id,
        const pugi::xml_node& itemNode,
        std::vector<Domain::ItemType>& items,
        const std::unordered_map<uint16_t, size_t>& server_id_index,
        std::vector<std::string>& warnings);

    /**
     * Parse attribute child nodes (<attribute key="..." value="..." />).
     */
    static void parseAttributes(
        const pugi::xml_node& itemNode,
        Domain::ItemType& item);
};

} // namespace MapEditor::IO
