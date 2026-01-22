#pragma once
#include "Domain/Item.h"
#include "Domain/ItemType.h"
#include "Services/SecondaryClientConstants.h"
#include "Services/ViewSettings.h"
#include "Rendering/Animation/AnimationTicks.h"
#include "Rendering/Backend/SpriteBatch.h"
#include "Rendering/Tile/TileColor.h"
#include <cstdint>
#include <functional>
#include <vector>

namespace MapEditor {

namespace Services {
class SpriteManager;
class ClientDataService;
class SecondaryClientData;
} // namespace Services

namespace Rendering {

class ItemRenderer; // Forward declaration

/**
 * Handles ground sprite rendering.
 *
 * Extracted from TileRenderer to separate ground-specific logic.
 * Ground is always rendered first and uses special tinting rules.
 * Delegates actual sprite queueing to ItemRenderer.
 */
class GroundRenderer {
public:
  static constexpr float TILE_SIZE = 32.0f;

  GroundRenderer(ItemRenderer &item_renderer,
                 Services::ClientDataService *client_data);

  /**
   * Set secondary client provider for cross-version item lookup.
   */
  void setSecondaryClientProvider(Services::SecondaryClientProvider provider) {
    secondary_client_.setProvider(std::move(provider));
  }

  /**
   * Queue ground sprite for rendering.
   *
   * @param ground The ground item to render
   * @param screen_x Screen X position
   * @param screen_y Screen Y position
   * @param size Tile size (TILE_SIZE * zoom)
   * @param tile_x/y/z Tile world coordinates (for pattern calculation)
   * @param anim_ticks Pre-calculated animation ticks
   * @param color Ground tint color (calculated by ColorFilter)
   * @param alpha Override alpha (1.0 = normal)
   * @param is_selected Whether ground is selected
   * @param view_settings View settings for wall outline etc.
   * @param missing_sprites Output: sprites not yet loaded
   * @param accumulated_elevation In/Out: elevation for item stacking
   * @return true if ground was rendered, false if invalid/skipped
   */
  bool queue(const Domain::Item *ground, float screen_x, float screen_y,
             float size, int tile_x, int tile_y, int tile_z,
             const AnimationTicks &anim_ticks, const TileColor &color,
             float alpha, bool is_selected,
             const Services::ViewSettings *view_settings,
             std::vector<uint32_t> &missing_sprites,
             float *accumulated_elevation);

private:
  ItemRenderer &item_renderer_;
  Services::ClientDataService *client_data_;
  Services::SecondaryClientHandle secondary_client_;
};

} // namespace Rendering
} // namespace MapEditor
