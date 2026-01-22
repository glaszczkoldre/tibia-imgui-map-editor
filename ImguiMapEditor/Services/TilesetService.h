#pragma once

#include "../Domain/Palette/Palette.h"
#include "../Domain/Tileset/TilesetRegistry.h"
#include <filesystem>
#include <memory>

namespace MapEditor::Brushes {
class BrushRegistry;
}

namespace MapEditor::Services {

/**
 * Service responsible for loading and managing tilesets and palettes.
 *
 * This service:
 * - Owns the TilesetRegistry and PaletteRegistry
 * - Loads individual tileset XML files from a directory
 * - Loads palettes from palettes.xml
 * - Provides access to registries via dependency injection
 */
class TilesetService {
public:
  explicit TilesetService(Brushes::BrushRegistry &brushRegistry);
  ~TilesetService() = default;

  // Non-copyable
  TilesetService(const TilesetService &) = delete;
  TilesetService &operator=(const TilesetService &) = delete;

  /**
   * Load tilesets from a data directory.
   * Looks for tileset XML files in dataPath/tilesets/
   *
   * @param dataPath Root data path (contains tilesets/ folder)
   * @return true if at least one tileset was loaded
   */
  bool loadTilesets(const std::filesystem::path &dataPath);

  /**
   * Load palettes from palettes.xml.
   * Must be called AFTER loadTilesets() since palettes reference tilesets.
   *
   * @param dataPath Root data path (contains palettes.xml)
   * @return true if palettes were loaded successfully
   */
  bool loadPalettes(const std::filesystem::path &dataPath);

  /**
   * Check if tilesets have been loaded.
   */
  bool isLoaded() const { return loaded_; }

  /**
   * Get the tileset registry (for dependency injection).
   */
  Domain::Tileset::TilesetRegistry &getTilesetRegistry() {
    return tilesetRegistry_;
  }
  const Domain::Tileset::TilesetRegistry &getTilesetRegistry() const {
    return tilesetRegistry_;
  }

  /**
   * Get the palette registry (for dependency injection).
   */
  Domain::Palette::PaletteRegistry &getPaletteRegistry() {
    return paletteRegistry_;
  }
  const Domain::Palette::PaletteRegistry &getPaletteRegistry() const {
    return paletteRegistry_;
  }

private:
  Brushes::BrushRegistry &brushRegistry_;
  Domain::Tileset::TilesetRegistry tilesetRegistry_;
  Domain::Palette::PaletteRegistry paletteRegistry_;
  bool loaded_ = false;
};

} // namespace MapEditor::Services
