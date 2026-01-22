#pragma once
#include "HistoryBuffer.h"
#include "HistoryEntry.h"
#include "TileSnapshot.h"
#include "HistoryConfig.h"
#include "../ChunkedMap.h"
#include "../Position.h"
#include "../Selection/SelectionSnapshot.h"
#include <string>
#include <unordered_map>
#include <optional>

namespace MapEditor::Services {
    class ClientDataService;
}

namespace MapEditor::Services::Selection {
    class SelectionService;
}

namespace MapEditor::Domain::History {

/**
 * Main API for undo/redo system.
 * Replaces ActionQueue with tile-snapshot based approach.
 */
class HistoryManager {
public:
    explicit HistoryManager(const HistoryConfig& config = HistoryConfig{});
    
    // ========================
    // Operation Recording API
    // ========================
    
    /**
     * Begin a multi-tile operation.
     * Call recordTileBefore() for each tile, make changes, call endOperation().
     * @param selection Optional SelectionService to capture selection state
     */
    void beginOperation(const std::string& description, ActionType type = ActionType::Other,
                        Services::Selection::SelectionService* selection = nullptr);
    
    /**
     * Record a tile's BEFORE state (call before modifying tile).
     */
    void recordTileBefore(const Position& pos, const Tile* tile);
    
    /**
     * End operation and push to history.
     * Captures AFTER states for all recorded tiles.
     * @param map The map to capture AFTER states from
     * @param selection Optional SelectionService to capture selection state
     */
    void endOperation(ChunkedMap* map, Services::Selection::SelectionService* selection = nullptr);
    
    /**
     * Cancel current operation without pushing to history.
     */
    void cancelOperation();
    
    /**
     * Check if operation is in progress.
     */
    bool isOperationActive() const { return operation_active_; }
    
    // ========================
    // Single-Tile Convenience
    // ========================
    
    /**
     * Record and commit a single-tile change.
     * Captures BEFORE, applies change, captures AFTER, pushes to history.
     */
    void recordSingleTileChange(
        ChunkedMap* map,
        const Position& pos,
        const std::string& description,
        ActionType type = ActionType::Other
    );
    
    // ========================
    // Undo/Redo API
    // ========================
    
    /**
     * Undo last operation.
     * @param map The map to apply changes to
     * @param clientData Optional ClientDataService for resolving ItemTypes
     * @param selection Optional SelectionService to restore selection state
     * @return Description of undone operation, or empty if nothing to undo
     */
    std::string undo(ChunkedMap* map, Services::ClientDataService* clientData = nullptr,
                     Services::Selection::SelectionService* selection = nullptr);
    
    /**
     * Redo last undone operation.
     * @param map The map to apply changes to
     * @param clientData Optional ClientDataService for resolving ItemTypes
     * @param selection Optional SelectionService to restore selection state
     * @return Description of redone operation, or empty if nothing to redo
     */
    std::string redo(ChunkedMap* map, Services::ClientDataService* clientData = nullptr,
                     Services::Selection::SelectionService* selection = nullptr);
    
    bool canUndo() const { return buffer_.canUndo(); }
    bool canRedo() const { return buffer_.canRedo(); }
    
    std::string getUndoDescription() const { return buffer_.getUndoDescription(); }
    std::string getRedoDescription() const { return buffer_.getRedoDescription(); }
    
    // ========================
    // Stats & Management
    // ========================
    
    size_t memoryUsage() const { return buffer_.totalMemory(); }
    size_t entryCount() const { return buffer_.size(); }
    
    void clear() { buffer_.clear(); }
    
private:
    // Hash for Position in unordered_map
    struct PositionHash {
        size_t operator()(const Position& p) const {
            return std::hash<int64_t>()(
                static_cast<int64_t>(p.x) | 
                (static_cast<int64_t>(p.y) << 20) |
                (static_cast<int64_t>(p.z) << 40)
            );
        }
    };
    
    HistoryBuffer buffer_;
    
    // Active operation state
    bool operation_active_ = false;
    std::string current_description_;
    ActionType current_type_ = ActionType::Other;
    std::unordered_map<Position, TileSnapshot, PositionHash> before_states_;
    
    // Selection state for current operation (optional)
    std::optional<Domain::Selection::SelectionSnapshot> selection_before_;
};

} // namespace MapEditor::Domain::History
