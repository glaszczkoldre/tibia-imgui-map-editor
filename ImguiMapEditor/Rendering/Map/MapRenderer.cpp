#include "Rendering/Passes/GhostFloorRenderer.h"
#include "Rendering/Passes/LightingPass.h"
#include "Rendering/Passes/TerrainPass.h"
#include "Rendering/Passes/WallOutlineRenderer.h"

// ... (Keep other includes)
#include "Core/Config.h"
#include "Rendering/Map/MapRenderer.h"
#include "Rendering/Overlays/WaypointOverlay.h"
#include "Services/ClientDataService.h"
#include "Services/SpriteManager.h"
#include <chrono>
#include <cmath>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace Rendering {

MapRenderer::MapRenderer(Services::ClientDataService *client_data,
                         Services::SpriteManager *sprite_manager)
    : client_data_(client_data), sprite_manager_(sprite_manager) {}

MapRenderer::~MapRenderer() = default;

bool MapRenderer::initialize() {
  if (!sprite_manager_) {
    spdlog::error("Failed to initialize MapRenderer: SpriteManager is null");
    return false;
  }

  // Create sprite batch
  sprite_batch_ = std::make_unique<SpriteBatch>();
  if (!sprite_batch_->initialize()) {
    spdlog::error("Failed to initialize sprite batch");
    return false;
  }

  // Create shared services
  tile_renderer_ = std::make_unique<TileRenderer>(
      *sprite_batch_, *sprite_manager_, client_data_, view_settings_);

  // --- Initialize Rendering Pipeline ---
  render_pipeline_.clear();

  // 1. Terrain Pass (Main map rendering)
  // Note: TerrainPass needs to be initialized with shared references
  auto terrain_pass = std::make_unique<TerrainPass>(
      *tile_renderer_, chunk_visibility_, *sprite_batch_, *sprite_manager_,
      frame_data_collector_);

  render_pipeline_.addPass(std::move(terrain_pass));

  // 2. Ghost Floor Pass
  render_pipeline_.addPass(std::make_unique<GhostFloorRenderer>(
      *tile_renderer_, *sprite_batch_, chunk_visibility_, *sprite_manager_));

  // 3. Wall Outline Pass (Overlay)
  auto wall_pass = std::make_unique<WallOutlineRenderer>(client_data_);
  if (!wall_pass->initialize()) {
    spdlog::warn(
        "Failed to initialize wall outline renderer - overlays disabled");
  }
  render_pipeline_.addPass(std::move(wall_pass));

  // 4. Lighting Pass
  render_pipeline_.addPass(std::make_unique<LightingPass>());

  spdlog::debug("MapRenderer initialized with {} passes",
                render_pipeline_.getPassCount());
  return true;
}

void MapRenderer::setViewSettings(Services::ViewSettings *settings) {
  view_settings_ = settings;
  if (tile_renderer_) {
    tile_renderer_->setViewSettings(settings);
  }
}

void MapRenderer::setLODMode(bool enabled) {
  render_pipeline_.setLODMode(enabled);
}

void MapRenderer::syncViewSettings() {
  if (view_settings_) {
    camera_.setZoom(view_settings_->zoom);
    camera_.setFloor(view_settings_->current_floor);
    show_grid_ = view_settings_->show_grid;
  }
}

void MapRenderer::render(const Domain::ChunkedMap &map, RenderState &state,
                         int viewport_width, int viewport_height,
                         const AnimationTicks &anim_ticks) {
  if (viewport_width < 1 || viewport_height < 1)
    return;

  // Setup frame
  if (!setupFrame(viewport_width, viewport_height)) {
    return;
  }

  // Calculate visible bounds and MVP
  VisibleBounds base_bounds = camera_.getVisibleBounds();

  // Begin frame - clear buffers
  frame_data_collector_.beginFrame();
  state.overlay_collector.clear();

  // Calculate MVP matrix
  const glm::mat4 &view_matrix = camera_.getViewMatrix();
  glm::mat4 mvp = render_target_.getProjection() * view_matrix;

  render_target_.enableBlending();

  // Note: SpriteBatch prepare/begin moved to TerrainPass or individual passes
  // as needed. TerrainPass handles its own batch begin/end.

  // Create Render Context
  RenderContext context{
      map,
      state,
      anim_ticks,
      camera_,
      viewport_width,
      viewport_height,
      sprite_batch_.get(),                            // Shared sprite batch
      mvp,                                            // MVP Matrix
      base_bounds,                                    // Visible bounds
      camera_.getFloor(),                             // Current floor
      frame_data_collector_.getMissingSpriteBuffer(), // Missing sprites buffer
      view_settings_};                                // View settings

  // Execute Pipeline
  render_pipeline_.render(context);

  // End frame and cleanup
  frame_data_collector_.endFrame(sprite_manager_);

  // Note: sprite_batch_->end() called by individual passes if they used it.

  last_draw_calls_ = sprite_batch_->getDrawCallCount();
  render_target_.unbind();
}

// --- Helper method implementations ---

bool MapRenderer::setupFrame(int viewport_width, int viewport_height) {
  camera_.setViewport(viewport_width, viewport_height);
  syncViewSettings();

  if (!render_target_.resize(viewport_width, viewport_height)) {
    return false;
  }

  render_target_.beginPass(glm::vec4(Config::Rendering::VIEWPORT_CLEAR.r,
                                     Config::Rendering::VIEWPORT_CLEAR.g,
                                     Config::Rendering::VIEWPORT_CLEAR.b,
                                     Config::Rendering::VIEWPORT_CLEAR.a));
  return true;
}

// renderToExternalFBO, setters/getters follow...

void MapRenderer::setCameraPosition(float x, float y) {
  camera_.setPosition(x, y);
}

void MapRenderer::setZoom(float zoom) { camera_.setZoom(zoom); }

void MapRenderer::setFloor(int floor) { camera_.setFloor(floor); }

uint32_t MapRenderer::getTextureId() const {
  return render_target_.isValid() ? render_target_.getTextureId() : 0;
}

void MapRenderer::setSelectionProvider(const ISelectionDataProvider *provider) {
  if (tile_renderer_) {
    tile_renderer_->setSelectionProvider(provider);
  }
}

void MapRenderer::setCreatureSimulator(Services::CreatureSimulator *simulator) {
  if (tile_renderer_) {
    tile_renderer_->setCreatureSimulator(simulator);
  }
}
} // namespace Rendering
} // namespace MapEditor
