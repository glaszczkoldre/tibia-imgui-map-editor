#pragma once

#include <string>
#include <cstdint>
#include "Domain/Outfit.h"
namespace MapEditor {

namespace IO { struct ClientItem; }

namespace Services {
    class ClientDataService;
    class SpriteManager;
}

namespace Rendering {
    class Texture;
}

namespace Domain {
    struct CreatureType;
}

namespace Rendering {

/**
 * Central helper for creature sprite/texture resolution.
 * 
 * PURPOSE: Eliminates duplicate code across 4+ UI components that all need
 * to render creature thumbnails. Provides a single API for:
 * 
 * 1. UI THUMBNAILS (ImGui):
 *    - getThumbnail(creature_name) - Get texture by creature name
 *    - getThumbnail(outfit) - Get texture by outfit data
 *    - getRecommendedSize() - Get size based on outfit dimensions
 * 
 * 2. GPU RENDERING (TileRenderer - future):
 *    - resolveSpriteId() - Get sprite IDs for GPU batch rendering
 * 
 * CONSUMERS:
 *    - BrowseTileWindow.cpp
 *    - SearchResultsWidget.cpp
 *    - AdvancedSearchDialog.cpp
 *    - TilesetWidget.cpp
 *    - TileRenderer.cpp (future)
 */
class CreatureSpriteHelper {
public:
    CreatureSpriteHelper(Services::ClientDataService* client_data,
                         Services::SpriteManager* sprite_manager);
    
    // ========== FOR UI THUMBNAILS ==========
    
    /**
     * Get a thumbnail texture for ImGui rendering by creature name.
     * Looks up creature type, resolves outfit, and gets composited texture.
     * 
     * @param creature_name Name of the creature (e.g., "Demon", "Rat")
     * @return Texture pointer for ImGui::Image, or nullptr if not found
     */
    Texture* getThumbnail(const std::string& creature_name);
    
    /**
     * Get a thumbnail texture for ImGui rendering by outfit.
     * Use when you already have the outfit data (e.g., from CreatureBrush).
     * 
     * @param outfit Outfit data with lookType and colors
     * @return Texture pointer for ImGui::Image, or nullptr if not found
     */
    Texture* getThumbnail(const Domain::Outfit& outfit);
    
    /**
     * Get recommended thumbnail size based on creature dimensions.
     * Returns max(width, height) * 32 for proper aspect ratio.
     * 
     * @param creature_name Name of the creature
     * @return Recommended size in pixels, or 32.0f if not found
     */
    float getRecommendedSize(const std::string& creature_name) const;
    
    /**
     * Get recommended thumbnail size for an outfit.
     */
    float getRecommendedSize(const Domain::Outfit& outfit) const;
    
    // ========== FOR GPU RENDERING (TileRenderer) ==========
    
    struct SpriteResult {
        uint32_t sprite_id = 0;           // Primary sprite ID
        bool needs_colorization = false;  // True if outfit needs template coloring
        uint32_t template_sprite_id = 0;  // Template mask sprite ID (if needs_colorization)
        uint8_t head = 0, body = 0, legs = 0, feet = 0;  // Outfit colors
        uint8_t width = 1, height = 1;    // Creature dimensions in tiles
    };
    
    /**
     * Resolve outfit to sprite ID(s) for GPU batch rendering.
     * Use in TileRenderer::queueCreatureSprite().
     * 
     * @param outfit Outfit data
     * @param direction Direction (0=N, 1=E, 2=S, 3=W)
     * @param animation_frame Current animation frame
     * @return SpriteResult with IDs and color data
     */
    SpriteResult resolveSpriteId(const Domain::Outfit& outfit,
                                  uint8_t direction = 2,
                                  int animation_frame = 0);
    
    /**
     * Check if helper is properly initialized.
     */
    bool isValid() const { return client_data_ && sprite_manager_; }
    
private:
    Services::ClientDataService* client_data_ = nullptr;
    Services::SpriteManager* sprite_manager_ = nullptr;
    
    // Internal helper to get outfit data for a lookType
    const IO::ClientItem* getOutfitData(uint16_t look_type) const;
};

} // namespace Rendering
} // namespace MapEditor
