#pragma once
#include "../ScriptReader.h"
#include "Domain/Item.h"
#include <memory>
#include <vector>

namespace MapEditor {

namespace Services {
    class ClientDataService;
}

namespace IO {

/**
 * Parses items from SEC Content={...} blocks.
 * 
 * Item format:
 *   item_id                    - Simple item
 *   item_id String="text"      - Item with text attribute
 *   item_id, {nested_items}    - Container with contents
 */
class SecItemParser {
public:
    /**
     * Parse item list from Content={...}
     * @param script Positioned after '{'
     * @param client_data For server ID -> item type lookup
     * @return Vector of items in stack order (ground first)
     */
    static std::vector<std::unique_ptr<Domain::Item>> parseItemList(
        ScriptReader& script,
        Services::ClientDataService* client_data);

private:
    SecItemParser() = delete;
    
    /**
     * Parse a single item and its attributes.
     * @param server_id The item's server ID
     * @param script For reading optional attributes
     * @param client_data For item type lookup
     * @return Parsed item or nullptr
     */
    static std::unique_ptr<Domain::Item> parseItem(
        int server_id,
        ScriptReader& script,
        Services::ClientDataService* client_data);
    
    /**
     * Parse nested container contents recursively.
     */
    static void parseContainerContents(
        ScriptReader& script,
        Domain::Item& container,
        Services::ClientDataService* client_data);
};

} // namespace IO
} // namespace MapEditor
