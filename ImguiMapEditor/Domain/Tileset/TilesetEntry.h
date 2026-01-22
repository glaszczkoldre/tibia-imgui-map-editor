#pragma once

#include "../../Brushes/Core/IBrush.h"
#include <string>
#include <variant>


namespace MapEditor::Domain::Tileset {

/**
 * Represents a named separator line in a tileset category.
 * Used for visual organization of tiles within a category.
 */
struct TilesetSeparator {
  std::string name; // Display label, e.g. "--- Floor Tiles ---"

  TilesetSeparator() = default;
  explicit TilesetSeparator(std::string separatorName)
      : name(std::move(separatorName)) {}
};

/**
 * A tileset category entry can be either:
 * - A brush (item/terrain/creature/etc.)
 * - A separator (visual divider with optional name)
 */
using TilesetEntry = std::variant<Brushes::IBrush *, TilesetSeparator>;

/**
 * Helper functions for working with TilesetEntry
 */
inline bool isSeparator(const TilesetEntry &entry) {
  return std::holds_alternative<TilesetSeparator>(entry);
}

inline bool isBrush(const TilesetEntry &entry) {
  return std::holds_alternative<Brushes::IBrush *>(entry);
}

inline Brushes::IBrush *getBrush(const TilesetEntry &entry) {
  return std::get<Brushes::IBrush *>(entry);
}

inline const TilesetSeparator &getSeparator(const TilesetEntry &entry) {
  return std::get<TilesetSeparator>(entry);
}

} // namespace MapEditor::Domain::Tileset
