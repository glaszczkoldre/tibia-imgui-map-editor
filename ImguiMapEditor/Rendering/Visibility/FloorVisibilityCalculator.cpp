#include "Rendering/Visibility/FloorVisibilityCalculator.h"
#include <algorithm>
#include <cmath>

namespace MapEditor {
namespace Rendering {

FloorVisibilityCalculator::FloorVisibilityCalculator(Services::ClientDataService* client_data)
    : client_data_(client_data)
{
}

const Domain::ItemType* FloorVisibilityCalculator::getItemType(const Domain::Item* item) const {
    if (!item || !client_data_) return nullptr;
    if (const Domain::ItemType* item_type = item->getType()) {
        return item_type;
    }
    return client_data_->getItemTypeByServerId(item->getServerId());
}

bool FloorVisibilityCalculator::tileLimitsFloorsView(const Domain::Tile* tile, bool is_free_view) const {
    if (!tile) return false;

    const Domain::Item* first_thing = tile->getGround();
    if (!first_thing && !tile->getItems().empty()) {
        first_thing = tile->getItems().front().get();
    }

    const Domain::ItemType* item_type = getItemType(first_thing);
    if (!item_type) return false;

    if (item_type->hasFlag(Domain::ItemFlag::IgnoreLook)) {
        return false;
    }

    const bool is_ground_tile = item_type->isGround() || item_type->is_ground;
    const bool is_bottom = item_type->always_on_bottom || item_type->is_on_bottom;
    const bool blocks_projectile =
        item_type->blocks_projectile ||
        item_type->hasFlag(Domain::ItemFlag::BlockMissiles);

    if (is_free_view) {
        return is_ground_tile || is_bottom;
    }

    return is_ground_tile || (is_bottom && blocks_projectile);
}

bool FloorVisibilityCalculator::isLookPossible(const Domain::Tile* tile) const {
    if (!tile) return false;
    
    // Check all items for projectile blocking
    const Domain::Item* ground = tile->getGround();
    if (ground) {
        const Domain::ItemType* ground_type = getItemType(ground);
        if (ground_type &&
            (ground_type->blocks_projectile ||
             ground_type->hasFlag(Domain::ItemFlag::BlockMissiles))) {
            return false;
        }
    }
    
    for (const auto& item : tile->getItems()) {
        const Domain::ItemType* item_type = getItemType(item.get());
        if (item_type &&
            (item_type->blocks_projectile ||
             item_type->hasFlag(Domain::ItemFlag::BlockMissiles))) {
            return false;
        }
    }
    
    return true;
}

int FloorVisibilityCalculator::calcFirstVisibleFloor(
    const Domain::ChunkedMap& map,
    int camera_x, int camera_y, int camera_z) const
{
    using FC = FloorConstants;
    
    int first_floor = 0;
    
    // Underground: limit view to underground floors only
    if (camera_z > FC::SEA_FLOOR) {
        first_floor = std::max(camera_z - FC::AWARE_UNDERGROUND_FLOOR_RANGE, 
                               static_cast<int>(FC::UNDERGROUND_FLOOR));
    }
    
    for (int ix = -1; ix <= 1 && first_floor < camera_z; ++ix) {
        for (int iy = -1; iy <= 1 && first_floor < camera_z; ++iy) {
            const int pos_x = camera_x + ix;
            const int pos_y = camera_y + iy;
            const bool is_center = ix == 0 && iy == 0;
            const bool is_straight_neighbor = std::abs(ix) != std::abs(iy);
            const Domain::Tile* position_tile = map.getTile(pos_x, pos_y, camera_z);
            const bool look_possible = isLookPossible(position_tile);

            if (!is_center && (!is_straight_neighbor || !look_possible)) {
                continue;
            }

            int upper_x = pos_x;
            int upper_y = pos_y;
            int upper_z = camera_z;
            int covered_x = pos_x;
            int covered_y = pos_y;
            int covered_z = camera_z;

            while (upper_z > 0 && covered_z > 0) {
                --upper_z;
                --covered_z;
                ++covered_x;
                ++covered_y;
                if (upper_z < first_floor) {
                    break;
                }

                if (const Domain::Tile* upper_tile = map.getTile(upper_x, upper_y, upper_z);
                    tileLimitsFloorsView(upper_tile, !look_possible)) {
                    first_floor = upper_z + 1;
                    break;
                }

                if (const Domain::Tile* covered_tile = map.getTile(covered_x, covered_y, covered_z);
                    tileLimitsFloorsView(covered_tile, look_possible)) {
                    first_floor = covered_z + 1;
                    break;
                }
            }
        }
    }
    
    return std::clamp(first_floor, 0, static_cast<int>(FC::MAX_Z));
}

int FloorVisibilityCalculator::calcLastVisibleFloor(int camera_z) const {
    using FC = FloorConstants;
    
    int z;
    
    if (camera_z > FC::SEA_FLOOR) {
        // Underground: see current floor + range below
        z = camera_z + FC::AWARE_UNDERGROUND_FLOOR_RANGE;
    } else {
        // Surface: see down to sea floor
        z = FC::SEA_FLOOR;
    }
    
    return std::clamp(z, 0, static_cast<int>(FC::MAX_Z));
}

} // namespace Rendering
} // namespace MapEditor
