#include "Rendering/Map/TileRenderer.h"
#include "Rendering/Backend/TileInstance.h"
#include "Rendering/ColorFilter.h"
#include "Rendering/Selection/ISelectionDataProvider.h"

#include "Rendering/Resources/TextureAtlas.h"
#include "Rendering/Visibility/LODPolicy.h"
#include "Services/ClientDataService.h"
#include "Services/CreatureSimulator.h"
#include "Services/SecondaryClientConstants.h"
#include "Services/SecondaryClientData.h"
#include "Services/SpriteManager.h"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace Rendering {

TileRenderer::TileRenderer(SpriteBatch &sprite_batch,
                           Services::SpriteManager &sprite_manager,
                           Services::ClientDataService *client_data,
                           Services::ViewSettings *view_settings)
    : sprite_batch_(sprite_batch), emitter_(sprite_batch),
      sprite_manager_(sprite_manager), client_data_(client_data),
      view_settings_(view_settings),
      item_renderer_(emitter_, sprite_manager, client_data),
      ground_renderer_(item_renderer_, client_data),
      creature_renderer_(emitter_, sprite_manager, client_data),
      spawn_overlay_renderer_(sprite_batch, sprite_manager) {}

void TileRenderer::queueTile(const Domain::Tile &tile, float screen_x,
                             float screen_y, float zoom,
                             const AnimationTicks &anim_ticks,
                             std::vector<uint32_t> &missing_sprites,
                             OverlayCollector *overlay_collector, float alpha) {
  // Delegate to explicit coordinate version (avoids code duplication)
  queueTile(tile, tile.getX(), tile.getY(), tile.getZ(), screen_x, screen_y,
            zoom, anim_ticks, missing_sprites, overlay_collector, alpha);
}

void TileRenderer::queueTile(const Domain::Tile &tile, int tile_x, int tile_y,
                             int tile_z, float screen_x, float screen_y,
                             float zoom, const AnimationTicks &anim_ticks,
                             std::vector<uint32_t> &missing_sprites,
                             OverlayCollector *overlay_collector, float alpha) {

  float size = TILE_SIZE * zoom;

  // Determine alpha values
  float ground_alpha = alpha;
  float item_alpha = alpha;

  // Ghost items setting only affects non-ground items
  if (alpha >= 1.0f && view_settings_ && view_settings_->ghost_items) {
    item_alpha = Config::Rendering::GHOST_ITEM_ALPHA;
  }

  // Calculate ground color using ColorFilter (RME-style tinting)
  TileColor ground_color(1.0f, 1.0f, 1.0f);
  if (view_settings_) {
    ground_color = ColorFilter::calculateGroundColor(tile, *view_settings_, 0);

    // Apply item highlight tinting (RME-style density visualization)
    if (view_settings_->highlight_items) {
      size_t item_count = tile.getItemCount();
      bool topmost_is_border = false;

      // Check if topmost item is a border (excludes from highlighting)
      if (item_count > 0) {
        const auto &items = tile.getItems();
        const auto *topmost = items.back().get();
        if (topmost && topmost->getType()) {
          topmost_is_border = topmost->getType()->is_border;
        }
      }

      ground_color = ColorFilter::applyItemHighlight(ground_color, item_count,
                                                     topmost_is_border);
    }

    // Apply spawn radius tinting is now done as an overlay after ground
    // rendering This ensures spawn areas are visible above ground but below
    // items
  }

  // Track accumulated elevation for stacking
  float accumulated_elevation = 0.0f;

  // Selection check using interface (Option B: clean separation)
  Domain::Position tile_pos{static_cast<int32_t>(tile_x),
                            static_cast<int32_t>(tile_y),
                            static_cast<int16_t>(tile_z)};

  // Optimization: Pre-calculate selection bounds check
  const bool is_tile_in_selection_bounds =
      selection_provider_ && has_selection_ &&
      (tile_x >= sel_min_x_ && tile_x <= sel_max_x_ && tile_y >= sel_min_y_ &&
       tile_y <= sel_max_y_ && tile_z >= sel_min_z_ && tile_z <= sel_max_z_);

  // Helper to check selection status via interface
  // Optimization: Bounds check is now hoisted out of the lambda
  auto isItemSelected = [&](const Domain::Item *item) -> bool {
    if (!is_tile_in_selection_bounds)
      return false;

    return selection_provider_->isItemSelected(tile_pos, item);
  };

  // SINGLE PASS ANALYSIS (Bolt Optimization)
  // We iterate items once to:
  // 1. Determine hooks (south/east)
  // 2. Identify OnTop items (to avoid re-scanning later)
  // 3. Check ground hooks first
  bool tile_has_hook_south = false;
  bool tile_has_hook_east = false;

  if (tile.hasGround()) {
    const auto *ground = tile.getGround();
    if (ground) {
      // Conciseness fix: Use C++17 if-init and logical OR
      if (const auto *type = ground->getType()) {
        tile_has_hook_south |= type->hook_south;
        tile_has_hook_east |= type->hook_east;
      }
    }

    // Queue ground first - delegate to GroundRenderer
    bool is_ground_selected = isItemSelected(ground);
    ground_renderer_.queue(ground, screen_x, screen_y, size, tile_x, tile_y,
                           tile_z, anim_ticks, ground_color, ground_alpha,
                           is_ground_selected, view_settings_, missing_sprites,
                           &accumulated_elevation);
  }

  // Clear scratch buffers
  on_top_item_cache_.clear();
  render_cache_.clear();

  // Optimization: Check for tooltip eligibility during item pass to avoid re-iteration
  bool check_tooltips = false;
  bool tile_needs_tooltip = false;

  // Only check tooltips if collector is present and enabled
  // and we are on the main floor (overlays usually main floor only)
  if (overlay_collector && view_settings_ &&
      tile.getZ() == view_settings_->current_floor) {

      // Spawns are always added to the collector for radius rendering,
      // regardless of tooltip visibility.
      if (tile.hasSpawn()) {
          overlay_collector->addSpawn(&tile, screen_x, screen_y);
      }

      // If tooltips are enabled, perform eligibility checks.
      if (view_settings_->show_tooltips) {
          check_tooltips = true; // Enable checks for stacked and OnTop items.

          // A spawn or a ground item with attributes can trigger a tooltip for the tile.
          if (OverlayCollector::needsTooltip(&tile)) {
              tile_needs_tooltip = true;
          }
      }
  }

  // Iterate all items once
  const auto &items = tile.getItems();
  render_cache_.reserve(items.size()); // Pre-allocate to avoid reallocations

  for (const auto &item : items) {
      const auto *type = item->getType();
      if (!type && client_data_) {
          type = client_data_->getItemTypeByServerId(item->getServerId());
      }

      // Store in render cache for ItemRenderer
      render_cache_.push_back({item.get(), type});

      if (type) {
        // Conciseness fix: logical OR assignment
        tile_has_hook_south |= type->hook_south;
        tile_has_hook_east |= type->hook_east;

        // Collect OnTop items for deferred rendering
        // Optimization: Cache the type with the item to avoid re-lookup
        if (type->is_on_top) {
          on_top_item_cache_.push_back({item.get(), type});
        }
      }
  }

  // Queue all items - delegate to ItemRenderer
  // Pass cached items with pre-resolved types
  item_renderer_.queueAll(render_cache_, screen_x, screen_y, size, tile_x,
                          tile_y, tile_z, anim_ticks, ground_color, item_alpha,
                          isItemSelected, view_settings_, missing_sprites,
                          &accumulated_elevation, tile_has_hook_south,
                          tile_has_hook_east, check_tooltips, &tile_needs_tooltip);

  // CREATURE RENDERING - Per-tile immediate (OTClient parity)
  // With diagonal tile iteration, creatures must be rendered PER-TILE to
  // maintain correct isometric depth. NW tiles (drawn first) should have
  // creatures that can be overlapped by SE tile items (drawn later). Deferring
  // per-floor would cause ALL creatures to appear on top of ALL items!
  if (tile.hasCreature() && view_settings_ && view_settings_->show_creatures) {
    const Domain::Creature *creature = tile.getCreature();
    if (creature) {
      // Default: use tile position and creature's stored direction
      uint8_t direction = static_cast<uint8_t>(creature->direction);
      int animation_frame = 0;

      // Check LOD Policy for animations
      bool animate = !is_lod_active_ || LODPolicy::ANIMATE_CREATURES;

      float creature_screen_x = screen_x;
      float creature_screen_y = screen_y;
      TileColor creature_ground_color =
          ground_color; // Lighting from spawn tile

      // If simulation is enabled, use simulated position
      bool simulation_enabled =
          creature_simulator_ && creature_simulator_->isEnabled();

      if (simulation_enabled && animate) {
        auto *state = creature_simulator_->getOrCreateState(
            creature, tile.getPosition(), nullptr);
        if (state) {
          direction = state->direction;
          animation_frame = state->animation_frame;

          // Calculate screen position from simulated current_pos + walk_offset
          creature_screen_x =
              state->current_pos.x * size + state->walk_offset_x * size;
          creature_screen_y =
              state->current_pos.y * size + state->walk_offset_y * size;
        }
      }

      // IMMEDIATE per-tile rendering (not deferred!)
      creature_renderer_.queue(creature, creature_screen_x, creature_screen_y,
                               size, tile_x, tile_y, tile_z, anim_ticks,
                               creature_ground_color, item_alpha, direction,
                               animation_frame, missing_sprites);
    }
  }

  // ONTOP ITEM RENDERING - Per-tile immediate (OTClient parity: Tile::drawTop)
  // Uses pre-collected list from the single-pass analysis
  for (const auto &[item, type] : on_top_item_cache_) {
    // We already know type->is_on_top is true from the collection pass,
    // and we have the resolved type cached.

    // IMMEDIATE per-tile rendering (not deferred!)
    item_renderer_.queueWithColor(
        type, screen_x, screen_y, size, tile_x, tile_y, tile_z, anim_ticks,
        missing_sprites, ground_color.r, ground_color.g, ground_color.b,
        item_alpha, nullptr, item, 0, tile_has_hook_south, tile_has_hook_east);

    // Check tooltip for OnTop items if needed (not covered by queueAll)
    if (check_tooltips && !tile_needs_tooltip) {
      if (OverlayCollector::needsTooltip(item)) {
        tile_needs_tooltip = true;
      }
    }
  }

  // Finalize Tooltip Collection
  if (tile_needs_tooltip && overlay_collector) {
      overlay_collector->addTooltip(&tile, screen_x, screen_y);
  }
}

void TileRenderer::queueItemWithColor(
    const Domain::ItemType *item_type, float screen_x, float screen_y,
    float size, int tile_x, int tile_y, int tile_z,
    const AnimationTicks &anim_ticks, std::vector<uint32_t> &missing_sprites,
    float r, float g, float b, float alpha, float *accumulated_elevation,
    const Domain::Item *item_inst, uint32_t sprite_id_offset) {
  // Delegate to ItemRenderer
  item_renderer_.queueWithColor(item_type, screen_x, screen_y, size, tile_x,
                                tile_y, tile_z, anim_ticks, missing_sprites, r,
                                g, b, alpha, accumulated_elevation, item_inst,
                                sprite_id_offset);
}

void TileRenderer::queueItem(const Domain::ItemType *item_type, float screen_x,
                             float screen_y, float size, int tile_x, int tile_y,
                             int tile_z, const AnimationTicks &anim_ticks,
                             std::vector<uint32_t> &missing_sprites,
                             float alpha, float *accumulated_elevation,
                             const Domain::Item *item_inst) {
  // Forward to color version with white (1, 1, 1)
  queueItemWithColor(item_type, screen_x, screen_y, size, tile_x, tile_y,
                     tile_z, anim_ticks, missing_sprites, 1.0f, 1.0f, 1.0f,
                     alpha, accumulated_elevation, item_inst);
}

void TileRenderer::queueInvalidItemPlaceholder(float screen_x, float screen_y,
                                               float size, float alpha, float r,
                                               float g, float b) {
  // Use the white pixel from the atlas (same layer as regular sprites)
  // This ensures correct Z-order since all sprites are in the same batch
  const auto *region = sprite_manager_.getAtlasManager().getWhitePixel();
  if (!region) {
    return; // Failed to get white pixel
  }

  // Tint the white pixel with the given color
  emitter_.emit(std::round(screen_x), std::round(screen_y), size, size, *region,
                r, g, b, alpha * 0.7f); // 70% opacity for better visibility
}

void TileRenderer::queueTileToCache(const Domain::Tile &tile, float screen_x,
                                    float screen_y,
                                    const AnimationTicks &anim_ticks,
                                    std::vector<uint32_t> &missing_sprites,
                                    std::vector<SpriteInstance> &output_sprites,
                                    float alpha) {
  // Delegate to explicit coordinate version
  queueTileToCache(tile, tile.getX(), tile.getY(), tile.getZ(), screen_x,
                   screen_y, anim_ticks, missing_sprites, output_sprites,
                   alpha);
}

void TileRenderer::queueTileToCache(const Domain::Tile &tile, int tile_x,
                                    int tile_y, int tile_z, float screen_x,
                                    float screen_y,
                                    const AnimationTicks &anim_ticks,
                                    std::vector<uint32_t> &missing_sprites,
                                    std::vector<SpriteInstance> &output_sprites,
                                    float alpha) {
  // Set output redirection for this renderer and sub-renderers
  emitter_.setCache(&output_sprites);

  // Always use full rendering (caching replaces LOD)
  queueTile(tile, tile_x, tile_y, tile_z, screen_x, screen_y, 1.0f, anim_ticks,
            missing_sprites, nullptr, alpha);

  // Clear redirect
  emitter_.setCache(nullptr);
}

void TileRenderer::queueTileToTileCache(const Domain::Tile &tile,
                                        float screen_x, float screen_y,
                                        const AnimationTicks &anim_ticks,
                                        std::vector<uint32_t> &missing_sprites,
                                        std::vector<TileInstance> &output_tiles,
                                        float alpha) {
  // Delegate to explicit coordinate version
  queueTileToTileCache(tile, tile.getX(), tile.getY(), tile.getZ(), screen_x,
                       screen_y, anim_ticks, missing_sprites, output_tiles,
                       alpha);
}

void TileRenderer::queueTileToTileCache(const Domain::Tile &tile, int tile_x,
                                        int tile_y, int tile_z, float screen_x,
                                        float screen_y,
                                        const AnimationTicks &anim_ticks,
                                        std::vector<uint32_t> &missing_sprites,
                                        std::vector<TileInstance> &output_tiles,
                                        float alpha) {
  // Set output redirection to TileInstance vectors
  emitter_.setTileCache(&output_tiles);

  // Always use full rendering (no LOD - caching replaces LOD)
  queueTile(tile, tile_x, tile_y, tile_z, screen_x, screen_y, 1.0f, anim_ticks,
            missing_sprites, nullptr, alpha);

  // Clear redirect
  emitter_.setTileCache(nullptr);
}

void TileRenderer::setSelectionProvider(
    const ISelectionDataProvider *provider) {
  selection_provider_ = provider;
  updateSelectionBounds();
}

void TileRenderer::updateSelectionBounds() {
  if (!selection_provider_ || selection_provider_->isEmpty()) {
    has_selection_ = false;
    return;
  }

  has_selection_ = selection_provider_->getSelectionBounds(
      sel_min_x_, sel_min_y_, sel_min_z_, sel_max_x_, sel_max_y_, sel_max_z_);
}

} // namespace Rendering
} // namespace MapEditor
