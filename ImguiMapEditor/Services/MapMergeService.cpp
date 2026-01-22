#include "MapMergeService.h"
#include "Application/EditorSession.h"
#include "Domain/Tile.h"
#include <spdlog/spdlog.h>

namespace MapEditor::AppLogic {

MapMergeService::MergeResult MapMergeService::merge(
    EditorSession& target,
    const Domain::ChunkedMap& source,
    const MergeOptions& options
) {
    MergeResult result;
    
    auto* target_map = target.getMap();
    if (!target_map) {
        result.error = "Target map is null";
        return result;
    }
    
    spdlog::info("Starting map merge with offset ({}, {}, {}), overwrite={}",
                 options.offset.x, options.offset.y, options.offset.z,
                 options.overwrite_existing);
    
    // Iterate source map tiles (forEachTile is const, callback receives const Tile*)
    source.forEachTile([&](const Domain::Tile* source_tile) {
        if (!source_tile) return;
        
        const auto& source_pos = source_tile->getPosition();
        
        // Calculate target position with offset
        Domain::Position target_pos{
            source_pos.x + options.offset.x,
            source_pos.y + options.offset.y,
            static_cast<int16_t>(source_pos.z + options.offset.z)
        };
        
        // Skip if target position is out of bounds (floor 0-15)
        if (target_pos.z < 0 || target_pos.z > 15) {
            result.tiles_skipped++;
            return;
        }
        
        Domain::Tile* existing_tile = target_map->getTile(target_pos);
        
        if (options.overwrite_existing) {
            // Create new tile as copy of source tile at target position
            auto new_tile = source_tile->clone();
            new_tile->setPosition(target_pos);
            target_map->setTile(target_pos, std::move(new_tile));
            result.tiles_merged++;
        } else {
            // Merge: add items from source to existing tile
            if (!existing_tile) {
                // No existing tile - just copy
                auto new_tile = source_tile->clone();
                new_tile->setPosition(target_pos);
                target_map->setTile(target_pos, std::move(new_tile));
                result.tiles_merged++;
            } else {
                // Merge items from source into existing tile
                // Copy ground if target has none
                if (!existing_tile->getGround() && source_tile->getGround()) {
                    existing_tile->setGround(
                        std::make_unique<Domain::Item>(*source_tile->getGround()));
                }
                
                // Add all non-ground items from source
                for (const auto& item : source_tile->getItems()) {
                    existing_tile->addItem(std::make_unique<Domain::Item>(*item));
                }
                result.tiles_merged++;
            }
        }
    });
    
    // Mark session as modified
    target.setModified(true);
    
    spdlog::info("Map merge complete: {} tiles merged, {} skipped",
                 result.tiles_merged, result.tiles_skipped);
    
    result.success = true;
    return result;
}

} // namespace MapEditor::AppLogic
