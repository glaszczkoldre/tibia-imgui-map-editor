#pragma once
#include "Tile.h"
#include "Position.h"
#include <vector>
#include <memory>

namespace MapEditor::Domain {

/**
 * Stores copied tiles for clipboard operations.
 * Tiles stored with relative positions from copy origin.
 * Pure data structure - no dependencies on Application/Presentation.
 */
class CopyBuffer {
public:
    struct CopiedTile {
        Position relative_pos;  // Offset from copy origin
        std::unique_ptr<Tile> tile;
        
        CopiedTile() = default;
        CopiedTile(const Position& pos, std::unique_ptr<Tile> t)
            : relative_pos(pos), tile(std::move(t)) {}
        
        // Move only
        CopiedTile(CopiedTile&&) = default;
        CopiedTile& operator=(CopiedTile&&) = default;
    };
    
    void setTiles(std::vector<CopiedTile> tiles);
    const std::vector<CopiedTile>& getTiles() const { return tiles_; }
    
    void clear();
    bool empty() const { return tiles_.empty(); }
    size_t size() const { return tiles_.size(); }
    
    // Bounds of copied region (for preview rendering)
    int32_t getWidth() const;
    int32_t getHeight() const;
    Position getMinBound() const;
    Position getMaxBound() const;
    
private:
    std::vector<CopiedTile> tiles_;
};

} // namespace MapEditor::Domain
