#pragma once
#include "Brushes/Core/IBrush.h"
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


namespace MapEditor::Brushes {

/**
 * Central registry for all brushes.
 * Manages brush ownership and lookup.
 *
 * NOTE: Tileset management is now handled by Domain::Tileset::TilesetRegistry.
 * This class only handles brush instances.
 */
class BrushRegistry {
public:
  BrushRegistry() = default;
  ~BrushRegistry() = default;

  // Non-copyable
  BrushRegistry(const BrushRegistry &) = delete;
  BrushRegistry &operator=(const BrushRegistry &) = delete;

  // ========== Brush Management ==========

  /**
   * Add a brush to the registry (takes ownership).
   */
  void addBrush(std::unique_ptr<IBrush> brush);

  /**
   * Get a brush by name. Returns nullptr if not found.
   */
  IBrush *getBrush(const std::string &name) const;

  /**
   * Get or create a RAW brush for an item ID.
   * RAW brushes are created on-demand and cached.
   */
  IBrush *getOrCreateRAWBrush(uint16_t itemId);

  /**
   * Clear all brushes from the registry.
   */
  void clear();

private:
  // Brush ownership
  std::map<std::string, std::unique_ptr<IBrush>> named_brushes_;
  std::map<uint16_t, std::unique_ptr<IBrush>> raw_brushes_; // ItemID -> Brush
};

} // namespace MapEditor::Brushes
