#include "TilesetXmlWriter.h"

#include "../Domain/Tileset/Tileset.h"
#include "../Brushes/Types/CreatureBrush.h"
#include "../Brushes/Types/RawBrush.h"
#include <spdlog/spdlog.h>

namespace MapEditor::IO {

using namespace Domain::Tileset;

bool TilesetXmlWriter::write(const std::filesystem::path& path,
                              const Tileset& tileset) {
  pugi::xml_document doc;

  // Root tileset node
  auto root = doc.append_child("tileset");
  root.append_attribute("name") = tileset.getName().c_str();

  // Write all entries directly
  const auto& entries = tileset.getEntries();
  for (size_t i = 0; i < entries.size(); ++i) {
    writeEntry(root, tileset, i);
  }

  // Save to file
  bool success = doc.save_file(path.c_str(), "  ");
  if (success) {
    spdlog::info("[TilesetXmlWriter] Saved tileset '{}' to {}",
                 tileset.getName(), path.string());
  } else {
    spdlog::error("[TilesetXmlWriter] Failed to save tileset to {}",
                  path.string());
  }
  return success;
}

void TilesetXmlWriter::writeEntry(pugi::xml_node& parent,
                                   const Tileset& tileset,
                                   size_t entryIndex) {
  const auto& entries = tileset.getEntries();
  if (entryIndex >= entries.size()) return;

  const auto& entry = entries[entryIndex];

  if (isSeparator(entry)) {
    const auto& sep = getSeparator(entry);
    auto node = parent.append_child("separator");
    if (!sep.name.empty()) {
      node.append_attribute("name") = sep.name.c_str();
    }
  } else if (isBrush(entry)) {
    const auto* brush = getBrush(entry);
    if (!brush) return;

    // Check brush type
    if (auto* rawBrush = dynamic_cast<const Brushes::RawBrush*>(brush)) {
      auto node = parent.append_child("item");
      node.append_attribute("id") = rawBrush->getItemId();
    } else if (auto* creatureBrush = dynamic_cast<const Brushes::CreatureBrush*>(brush)) {
      auto node = parent.append_child("creature");
      node.append_attribute("name") = creatureBrush->getName().c_str();
    } else {
      // Generic brush
      auto node = parent.append_child("brush");
      node.append_attribute("name") = brush->getName().c_str();
    }
  }
}

} // namespace MapEditor::IO
