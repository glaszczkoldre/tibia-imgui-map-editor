#pragma once
#include "../../Core/Config.h"
#include "../ColorFilter.h"
#include "../Overlays/OverlayCollector.h"
#include "Domain/Item.h"
#include "Domain/ItemType.h"
#include "Rendering/Animation/AnimationTicks.h"
#include "Rendering/Backend/TileInstance.h"
#include "Rendering/Tile/TileColor.h"
#include "Rendering/Utils/SpriteEmitter.h"
#include "Services/ClientDataService.h"
#include "Services/SecondaryClientConstants.h"
#include "Services/SecondaryClientData.h"
#include "Services/ViewSettings.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <span>

namespace MapEditor {

namespace Services {
class SpriteManager;
class ClientDataService;
class SecondaryClientData;
} // namespace Services

namespace Rendering {

/**
 * Handles item sprite rendering with stacking, patterns, and animation.
 *
 * Extracted from TileRenderer to separate item-specific logic.
 * Handles both ground items (when delegated) and stacked items.
 */
class ItemRenderer {
public:
  static constexpr float TILE_SIZE = 32.0f;

  struct RenderItem {
    const Domain::Item *item;
    const Domain::ItemType *type;
  };

  // NOTE: client_data is const - read-only access for item type lookup
  ItemRenderer(SpriteEmitter &emitter, Services::SpriteManager &sprites,
               const Services::ClientDataService *client_data);

  /**
   * Set secondary client provider for cross-version item lookup.
   */
  void setSecondaryClientProvider(Services::SecondaryClientProvider provider) {
    secondary_client_.setProvider(std::move(provider));
  }

  /**
   * Queue all items from a tile for rendering using pre-resolved types.
   *
   * @param items Span of items with resolved types
   * @param screen_x/y Screen position
   * @param size Tile size (TILE_SIZE * zoom)
   * @param tile_x/y/z Tile world coordinates
   * @param anim_ticks Pre-calculated animation ticks
   * @param ground_color Ground color for border inheritance
   * @param alpha Base alpha
   * @param isSelected Lambda to check if item is selected
   * @param view_settings View settings for locked door highlighting
   * @param missing_sprites Output: sprites not yet loaded
   * @param accumulated_elevation In/Out: elevation for item stacking
   */
  template <typename SelectFunc>
  void
  queueAll(std::span<const RenderItem> items,
           float screen_x, float screen_y, float size, int tile_x, int tile_y,
           int tile_z, const AnimationTicks &anim_ticks,
           const TileColor &ground_color, float alpha, SelectFunc &&isSelected,
           const Services::ViewSettings *view_settings,
           std::vector<uint32_t> &missing_sprites, float *accumulated_elevation,
           bool tile_has_hook_south = false, bool tile_has_hook_east = false,
           bool check_tooltips = false, bool *out_has_tooltip = nullptr);

  /**
   * Queue a single item with explicit color.
   * Core rendering logic for items with patterns, animation, stacking.
   */
  void queueWithColor(const Domain::ItemType *item_type, float screen_x,
                      float screen_y, float size, int tile_x, int tile_y,
                      int tile_z, const AnimationTicks &anim_ticks,
                      std::vector<uint32_t> &missing_sprites, float r, float g,
                      float b, float alpha,
                      float *accumulated_elevation = nullptr,
                      const Domain::Item *item_inst = nullptr,
                      uint32_t sprite_id_offset = 0,
                      bool tile_has_hook_south = false,
                      bool tile_has_hook_east = false);

  /**
   * Queue an invalid item placeholder (colored square).
   */
  void queueInvalidPlaceholder(float screen_x, float screen_y, float size,
                               float alpha, float r, float g, float b);

private:
  SpriteEmitter &emitter_;
  Services::SpriteManager &sprite_manager_;
  const Services::ClientDataService *client_data_; // Non-owning, read-only
  Services::SecondaryClientHandle secondary_client_;
};

// Template implementation must be in header
template <typename SelectFunc>
void ItemRenderer::queueAll(
    std::span<const RenderItem> items, float screen_x,
    float screen_y, float size, int tile_x, int tile_y, int tile_z,
    const AnimationTicks &anim_ticks, const TileColor &ground_color,
    float alpha, SelectFunc &&isSelected,
    const Services::ViewSettings *view_settings,
    std::vector<uint32_t> &missing_sprites, float *accumulated_elevation,
    bool tile_has_hook_south, bool tile_has_hook_east,
    bool check_tooltips, bool *out_has_tooltip) {
  bool show_invalid = view_settings && view_settings->show_invalid_items;
  bool highlight_locked_doors =
      view_settings && view_settings->highlight_locked_doors;

  // Helper lambda to render a single item (DRY - avoids code duplication)
  // OPTIMIZATION: Accept resolved type to avoid redundant lookup
  auto renderItem = [&](const RenderItem &entry) {
    const auto *item = entry.item;
    const auto *item_type = entry.type;
    // Note: item_type is already resolved by caller

    // Secondary client fallback
    bool is_from_secondary = false;
    auto *sec_client = secondary_client_.get();
    if (!item_type && show_invalid && sec_client && sec_client->isActive()) {
      item_type = sec_client->getItemTypeByServerId(item->getServerId());
      is_from_secondary = (item_type != nullptr);
    }

    // Integrated Tooltip Check (Bolt Optimization)
    if (check_tooltips && out_has_tooltip && !(*out_has_tooltip)) {
        if (OverlayCollector::needsTooltip(item)) {
            *out_has_tooltip = true;
        }
    }

    // Item is invalid if:
    // 1. item_type is null (ID not in items.otb at all), OR
    // 2. item_type exists but has no client_id (gap entry in items.otb)
    bool is_invalid =
        (item_type == nullptr) || !item_type->isValidForRendering();

    if (item_type && !item_type->sprite_ids.empty()) {
      // Calculate item color (borders inherit ground color)
      bool is_border = item_type->is_border;
      TileColor item_color =
          ColorFilter::calculateItemColor(item_type, ground_color, is_border);

      // Locked door highlighting
      if (highlight_locked_doors && item_type->isDoor() &&
          item_type->is_locked) {
        item_color.g *= 0.5f;
        item_color.b *= 0.5f;
      }

      // Selection tinting
      if (isSelected(item)) {
        item_color.r *= 0.5f;
        item_color.g *= 0.5f;
        item_color.b *= 0.5f;
      }

      float use_alpha = alpha;
      uint32_t sprite_offset = 0;

      // Secondary client tinting (yellow for stacked items)
      if (is_from_secondary && sec_client) {
        float tint = sec_client->getTintIntensity();
        item_color.b *= (1.0f - tint);
        use_alpha *= sec_client->getAlphaMultiplier();
        sprite_offset = Services::SECONDARY_SPRITE_OFFSET;
      }

      queueWithColor(item_type, screen_x, screen_y, size, tile_x, tile_y,
                     tile_z, anim_ticks, missing_sprites, item_color.r,
                     item_color.g, item_color.b, use_alpha,
                     accumulated_elevation, item, sprite_offset,
                     tile_has_hook_south, tile_has_hook_east);
    } else if (is_invalid && show_invalid) {
      queueInvalidPlaceholder(
          screen_x, screen_y, size, alpha, Config::Colors::INVALID_STACKED_R,
          Config::Colors::INVALID_STACKED_G, Config::Colors::INVALID_STACKED_B);
    }
  };

  // ============================================================
  // MULTI-PASS RENDERING (OTClient Painter Algorithm)
  // ============================================================
  // Pass 1: OnBottom items (walls, pillars) - FORWARD order
  // Pass 2: Common items (furniture, decorations) - REVERSE order
  // NOTE: OnTop items (P3) are rendered immediately per-tile in
  //       TileRenderer::queueTile() AFTER creatures for correct depth.
  // ============================================================

  // PASS 1: OnBottom items (priority 2) - FORWARD
  for (const auto &entry : items) {
    if (entry.type && entry.type->is_on_bottom) {
      renderItem(entry);
    }
  }

  // PASS 2: Common items (priority 5) - FORWARD (like RME)
  // Items at the end of the vector (topmost) must be drawn LAST to appear on top
  for (const auto &entry : items) {
    if (entry.type && !entry.type->is_on_bottom && !entry.type->is_on_top) {
      renderItem(entry);
    } else if (!entry.type) {
      // Items without type info are treated as common items
      renderItem(entry);
    }
  }

  // NOTE: OnTop items rendered per-tile in TileRenderer::queueTile()
}

} // namespace Rendering
} // namespace MapEditor
