#pragma once

#include "TilesetEntry.h"
#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace MapEditor::Domain::Tileset {

using Brushes::IBrush;

/**
 * A Tileset is a named collection of entries (brushes, items, creatures,
 * separators).
 *
 * In the new XML-driven system:
 * - Each tileset XML file defines one Tileset
 * - Tilesets are referenced by Palettes
 * - The tileset name appears in the palette's dropdown selector
 *
 * Example: "Undead" tileset contains skeleton, ghost, vampire brushes.
 */
class Tileset {
public:
  explicit Tileset(const std::string &name) : name_(name) {}

  const std::string &getName() const { return name_; }

  void setSourceFile(const std::filesystem::path &path) { sourceFile_ = path; }
  const std::filesystem::path &getSourceFile() const { return sourceFile_; }

  // ===== Entry Access =====

  /**
   * @brief Gets all entries (brushes, items, creatures, separators).
   */
  const std::vector<TilesetEntry> &getEntries() const { return entries_; }

  /**
   * @brief Gets mutable access to entries for reordering operations.
   */
  std::vector<TilesetEntry> &getEntriesMutable() {
    dirty_ = true;
    return entries_;
  }

  /**
   * @brief Gets only the brush entries.
   */
  std::vector<IBrush *> getBrushes() const {
    std::vector<IBrush *> brushes;
    brushes.reserve(entries_.size());
    for (const auto &entry : entries_) {
      if (isBrush(entry)) {
        brushes.push_back(getBrush(entry));
      }
    }
    return brushes;
  }

  // ===== Entry Manipulation =====

  void addBrush(IBrush *brush) {
    if (brush) {
      entries_.emplace_back(brush);
      dirty_ = true;
    }
  }

  void addSeparator(const std::string &name) {
    entries_.emplace_back(TilesetSeparator(name));
    dirty_ = true;
  }

  void insertSeparatorAt(size_t index, const std::string &name) {
    if (index >= entries_.size()) {
      entries_.emplace_back(TilesetSeparator(name));
    } else {
      entries_.insert(entries_.begin() + static_cast<ptrdiff_t>(index),
                      TilesetSeparator(name));
    }
    dirty_ = true;
  }

  /**
   * Insert a brush after a specific brush name.
   */
  void insertBrushAfter(IBrush *brush, const std::string &afterBrushName) {
    if (!brush)
      return;

    if (afterBrushName.empty()) {
      entries_.emplace_back(brush);
      dirty_ = true;
      return;
    }

    auto it =
        std::find_if(entries_.begin(), entries_.end(),
                     [&afterBrushName](const TilesetEntry &entry) {
                       return isBrush(entry) && getBrush(entry) &&
                              getBrush(entry)->getName() == afterBrushName;
                     });

    if (it != entries_.end()) {
      entries_.insert(++it, brush);
    } else {
      entries_.emplace_back(brush);
    }
    dirty_ = true;
  }

  /**
   * Move an entry from one index to another.
   */
  void moveEntry(size_t fromIndex, size_t toIndex) {
    if (fromIndex >= entries_.size() || toIndex >= entries_.size() ||
        fromIndex == toIndex) {
      return;
    }

    TilesetEntry entry = std::move(entries_[fromIndex]);
    entries_.erase(entries_.begin() + static_cast<ptrdiff_t>(fromIndex));

    if (toIndex > fromIndex) {
      toIndex--;
    }

    entries_.insert(entries_.begin() + static_cast<ptrdiff_t>(toIndex),
                    std::move(entry));
    dirty_ = true;
  }

  /**
   * Swap two entries.
   */
  void swapEntries(size_t indexA, size_t indexB) {
    if (indexA >= entries_.size() || indexB >= entries_.size() ||
        indexA == indexB) {
      return;
    }
    std::swap(entries_[indexA], entries_[indexB]);
    dirty_ = true;
  }

  /**
   * Remove an entry at the given index.
   */
  void removeEntry(size_t index) {
    if (index < entries_.size()) {
      entries_.erase(entries_.begin() + static_cast<ptrdiff_t>(index));
      dirty_ = true;
    }
  }

  /**
   * Update the name of a separator at the given index.
   */
  void setSeparatorName(size_t index, const std::string &name) {
    if (index < entries_.size() && isSeparator(entries_[index])) {
      std::get<TilesetSeparator>(entries_[index]).name = name;
      dirty_ = true;
    }
  }

  /**
   * Insert an entry at a specific index.
   */
  void insertEntryAt(size_t index, const TilesetEntry &entry) {
    if (index > entries_.size()) {
      index = entries_.size();
    }
    entries_.insert(entries_.begin() + static_cast<ptrdiff_t>(index), entry);
    dirty_ = true;
  }

  bool isEmpty() const { return entries_.empty(); }
  size_t size() const { return entries_.size(); }

  bool isDirty() const { return dirty_; }
  void clearDirty() { dirty_ = false; }

private:
  std::string name_;
  std::filesystem::path sourceFile_;
  std::vector<TilesetEntry> entries_;
  bool dirty_ = false;
};

} // namespace MapEditor::Domain::Tileset
