#include "OverlayCollector.h"
#include "Domain/Tile.h"
#include "Domain/Item.h"
namespace MapEditor {
namespace Rendering {

bool OverlayCollector::needsTooltip(const Domain::Item* item) {
    if (!item) return false;
    return (item->getActionId() > 0 || item->getUniqueId() > 0 ||
            item->getDoorId() > 0 || !item->getText().empty() ||
            item->getTeleportDestination() != nullptr);
}

bool OverlayCollector::needsTooltip(const Domain::Tile* tile) {
    if (tile->hasSpawn()) return true;
    if (tile->hasGround() && needsTooltip(tile->getGround())) {
        return true;
    }
    return false;
}

void OverlayCollector::collectFromTile(const Domain::Tile& tile, 
                                        float screen_x, float screen_y, 
                                        bool show_tooltips) {
    OverlayEntry entry{&tile, {screen_x, screen_y}, nullptr};
    
    // 1. Check for Spawns
    if (tile.hasSpawn()) {
        spawns.push_back(entry);
    }
    
    // 2. Check for tooltip eligibility
    if (show_tooltips) {
        bool needs_tooltip = needsTooltip(&tile); // Check Spawn/Ground

        // Check Items Attributes
        if (!needs_tooltip) {
            const auto& items = tile.getItems();
            for (const auto& item_ptr : items) {
                if (needsTooltip(item_ptr.get())) {
                    needs_tooltip = true;
                    break;
                }
            }
        }

        if (needs_tooltip) {
            tooltips.push_back(entry);
        }
    }
}

} // namespace Rendering
} // namespace MapEditor
