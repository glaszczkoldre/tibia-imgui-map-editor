#pragma once
#include "Domain/ChunkedMap.h"
#include "Services/ClientDataService.h"
#include <functional>
#include <cstddef>

namespace MapEditor {
namespace Services {

/**
 * Result of a cleanup operation.
 * Provides statistics about what was removed.
 */
struct CleanupResult {
    size_t items_removed = 0;
    size_t tiles_removed = 0;
    size_t tiles_processed = 0;
    size_t total_tiles = 0;
};

/**
 * Progress callback for long-running operations.
 * @param progress Value from 0.0 to 1.0 indicating completion percentage.
 */
using ProgressCallback = std::function<void(float progress)>;

/**
 * Service for map cleanup operations.
 * 
 * Each operation is independent and follows single-responsibility principle.
 * All operations are NON-UNDOABLE - they directly modify the map.
 * 
 * Usage:
 *   CleanupResult result = MapCleanupService::cleanInvalidItems(map, clientData);
 */
class MapCleanupService {
public:
    /**
     * Remove items whose ItemType ID doesn't exist in client data.
     * 
     * @param map The map to clean
     * @param client_data Client data service to check item validity
     * @param on_progress Optional callback for progress updates
     * @return Statistics about removed items
     */
    static CleanupResult cleanInvalidItems(
        Domain::ChunkedMap& map,
        const ClientDataService& client_data,
        ProgressCallback on_progress = nullptr
    );
    
    /**
     * Remove moveable items from tiles that belong to houses.
     * 
     * @param map The map to clean
     * @param client_data Client data service to check if items are moveable
     * @param on_progress Optional callback for progress updates
     * @return Statistics about removed items
     */
    static CleanupResult cleanHouseItems(
        Domain::ChunkedMap& map,
        const ClientDataService& client_data,
        ProgressCallback on_progress = nullptr
    );
    
    /**
     * Remove all items with a specific ID from the map.
     * 
     * @param map The map to clean
     * @param item_id The item ID to remove
     * @param on_progress Optional callback for progress updates
     * @return Statistics about removed items
     */
    static CleanupResult removeItemsById(
        Domain::ChunkedMap& map,
        uint16_t item_id,
        ProgressCallback on_progress = nullptr
    );
};

} // namespace Services
} // namespace MapEditor
