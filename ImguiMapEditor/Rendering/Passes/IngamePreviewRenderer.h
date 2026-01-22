#pragma once
#include "Core/Config.h"
#include "Domain/ChunkedMap.h"
#include "Rendering/Backend/SpriteBatch.h"
#include "Rendering/Light/LightManager.h"
#include "Rendering/Map/TileRenderer.h"
#include "Rendering/Visibility/FloorVisibilityCalculator.h"
#include <chrono>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>

namespace MapEditor {

namespace Services {
class SpriteManager;
class ClientDataService;
struct ViewSettings;
} // namespace Services

namespace Rendering {

/**
 * Renderer for in-game preview windows (IngameBox).
 *
 * Renders to an external FBO without managing the framebuffer itself.
 * Designed for the 15x11 tile preview window that shows what a player
 * would see in-game.
 *
 * Uses FloorVisibilityCalculator for accurate OTClient-style floor rendering.
 */
class IngamePreviewRenderer {
public:
  IngamePreviewRenderer(TileRenderer &tile_renderer, SpriteBatch &sprite_batch,
                        Services::SpriteManager &sprite_manager,
                        Services::ClientDataService *client_data);

  /**
   * Render preview to currently bound framebuffer.
   *
   * Caller is responsible for:
   * - Binding their own FBO before calling
   * - Unbinding their FBO after calling
   *
   * @param map Map to render
   * @param viewport_width Preview viewport width
   * @param viewport_height Preview viewport height
   * @param camera_x Camera X position in tiles
   * @param camera_y Camera Y position in tiles
   * @param floor Floor to render
   * @param zoom Zoom level
   */
  void render(const Domain::ChunkedMap &map, int viewport_width,
              int viewport_height, float camera_x, float camera_y, int floor,
              float zoom, Services::ViewSettings *view_settings = nullptr);

private:
  void updateProjection(int width, int height);

  TileRenderer &tile_renderer_;
  SpriteBatch &sprite_batch_;
  Services::SpriteManager &sprite_manager_;
  Services::ClientDataService *client_data_;

  // Floor visibility calculator (OTClient algorithm)
  std::unique_ptr<FloorVisibilityCalculator> floor_calculator_;

  // Light system (independent from MapRenderer)
  std::unique_ptr<LightManager> light_manager_;
  uint8_t last_ambient_light_ = 255;

  // Fading state
  int last_player_z_ = -1;
  int last_camera_x_ = -1;
  int last_camera_y_ = -1;
  int cached_first_visible_floor_ = 7;

  // Timers for each floor's opacity (0.0 to 1.0)
  std::unordered_map<int, double> fading_floor_progress_;

  // Timer management
  std::chrono::time_point<std::chrono::steady_clock> last_frame_time_;

  glm::mat4 projection_{1.0f};
  glm::mat4 view_{1.0f};
};

} // namespace Rendering
} // namespace MapEditor
