#include "Rendering/Camera/ViewCamera.h"
#include "Core/Config.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>


namespace MapEditor::Rendering {

ViewCamera::ViewCamera() = default;

void ViewCamera::setPosition(float x, float y) {
  position_ = glm::vec2(x, y);
  updateMatrix();
}

void ViewCamera::setZoom(float zoom) {
  // Clamp to Config limits (duplicated from MapRenderer logic for safety)
  zoom_ = std::clamp(zoom, Config::Camera::MIN_ZOOM, Config::Camera::MAX_ZOOM);
  updateMatrix();
}

void ViewCamera::setFloor(int floor) {
  // [PR FIX] Use Config constants for floor limits instead of magic numbers
  floor_ = std::clamp(floor, static_cast<int>(Config::Map::MIN_FLOOR),
                      static_cast<int>(Config::Map::MAX_FLOOR));
  // Floor doesn't affect view matrix, but is part of camera state
}

void ViewCamera::setViewport(int width, int height) {
  if (width != viewport_width_ || height != viewport_height_) {
    viewport_width_ = width;
    viewport_height_ = height;
    updateMatrix();
  }
}

void ViewCamera::updateMatrix() {
  // Same logic as MapRenderer::render
  float tile_size = Config::Rendering::TILE_SIZE;

  glm::vec3 center(viewport_width_ / 2.0f, viewport_height_ / 2.0f, 0.0f);
  glm::vec3 camera_pos_px(-position_.x * tile_size, -position_.y * tile_size,
                          0.0f);

  view_matrix_ = glm::mat4(1.0f);
  view_matrix_ = glm::translate(view_matrix_, center);
  view_matrix_ = glm::scale(view_matrix_, glm::vec3(zoom_, zoom_, 1.0f));
  view_matrix_ = glm::translate(view_matrix_, camera_pos_px);
}

Domain::Position ViewCamera::screenToTile(float screen_x,
                                          float screen_y) const {
  float tile_size = Config::Rendering::TILE_SIZE;

  float tile_x =
      (screen_x - viewport_width_ / 2.0f) / (tile_size * zoom_) + position_.x;
  float tile_y =
      (screen_y - viewport_height_ / 2.0f) / (tile_size * zoom_) + position_.y;

  return Domain::Position(static_cast<int32_t>(tile_x),
                          static_cast<int32_t>(tile_y),
                          static_cast<int16_t>(floor_));
}

glm::vec2 ViewCamera::tileToScreen(const Domain::Position &pos) const {
  float tile_size = Config::Rendering::TILE_SIZE;

  float screen_x =
      (pos.x - position_.x) * tile_size * zoom_ + viewport_width_ / 2.0f;
  float screen_y =
      (pos.y - position_.y) * tile_size * zoom_ + viewport_height_ / 2.0f;

  return glm::vec2(screen_x, screen_y);
}

VisibleBounds ViewCamera::getVisibleBounds() const {
  return VisibleBounds::calculate(position_.x, position_.y, zoom_,
                                  viewport_width_, viewport_height_,
                                  Config::Rendering::TILE_SIZE);
}

} // namespace MapEditor::Rendering
