#pragma once
#include "Core/Config.h"
#include "Domain/ItemType.h"
#include "Domain/Tile.h"
#include "Rendering/Animation/AnimationTicks.h"
#include "Rendering/Backend/SpriteBatch.h"
#include "Rendering/Overlays/OverlayCollector.h"
#include "Rendering/Passes/SpawnTintPass.h"
#include "Rendering/Resources/AtlasManager.h"
#include "Rendering/Tile/CreatureRenderer.h"
#include "Rendering/Tile/GroundRenderer.h"
#include "Rendering/Tile/ItemRenderer.h"
#include "Rendering/Tile/TileColor.h"
#include "Rendering/Utils/SpriteEmitter.h"
#include "Services/SecondaryClientConstants.h"
#include "Services/ViewSettings.h"
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace MapEditor {

namespace Services {
class ClientDataService;
class SpriteManager;
class SecondaryClientData;
class CreatureSimulator;
} // namespace Services

namespace Rendering {

// Forward declaration
class ISelectionDataProvider;

/**
 * Handles sprite queueing for tiles and items.
 *
 * Extracted from MapRenderer to separate tile rendering logic
 * from render pipeline orchestration.
 */
class TileRenderer {
public:
  static constexpr float TILE_SIZE = Config::Rendering::TILE_SIZE;

  TileRenderer(SpriteBatch &sprite_batch,
               Services::SpriteManager &sprite_manager,
               Services::ClientDataService *client_data,
               Services::ViewSettings *view_settings);

  /**
   * Queue all sprites for a tile (ground + items).
   * @param tile The tile to render
   * @param screen_x Screen X position
   * @param screen_y Screen Y position
   * @param zoom Current zoom level
   * @param anim_ticks Pre-calculated animation ticks
   * @param missing_sprites Output vector for sprites not yet loaded
   * @param overlay_collector Optional: Collects overlay data (tooltips, spawns,
   * etc)
   * @param alpha Override alpha (1.0 = normal, <1.0 = ghost)
   */
  void queueTile(const Domain::Tile &tile, float screen_x, float screen_y,
                 float zoom, const AnimationTicks &anim_ticks,
                 std::vector<uint32_t> &missing_sprites,
                 OverlayCollector *overlay_collector = nullptr,
                 float alpha = 1.0f);

  // Explicit coordinate overload (Performance Optimization)
  void queueTile(const Domain::Tile &tile, int tile_x, int tile_y, int tile_z,
                 float screen_x, float screen_y, float zoom,
                 const AnimationTicks &anim_ticks,
                 std::vector<uint32_t> &missing_sprites,
                 OverlayCollector *overlay_collector = nullptr,
                 float alpha = 1.0f);

  /**
   * Queue sprites for a single item type with explicit color.
   * @param accumulated_elevation Optional: tracks elevation for stacking items
   * @param sprite_id_offset Optional: offset to add to all sprite IDs (for
   * secondary client)
   */
  void queueItemWithColor(const Domain::ItemType *item_type, float screen_x,
                          float screen_y, float size, int tile_x, int tile_y,
                          int tile_z, const AnimationTicks &anim_ticks,
                          std::vector<uint32_t> &missing_sprites, float r,
                          float g, float b, float alpha = 1.0f,
                          float *accumulated_elevation = nullptr,
                          const Domain::Item *item_inst = nullptr,
                          uint32_t sprite_id_offset = 0);

  /**
   * Queue sprites for a single item type (white color).
   * @param accumulated_elevation Optional: tracks elevation for stacking items
   */
  void queueItem(const Domain::ItemType *item_type, float screen_x,
                 float screen_y, float size, int tile_x, int tile_y, int tile_z,
                 const AnimationTicks &anim_ticks,
                 std::vector<uint32_t> &missing_sprites, float alpha = 1.0f,
                 float *accumulated_elevation = nullptr,
                 const Domain::Item *item_inst = nullptr);

  /**
   * Queue tile sprites to a vector for caching (instead of SpriteBatch).
   * Used by chunk caching to store geometry for later replay.
   *
   * @param tile The tile to render
   * @param screen_x Screen X position (world coords, not scaled)
   * @param screen_y Screen Y position (world coords, not scaled)
   * @param anim_ticks Pre-calculated animation ticks
   * @param missing_sprites Output vector for sprites not yet loaded
   * @param output_sprites Output vector for generated sprite instances
   * @param alpha Override alpha (1.0 = normal, <1.0 = ghost)
   */
  void queueTileToCache(const Domain::Tile &tile, float screen_x,
                        float screen_y, const AnimationTicks &anim_ticks,
                        std::vector<uint32_t> &missing_sprites,
                        std::vector<SpriteInstance> &output_sprites,
                        float alpha = 1.0f);

  // Explicit coordinate overload (Performance Optimization)
  void queueTileToCache(const Domain::Tile &tile, int tile_x, int tile_y,
                        int tile_z, float screen_x, float screen_y,
                        const AnimationTicks &anim_ticks,
                        std::vector<uint32_t> &missing_sprites,
                        std::vector<SpriteInstance> &output_sprites,
                        float alpha = 1.0f);

  /**
   * ID-based cache generation (new architecture).
   * Outputs TileInstance with sprite_id for GPU-side UV lookup.
   */
  void queueTileToTileCache(const Domain::Tile &tile, float screen_x,
                            float screen_y, const AnimationTicks &anim_ticks,
                            std::vector<uint32_t> &missing_sprites,
                            std::vector<TileInstance> &output_tiles,
                            float alpha = 1.0f);

  void queueTileToTileCache(const Domain::Tile &tile, int tile_x, int tile_y,
                            int tile_z, float screen_x, float screen_y,
                            const AnimationTicks &anim_ticks,
                            std::vector<uint32_t> &missing_sprites,
                            std::vector<TileInstance> &output_tiles,
                            float alpha = 1.0f);

  /**
   * Update view settings pointer.
   */
  void setViewSettings(Services::ViewSettings *settings) {
    view_settings_ = settings;
  }

  /**
   * Set creature simulator for animation state lookup.
   */
  void setCreatureSimulator(Services::CreatureSimulator *simulator) {
    creature_simulator_ = simulator;
  }

  /**
   * Set current zoom level.
   * Called by MapRenderer before rendering.
   */
  void setZoom(float zoom) { current_zoom_ = zoom; }

  /**
   * Set secondary client provider for cross-version fallback.
   * When primary client doesn't have an item, lookup in secondary.
   * Items from secondary render with red tint.
   */
  void setSecondaryClientProvider(Services::SecondaryClientProvider provider) {
    secondary_client_.setProvider(provider);
    item_renderer_.setSecondaryClientProvider(provider);
    ground_renderer_.setSecondaryClientProvider(provider);
  }

  /**
   * Set LOD active state.
   * If true, applies optimizations (e.g. simplified creature rendering).
   */
  void setLODMode(bool enabled) { is_lod_active_ = enabled; }

  /**
   * Set selection data provider for rendering highlights.
   * Uses interface to decouple from SelectionService.
   */
  void setSelectionProvider(const ISelectionDataProvider *provider);

private:
  bool is_lod_active_ = false;
  SpriteBatch &sprite_batch_;
  SpriteEmitter emitter_;
  Services::SpriteManager &sprite_manager_;
  Services::ClientDataService *client_data_;
  Services::ViewSettings *view_settings_;
  Services::SecondaryClientHandle secondary_client_;
  Services::CreatureSimulator *creature_simulator_ = nullptr;
  const ISelectionDataProvider *selection_provider_ = nullptr;

  // Sub-renderers (facade pattern)
  ItemRenderer item_renderer_;
  GroundRenderer ground_renderer_;
  CreatureRenderer creature_renderer_;
  SpawnTintPass spawn_overlay_renderer_;

  // Current zoom level (set via setZoom())
  float current_zoom_ = 1.0f;

  // Optimization: Cached selection bounds to skip map lookups
  int32_t sel_min_x_ = 0, sel_min_y_ = 0;
  int32_t sel_max_x_ = 0, sel_max_y_ = 0;
  int16_t sel_min_z_ = 0, sel_max_z_ = 0;
  bool has_selection_ = false;

  void updateSelectionBounds();

  void queueInvalidItemPlaceholder(float screen_x, float screen_y, float size,
                                   float alpha, float r = 0.9f, float g = 0.1f,
                                   float b = 0.1f);

  // Scratch buffer for avoiding allocations in hot path
  // Stores {Item*, ItemType*} to avoid redundant lookups in the OnTop pass
  std::vector<ItemRenderer::RenderItem> on_top_item_cache_;

  // Stores all items with resolved types for efficient rendering
  std::vector<ItemRenderer::RenderItem> render_cache_;
};

} // namespace Rendering
} // namespace MapEditor
