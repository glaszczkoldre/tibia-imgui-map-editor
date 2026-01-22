#pragma once
#include "Brushes/Core/IBrush.h"
#include <string>

namespace MapEditor::Brushes {

/**
 * Brush for placing named waypoints on tiles.
 *
 * Waypoints are named navigation points used for NPC routes, etc.
 */
class WaypointBrush : public IBrush {
public:
  WaypointBrush();
  ~WaypointBrush() override = default;

  // IBrush interface
  void draw(Domain::ChunkedMap &map, Domain::Tile *tile,
            const DrawContext &ctx) override;
  void undraw(Domain::ChunkedMap &map, Domain::Tile *tile) override;

  const std::string &getName() const override { return name_; }
  BrushType getType() const override { return BrushType::Waypoint; }
  uint32_t getLookId() const override { return 0; }

  // Waypoint name configuration
  void setWaypointName(const std::string &name) { waypointName_ = name; }
  const std::string &getWaypointName() const { return waypointName_; }

private:
  std::string name_ = "Waypoint";
  std::string waypointName_; // Name for the waypoint to place
};

} // namespace MapEditor::Brushes
