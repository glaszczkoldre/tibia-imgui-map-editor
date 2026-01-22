#pragma once
#include "../Position.h"
#include "../Tile.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace MapEditor::Domain::History {

/**
 * Serialized snapshot of a tile's complete state.
 * Used for undo/redo - stores tile state before and after changes.
 */
class TileSnapshot {
public:
    TileSnapshot() = default;
    
    /**
     * Capture current tile state into snapshot.
     * @param tile The tile to capture (can be nullptr for empty tile)
     * @param pos Position of the tile
     */
    static TileSnapshot capture(const Tile* tile, const Position& pos);
    
    /**
     * Restore tile from snapshot.
     * @return New tile with restored state, or nullptr if snapshot was empty
     */
    std::unique_ptr<Tile> restore() const;
    
    /**
     * Get position of this snapshot.
     */
    const Position& getPosition() const { return position_; }
    
    /**
     * Check if snapshot is empty (no tile data).
     */
    bool isEmpty() const { return data_.empty(); }
    
    /**
     * Get raw data size (uncompressed).
     */
    size_t dataSize() const { return data_.size(); }
    
    /**
     * Get memory footprint.
     */
    size_t memsize() const;
    
    // Direct access for compression
    std::vector<uint8_t>& data() { return data_; }
    const std::vector<uint8_t>& data() const { return data_; }
    void setData(std::vector<uint8_t> data) { data_ = std::move(data); }
    
private:
    Position position_;
    std::vector<uint8_t> data_;  // Serialized tile data
    
    // Serialization helpers
    void serializeTile(const Tile* tile);
    std::unique_ptr<Tile> deserializeTile() const;
};

} // namespace MapEditor::Domain::History
