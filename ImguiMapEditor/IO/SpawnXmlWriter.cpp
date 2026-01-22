#include "SpawnXmlWriter.h"
#include "Domain/Creature.h"
#include "Domain/Spawn.h"
#include "Domain/Tile.h"
#include <fstream>
#include <map>


namespace MapEditor::IO {

bool SpawnXmlWriter::write(const std::filesystem::path &path,
                           const Domain::ChunkedMap &map) {
  std::ofstream file(path);
  if (!file.is_open()) {
    return false;
  }

  file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  file << "<spawns>\n";

  // Collect spawns by position
  std::map<Domain::Position, const Domain::Spawn *> spawn_map;

  map.forEachTile([&spawn_map](const Domain::Tile *tile) {
    if (!tile)
      return;
    if (tile->hasSpawn()) {
      const Domain::Spawn *spawn = tile->getSpawn();
      if (spawn) {
        spawn_map[spawn->position] = spawn;
      }
    }
  });

  for (const auto &[pos, spawn] : spawn_map) {
    if (!spawn)
      continue;

    file << "\t<spawn centerx=\"" << pos.x << "\"";
    file << " centery=\"" << pos.y << "\"";
    file << " centerz=\"" << static_cast<int>(pos.z) << "\"";
    file << " radius=\"" << spawn->radius << "\">\n";

    // Scan tiles within spawn radius for creatures
    for (int dx = -spawn->radius; dx <= spawn->radius; dx++) {
      for (int dy = -spawn->radius; dy <= spawn->radius; dy++) {
        Domain::Position checkPos(pos.x + dx, pos.y + dy, pos.z);
        const Domain::Tile *tile = map.getTile(checkPos);
        if (!tile || !tile->hasCreature())
          continue;

        const Domain::Creature *creature = tile->getCreature();
        if (!creature)
          continue;

        file << "\t\t<monster name=\"" << creature->name << "\"";
        file << " x=\"" << dx << "\"";
        file << " y=\"" << dy << "\"";
        file << " spawntime=\"" << creature->spawn_time << "\"";
        file << " direction=\"" << creature->direction << "\"/>\n";
      }
    }

    file << "\t</spawn>\n";
  }

  file << "</spawns>\n";
  file.close();

  return true;
}

} // namespace MapEditor::IO
