#pragma once
#include "Rendering/Utils/SpriteEmitter.h"
#include "Rendering/Animation/AnimationTicks.h"
#include "Rendering/Tile/TileColor.h"
#include "Domain/Creature.h"
#include "Domain/Outfit.h"
#include "Services/ViewSettings.h"
#include "Services/ClientDataService.h"
#include "Services/SpriteManager.h"
#include "../../Core/Config.h"
#include <vector>
#include <cstdint>

namespace MapEditor {

namespace Services {
    class SpriteManager;
    class ClientDataService;
    class CreatureSimulator;
}

namespace Rendering {

/**
 * Handles creature sprite rendering via GPU batch.
 * 
 * Renders creatures as part of the tile pipeline for proper Z-ordering
 * and lighting integration.
 */
class CreatureRenderer {
public:
    static constexpr float TILE_SIZE = 32.0f;
    
    CreatureRenderer(SpriteEmitter& emitter,
                     Services::SpriteManager& sprites,
                     const Services::ClientDataService* client_data);
    
    /**
     * Queue a creature for GPU batch rendering.
     * 
     * @param creature Creature to render
     * @param screen_x Screen X position
     * @param screen_y Screen Y position
     * @param size Tile size (TILE_SIZE * zoom)
     * @param tile_x/y/z Tile world coordinates
     * @param anim_ticks Pre-calculated animation ticks
     * @param ground_color Ground color for lighting
     * @param alpha Base alpha
     * @param direction Creature facing direction
     * @param animation_frame Current animation frame
     * @param missing_sprites Output: sprites not yet loaded
     */
    void queue(const Domain::Creature* creature,
               float screen_x, float screen_y, float size,
               int tile_x, int tile_y, int tile_z,
               const AnimationTicks& anim_ticks,
               const TileColor& ground_color,
               float alpha,
               uint8_t direction,
               int animation_frame,
               std::vector<uint32_t>& missing_sprites);

private:
    SpriteEmitter& emitter_;
    Services::SpriteManager& sprite_manager_;
    const Services::ClientDataService* client_data_;
};

} // namespace Rendering
} // namespace MapEditor
