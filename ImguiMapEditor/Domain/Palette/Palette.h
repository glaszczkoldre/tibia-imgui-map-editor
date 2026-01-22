#pragma once

#include "../Tileset/Tileset.h"
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace MapEditor::Domain::Palette {

/**
 * A Palette is a named collection of tilesets that appear in a single window.
 *
 * The palette name is shown as the window title (e.g., "Boss Encounters").
 * The tilesets within are shown in a dropdown (e.g., "Bosses", "Magic").
 *
 * Palettes are defined in palettes.xml and reference tilesets via includes.
 */
class Palette {
public:
  explicit Palette(const std::string &name) : name_(name) {}

  const std::string &getName() const { return name_; }

  void addTileset(Tileset::Tileset *tileset) {
    if (tileset) {
      tilesets_.push_back(tileset);
    }
  }

  const std::vector<Tileset::Tileset *> &getTilesets() const {
    return tilesets_;
  }

  /**
   * Get tileset names for dropdown display.
   */
  std::vector<std::string> getTilesetNames() const {
    std::vector<std::string> names;
    names.reserve(tilesets_.size());
    for (const auto *tileset : tilesets_) {
      names.push_back(tileset->getName());
    }
    return names;
  }

  /**
   * Get tileset by index (for dropdown selection).
   */
  Tileset::Tileset *getTilesetAt(size_t index) const {
    if (index < tilesets_.size()) {
      return tilesets_[index];
    }
    return nullptr;
  }

  /**
   * Get tileset by name.
   */
  Tileset::Tileset *getTileset(const std::string &name) const {
    for (auto *tileset : tilesets_) {
      if (tileset->getName() == name) {
        return tileset;
      }
    }
    return nullptr;
  }

  size_t getTilesetCount() const { return tilesets_.size(); }
  bool isEmpty() const { return tilesets_.empty(); }

  void setSourceFile(const std::filesystem::path &path) { sourceFile_ = path; }
  const std::filesystem::path &getSourceFile() const { return sourceFile_; }

private:
  std::string name_;
  std::vector<Tileset::Tileset *>
      tilesets_; // Non-owning pointers from TilesetRegistry
  std::filesystem::path sourceFile_;
};

/**
 * Registry of all loaded palettes.
 *
 * Palettes are registered when loading palettes.xml.
 * The registry owns the Palette objects.
 *
 * NOTE: This class is NOT a singleton. It should be owned by TilesetService
 * and injected where needed per project dependency injection rules.
 */
class PaletteRegistry {
public:
  PaletteRegistry() = default;
  ~PaletteRegistry() = default;

  // Non-copyable, movable
  PaletteRegistry(const PaletteRegistry &) = delete;
  PaletteRegistry &operator=(const PaletteRegistry &) = delete;
  PaletteRegistry(PaletteRegistry &&) = default;
  PaletteRegistry &operator=(PaletteRegistry &&) = default;

  void registerPalette(std::unique_ptr<Palette> palette) {
    if (palette) {
      std::string name = palette->getName();
      paletteOrder_.push_back(name);
      palettes_[name] = std::move(palette);
    }
  }

  Palette *getPalette(const std::string &name) {
    auto it = palettes_.find(name);
    return it != palettes_.end() ? it->second.get() : nullptr;
  }

  const Palette *getPalette(const std::string &name) const {
    auto it = palettes_.find(name);
    return it != palettes_.end() ? it->second.get() : nullptr;
  }

  /**
   * Get all palette names in registration order.
   * Used for generating ribbon buttons.
   */
  const std::vector<std::string> &getPaletteNames() const {
    return paletteOrder_;
  }

  /**
   * Get all palettes.
   */
  std::vector<Palette *> getAllPalettes() const {
    std::vector<Palette *> result;
    result.reserve(palettes_.size());
    for (const auto &name : paletteOrder_) {
      auto it = palettes_.find(name);
      if (it != palettes_.end()) {
        result.push_back(it->second.get());
      }
    }
    return result;
  }

  void clear() {
    palettes_.clear();
    paletteOrder_.clear();
  }

  size_t size() const { return palettes_.size(); }
  bool empty() const { return palettes_.empty(); }

private:
  std::map<std::string, std::unique_ptr<Palette>> palettes_;
  std::vector<std::string> paletteOrder_; // Maintain insertion order for UI
};

} // namespace MapEditor::Domain::Palette
