#pragma once
#include "IMinimapDataSource.h"
namespace MapEditor {

// Forward declarations
namespace Domain {
    class ChunkedMap;
}

namespace Services {
    class ClientDataService;
}

namespace Rendering {

/**
 * Minimap data source backed by ChunkedMap.
 * Provides tile colors by looking up ItemType.minimap_color.
 */
class ChunkedMapMinimapSource : public IMinimapDataSource {
public:
    /**
     * Create data source
     * @param map Chunked map (not owned)
     * @param clientData Client data service for ItemType lookup (not owned)
     */
    ChunkedMapMinimapSource(const Domain::ChunkedMap* map, 
                            const Services::ClientDataService* clientData);
    
    ~ChunkedMapMinimapSource() override = default;
    
    // IMinimapDataSource interface
    uint8_t getTileColor(int32_t x, int32_t y, int16_t z) const override;
    MinimapBounds getMapBounds() const override;
    bool hasTile(int32_t x, int32_t y, int16_t z) const override;

private:
    void computeBounds();
    
    const Domain::ChunkedMap* map_;
    const Services::ClientDataService* client_data_;
    
    // Cached bounds computed once at construction
    MinimapBounds cached_bounds_;
    bool bounds_valid_ = false;
};

} // namespace Rendering
} // namespace MapEditor
