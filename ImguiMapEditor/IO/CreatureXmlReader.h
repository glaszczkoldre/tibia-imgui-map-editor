#pragma once
#include "Domain/CreatureType.h"
#include <filesystem>
#include <vector>
#include <string>
#include <memory>

namespace pugi {
    class xml_node;
}

namespace MapEditor::IO {

/**
 * Result of loading creatures.xml
 */
struct CreatureXmlResult {
    bool success = false;
    std::string error;
    std::vector<std::string> warnings;
    std::vector<std::unique_ptr<Domain::CreatureType>> creatures;
};

/**
 * Reads creatures.xml following RME format.
 * Supports both <monsters> and <npcs> sections.
 * 
 * XML Format:
 * <creatures>
 *   <monsters>
 *     <monster name="Demon" looktype="35" />
 *     <monster name="Achad" looktype="146" lookhead="95" lookbody="93" 
 *              looklegs="38" lookfeet="59" lookaddon="3" />
 *   </monsters>
 *   <npcs>
 *     <npc name="Sam" looktype="131" ... />
 *   </npcs>
 * </creatures>
 * 
 * Stateless reader - all methods are static.
 */
class CreatureXmlReader {
public:
    CreatureXmlReader() = delete;  // Static-only class
    
    /**
     * Load creatures from creatures.xml file.
     * @param path Path to creatures.xml
     * @return Result with success status and loaded creatures
     */
    [[nodiscard]] static CreatureXmlResult read(const std::filesystem::path& path);

private:
    /**
     * Parse creature node (monster or npc).
     * @param node XML node to parse
     * @param isNpc True if this is an NPC, false for monster
     * @param warnings Collection for parsing warnings
     * @return Parsed CreatureType or nullptr on error
     */
    static std::unique_ptr<Domain::CreatureType> parseCreatureNode(
        const pugi::xml_node& node,
        bool isNpc,
        std::vector<std::string>& warnings);
};

} // namespace MapEditor::IO
