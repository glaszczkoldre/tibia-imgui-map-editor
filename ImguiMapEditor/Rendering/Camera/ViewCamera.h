#pragma once
#include <glm/glm.hpp>
#include "Domain/Position.h"
#include "Rendering/Visibility/VisibleBounds.h"

namespace MapEditor::Rendering {

/**
 * Manages rendering camera state and coordinate transformations.
 *
 * NOTE: Viewport dimensions ARE stored here for coordinate transformation
 * purposes (screenToTile, tileToScreen, view matrix calculation). However,
 * MapRenderer maintains its own viewport copy and does NOT read these values
 * for rendering decisions - ensuring rendering code doesn't depend on camera.
 */
class ViewCamera {
public:
  ViewCamera();

  // Core state setters
  void setPosition(float x, float y);
  void setZoom(float zoom);
  void setFloor(int floor);
  void setViewport(int width, int height);

  // State getters
  glm::vec2 getPosition() const { return position_; }
  float getX() const { return position_.x; }
  float getY() const { return position_.y; }
  float getZoom() const { return zoom_; }
  int getFloor() const { return floor_; }
  int getViewportWidth() const { return viewport_width_; }
  int getViewportHeight() const { return viewport_height_; }

  // Matrices
  const glm::mat4 &getViewMatrix() const { return view_matrix_; }

  // Coordinate Transformations
  Domain::Position screenToTile(float screen_x, float screen_y) const;
  glm::vec2 tileToScreen(const Domain::Position &pos) const;

  /**
   * Calculates visible tile bounds based on current camera state.
   * Uses Config::Rendering::TILE_SIZE internally.
   */
  VisibleBounds getVisibleBounds() const;

private:
  void updateMatrix();

  glm::vec2 position_{0.0f, 0.0f};
  float zoom_ = 1.0f;
  int floor_ = 7;
  int viewport_width_ = 1;
  int viewport_height_ = 1;

  glm::mat4 view_matrix_{1.0f};
};

} // namespace MapEditor::Rendering
