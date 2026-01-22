#include "TilesetService.h"

#include "../Brushes/BrushRegistry.h"
#include "../IO/PaletteXmlReader.h"
#include "../IO/TilesetXmlReader.h"
#include <spdlog/spdlog.h>

namespace MapEditor::Services {

TilesetService::TilesetService(Brushes::BrushRegistry &brushRegistry)
    : brushRegistry_(brushRegistry) {}

bool TilesetService::loadTilesets(const std::filesystem::path &dataPath) {
  // Look for tileset XML files in the data directory
  std::filesystem::path tilesetsPath = dataPath / "tilesets";

  if (!std::filesystem::exists(tilesetsPath)) {
    spdlog::warn("[TilesetService] Tilesets directory not found: {}",
                 tilesetsPath.string());
    return false;
  }

  // Pass owned tilesetRegistry_ to reader
  IO::TilesetXmlReader reader(brushRegistry_, tilesetRegistry_);
  int loadedCount = 0;

  // Use recursive iterator to find tilesets in subdirectories
  for (const auto &entry :
       std::filesystem::recursive_directory_iterator(tilesetsPath)) {
    if (entry.is_regular_file() && entry.path().extension() == ".xml") {
      if (reader.loadTilesetFile(entry.path())) {
        loadedCount++;
      }
    }
  }

  spdlog::info("[TilesetService] Loaded {} tileset files", loadedCount);
  spdlog::info("[TilesetService] Total tilesets in registry: {}",
               tilesetRegistry_.getAllTilesets().size());

  loaded_ = loadedCount > 0;
  return loaded_;
}

bool TilesetService::loadPalettes(const std::filesystem::path &dataPath) {
  std::filesystem::path palettesPath = dataPath / "palettes.xml";

  if (!std::filesystem::exists(palettesPath)) {
    spdlog::warn("[TilesetService] palettes.xml not found at: {}",
                 palettesPath.string());
    return false;
  }

  // Pass owned registries to reader
  IO::PaletteXmlReader reader(tilesetRegistry_, paletteRegistry_);
  if (!reader.load(palettesPath)) {
    spdlog::error("[TilesetService] Failed to load palettes.xml");
    return false;
  }

  spdlog::info("[TilesetService] Loaded {} palettes",
               paletteRegistry_.getPaletteNames().size());

  return true;
}

} // namespace MapEditor::Services
