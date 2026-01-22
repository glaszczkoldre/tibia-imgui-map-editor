#pragma once

#include <filesystem>
#include <pugixml.hpp>

namespace MapEditor::Domain::Tileset {
class Tileset;
}

namespace MapEditor::IO {

/**
 * Writes tileset data to XML files.
 * Saves in flat format (no categories):
 *   <tileset name="...">
 *     <brush name="..."/>
 *     <item id="..."/>
 *     <creature name="..."/>
 *     <separator name="..."/>
 *   </tileset>
 */
class TilesetXmlWriter {
public:
  /**
   * Write a tileset to an XML file.
   * @param path Destination path
   * @param tileset Tileset to write
   * @return true on success
   */
  static bool write(const std::filesystem::path& path,
                    const Domain::Tileset::Tileset& tileset);

private:
  static void writeEntry(pugi::xml_node& parent,
                         const Domain::Tileset::Tileset& tileset,
                         size_t entryIndex);
};

} // namespace MapEditor::IO
