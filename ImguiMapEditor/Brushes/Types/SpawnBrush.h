#pragma once
/**
 * @file SpawnBrush.h
 * @brief Brush for placing spawn points on the map.
 */

#include "Brushes/Core/IBrush.h"

namespace MapEditor::Services {
class BrushSettingsService;
}

namespace MapEditor::Brushes {

/**
 * Brush for placing spawn points directly on the map.
 *
 * Uses BrushSettingsService for default radius.
 * Spawn timer is set from BrushSettingsService.defaultSpawnTime.
 */
class SpawnBrush : public IBrush {
public:
  SpawnBrush();
  ~SpawnBrush() override = default;

  // IBrush interface
  const std::string &getName() const override { return name_; }
  BrushType getType() const override { return BrushType::Spawn; }
  uint32_t getLookId() const override { return 0; } // No item preview

  void draw(Domain::ChunkedMap &map, Domain::Tile *tile,
            const DrawContext &ctx) override;
  void undraw(Domain::ChunkedMap &map, Domain::Tile *tile) override;

  // Spawn-specific
  void setSettingsService(Services::BrushSettingsService *service) {
    settingsService_ = service;
  }

private:
  std::string name_ = "Spawn";
  Services::BrushSettingsService *settingsService_ = nullptr;
};

} // namespace MapEditor::Brushes
