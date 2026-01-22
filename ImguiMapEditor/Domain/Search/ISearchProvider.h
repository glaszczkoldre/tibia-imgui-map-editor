#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace MapEditor::Domain::Search {

/**
 * Result from a catalog search (items/creatures available to place)
 */
struct PickResult {
    uint16_t server_id = 0;
    std::string name;
    bool is_creature = false;
    
    // For sorting/comparison
    bool operator<(const PickResult& other) const {
        return name < other.name;
    }
};

/**
 * Interface for searching a catalog of placeable entities
 * Implementations: ItemSearchProvider, CreatureSearchProvider, BrushSearchProvider (future)
 */
class ISearchProvider {
public:
    virtual ~ISearchProvider() = default;
    
    /**
     * Search catalog by query string
     * @param query - search string (name, id, or prefix like "cid:")
     * @param limit - max results to return
     * @return vector of matching results
     */
    virtual std::vector<PickResult> search(const std::string& query, size_t limit = 50) const = 0;
    
    /**
     * Get display name for provider (e.g., "Items", "Creatures")
     */
    virtual std::string getProviderName() const = 0;
};

} // namespace MapEditor::Domain::Search
