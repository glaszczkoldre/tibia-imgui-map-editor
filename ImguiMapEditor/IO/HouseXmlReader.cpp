#include "HouseXmlReader.h"
#include "XmlUtils.h"
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace IO {

HouseXmlReader::Result HouseXmlReader::read(const std::filesystem::path& path, Domain::ChunkedMap& map) {
    Result result;
    pugi::xml_document doc;

    pugi::xml_node root = XmlUtils::loadXmlFile(path, "houses", doc, result.error);
    if (!root) {
        return result;
    }

    for (pugi::xml_node houseNode : root.children("house")) {
        uint32_t id = houseNode.attribute("houseid").as_uint();
        if (id == 0) continue;

        auto house = std::make_unique<Domain::House>(id);
        house->name = houseNode.attribute("name").as_string();
        house->rent = houseNode.attribute("rent").as_uint();
        house->town_id = houseNode.attribute("townid").as_uint();
        house->is_guildhall = houseNode.attribute("guildhall").as_bool();
        
        house->entry_position.x = houseNode.attribute("entryx").as_int();
        house->entry_position.y = houseNode.attribute("entryy").as_int();
        house->entry_position.z = houseNode.attribute("entryz").as_int();

        // Add to map dictionary
        // Note: Map class needs addHouse method, we will need to ensure it exists.
        map.addHouse(std::move(house));
        result.houses_loaded++;
    }

    result.success = true;
    return result;
}

} // namespace IO
} // namespace MapEditor
