#include "ChunkedMapMinimapSource.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Tile.h"
#include "Domain/Item.h"
#include "Domain/ItemType.h"
#include "Services/ClientDataService.h"
#include <limits>

namespace MapEditor {
namespace Rendering {

ChunkedMapMinimapSource::ChunkedMapMinimapSource(
    const Domain::ChunkedMap* map, 
    const Services::ClientDataService* clientData)
    : map_(map)
    , client_data_(clientData) {
    // Compute bounds once at construction
    computeBounds();
}

void ChunkedMapMinimapSource::computeBounds() {
    if (!map_) return;
    
    bounds_valid_ = false;
    cached_bounds_ = MinimapBounds();
    
    int32_t min_x = std::numeric_limits<int32_t>::max();
    int32_t min_y = std::numeric_limits<int32_t>::max();
    int32_t max_x = std::numeric_limits<int32_t>::min();
    int32_t max_y = std::numeric_limits<int32_t>::min();
    
    // Scan all tiles to find actual bounds
    bool found_any = false;
    map_->forEachTile([&](const Domain::Tile* tile) {
        if (tile) {
            const auto& pos = tile->getPosition();
            min_x = std::min(min_x, static_cast<int32_t>(pos.x));
            min_y = std::min(min_y, static_cast<int32_t>(pos.y));
            max_x = std::max(max_x, static_cast<int32_t>(pos.x));
            max_y = std::max(max_y, static_cast<int32_t>(pos.y));
            found_any = true;
        }
    });
    
    if (found_any) {
        cached_bounds_.min_x = min_x;
        cached_bounds_.min_y = min_y;
        cached_bounds_.max_x = max_x;
        cached_bounds_.max_y = max_y;
        bounds_valid_ = true;
    }
}

uint8_t ChunkedMapMinimapSource::getTileColor(int32_t x, int32_t y, int16_t z) const {
    if (!map_ || !client_data_) return 0;
    
    const Domain::Tile* tile = map_->getTile(x, y, z);
    if (!tile) return 0;
    
    // Check items top-to-bottom (reverse order)
    const auto& items = tile->getItems();
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        const Domain::Item* item = it->get();
        if (!item) continue;
        
        const Domain::ItemType* type = client_data_->getItemTypeByServerId(item->getServerId());
        if (type && type->minimap_color != 0) {
            return static_cast<uint8_t>(type->minimap_color);
        }
    }
    
    // Fall back to ground
    const Domain::Item* ground = tile->getGround();
    if (ground) {
        const Domain::ItemType* type = client_data_->getItemTypeByServerId(ground->getServerId());
        if (type && type->minimap_color != 0) {
            return static_cast<uint8_t>(type->minimap_color);
        }
    }
    
    return 0;
}

MinimapBounds ChunkedMapMinimapSource::getMapBounds() const {
    return cached_bounds_;
}

bool ChunkedMapMinimapSource::hasTile(int32_t x, int32_t y, int16_t z) const {
    if (!map_) return false;
    return map_->getTile(x, y, z) != nullptr;
}

} // namespace Rendering
} // namespace MapEditor
