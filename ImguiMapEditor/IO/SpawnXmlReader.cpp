#include "SpawnXmlReader.h"
#include "XmlUtils.h"
#include "Domain/Creature.h"
#include <spdlog/spdlog.h>
#include <algorithm> // for max

namespace MapEditor {
namespace IO {

SpawnXmlReader::Result SpawnXmlReader::read(const std::filesystem::path& path, Domain::ChunkedMap& map) {
    Result result;
    pugi::xml_document doc;

    pugi::xml_node root = XmlUtils::loadXmlFile(path, "spawns", doc, result.error);
    if (!root) {
        return result;
    }

    for (pugi::xml_node spawnNode : root.children("spawn")) {
        Domain::Position spawnPos;
        spawnPos.x = spawnNode.attribute("centerx").as_int();
        spawnPos.y = spawnNode.attribute("centery").as_int();
        spawnPos.z = spawnNode.attribute("centerz").as_int();
        
        int radius = spawnNode.attribute("radius").as_int();
        if (radius < 1) radius = 5; // Default fallback

        // Create spawn on the center tile
        auto* centerTile = map.getOrCreateTile(spawnPos);
        if (!centerTile) {
            spdlog::warn("Could not create tile for spawn at {},{},{}", spawnPos.x, spawnPos.y, spawnPos.z);
            continue;
        }

        // Check if spawn already exists (embedded OTBM spawn vs XML spawn)
        if (centerTile->getSpawn()) {
            spdlog::warn("Duplicate spawn at {},{},{} (skipping XML entry)", spawnPos.x, spawnPos.y, spawnPos.z);
            continue;
        }

        // Spawn only stores position and radius (no creature list anymore)
        auto spawn = std::make_unique<Domain::Spawn>(spawnPos, radius);

        // Create creatures on their actual tiles (spawnPos + offset)
        for (pugi::xml_node creatureNode : spawnNode.children()) {
            std::string tagName = creatureNode.name();
            if (tagName != "monster" && tagName != "npc") continue;

            std::string name = creatureNode.attribute("name").as_string();
            int spawn_time = creatureNode.attribute("spawntime").as_int();
            int offset_x = creatureNode.attribute("x").as_int();
            int offset_y = creatureNode.attribute("y").as_int();
            int direction = creatureNode.attribute("direction").as_int(2); // Default: South

            // Calculate absolute position
            Domain::Position creaturePos;
            creaturePos.x = spawnPos.x + offset_x;
            creaturePos.y = spawnPos.y + offset_y;
            creaturePos.z = spawnPos.z;

            // Get or create tile for creature
            auto* creatureTile = map.getOrCreateTile(creaturePos);
            if (!creatureTile) {
                spdlog::warn("Could not create tile for creature {} at {},{},{}", 
                    name, creaturePos.x, creaturePos.y, creaturePos.z);
                continue;
            }

            // Create creature on the tile
            auto creature = std::make_unique<Domain::Creature>(name, spawn_time, direction);
            creatureTile->setCreature(std::move(creature));
            
#ifndef NDEBUG
            spdlog::info("[SpawnXmlReader] Placed creature '{}' at ({},{},{})", 
                name, creaturePos.x, creaturePos.y, creaturePos.z);
#endif
            
            result.creatures_loaded++;
        }

        centerTile->setSpawn(std::move(spawn));
        result.spawns_loaded++;
    }

    result.success = true;
    return result;
}

} // namespace IO
} // namespace MapEditor

