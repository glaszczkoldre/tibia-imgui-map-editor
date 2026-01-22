#include "LassoSelectionProcessor.hpp"
#include "Application/EditorSession.h"
#include "Application/Selection/FloorScopeHelper.h"
#include "Core/Config.h"
#include <algorithm>

namespace MapEditor {
namespace Application {
namespace Selection {

bool LassoSelectionProcessor::isPointInPolygon(
    const glm::vec2 &point, std::span<const glm::vec2> polygon) {
  if (polygon.size() < 3)
    return false;

  int crossings = 0;
  for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
    if ((polygon[i].y > point.y) != (polygon[j].y > point.y)) {
      float x = (polygon[j].x - polygon[i].x) * (point.y - polygon[i].y) /
                    (polygon[j].y - polygon[i].y) +
                polygon[i].x;
      if (point.x < x)
        crossings++;
    }
  }
  return (crossings % 2) == 1;
}

void LassoSelectionProcessor::process(
    AppLogic::EditorSession *session, const Domain::ICoordinateTransformer &camera,
    const Domain::SelectionSettings *selection_settings,
    std::span<const glm::vec2> polygon_points, SelectionMode mode) {

  if (!session || polygon_points.size() < 3) {
    return;
  }

  // Clear selection if in Replace mode
  if (mode == SelectionMode::Replace) {
    session->getSelectionService().clear();
  }

  // Calculate bounding box of lasso polygon
  float min_sx = polygon_points[0].x, max_sx = polygon_points[0].x;
  float min_sy = polygon_points[0].y, max_sy = polygon_points[0].y;
  for (const auto &pt : std::span(polygon_points).subspan(1)) {
    min_sx = std::min(min_sx, pt.x);
    max_sx = std::max(max_sx, pt.x);
    min_sy = std::min(min_sy, pt.y);
    max_sy = std::max(max_sy, pt.y);
  }

  // Convert bounding box to tile coordinates
  Domain::Position min_tile = camera.screenToTile({min_sx, min_sy});
  Domain::Position max_tile = camera.screenToTile({max_sx, max_sy});

  // Use FloorScopeHelper to determine floor range
  int16_t current_floor = camera.getCurrentFloor();
  Domain::SelectionFloorScope scope =
      selection_settings ? selection_settings->floor_scope
                         : Domain::SelectionFloorScope::CurrentFloor;
  auto floor_range = AppLogic::getFloorRange(scope, current_floor);

  // Iterate tiles in bounding box and test point-in-polygon
  float tile_size_screen =
      camera.getZoom() * Config::Rendering::TILE_SIZE; // Full tile size in screen coords

  // Iterate over all floors in range
  for (int16_t floor = floor_range.start_z; floor >= floor_range.end_z;
       --floor) {
    for (int32_t ty = min_tile.y; ty <= max_tile.y; ++ty) {
      for (int32_t tx = min_tile.x; tx <= max_tile.x; ++tx) {
        Domain::Position tile_pos{tx, ty, floor};
        glm::vec2 tile_tl = camera.tileToScreen(tile_pos); // Top-left corner

        // Check all 4 corners of the tile
        std::array<glm::vec2, 4> corners = {{
            tile_tl,                                              // Top-left
            {tile_tl.x + tile_size_screen, tile_tl.y},            // Top-right
            {tile_tl.x, tile_tl.y + tile_size_screen},            // Bottom-left
            {tile_tl.x + tile_size_screen, tile_tl.y + tile_size_screen} // Bottom-right
        }};

        bool any_corner_inside = false;
        for (const auto &corner : corners) {
          if (isPointInPolygon(corner, polygon_points)) {
            any_corner_inside = true;
            break;
          }
        }

        if (any_corner_inside) {
          if (mode == SelectionMode::Subtract) {
            // Deselect all at this position
            session->getSelectionService().removeAllAt(tile_pos);
          } else {
            // Add tile to selection (applies to both Add and Replace modes)
            session->getSelectionService().selectTile(session->getMap(),
                                                      tile_pos);
          }
        }
      }
    }
  }
}

} // namespace Selection
} // namespace Application
} // namespace MapEditor
