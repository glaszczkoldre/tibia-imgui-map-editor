#pragma once
#include "HistoryEntry.h"
#include "HistoryConfig.h"
#include <boost/circular_buffer.hpp>
#include <memory>

namespace MapEditor::Domain::History {

/**
 * Circular buffer wrapper for history entries.
 * Uses Boost.CircularBuffer for efficient fixed-size undo/redo storage.
 */
class HistoryBuffer {
public:
    explicit HistoryBuffer(const HistoryConfig& config = HistoryConfig{});
    
    /**
     * Push a new entry. Clears any redo entries ahead of current position.
     */
    void push(std::unique_ptr<HistoryEntry> entry);
    
    /**
     * Move back (undo navigation).
     * @return Entry that was undone, or nullptr if can't undo
     */
    HistoryEntry* moveBack();
    
    /**
     * Move forward (redo navigation).
     * @return Entry that was redone, or nullptr if can't redo
     */
    HistoryEntry* moveForward();
    
    /**
     * Check if can undo.
     */
    bool canUndo() const { return current_index_ > 0; }
    
    /**
     * Check if can redo.
     */
    bool canRedo() const { return current_index_ < entries_.size(); }
    
    /**
     * Get description of next undo action.
     */
    std::string getUndoDescription() const;
    
    /**
     * Get description of next redo action.
     */
    std::string getRedoDescription() const;
    
    /**
     * Get total memory usage.
     */
    size_t totalMemory() const { return current_memory_; }
    
    /**
     * Get number of entries.
     */
    size_t size() const { return entries_.size(); }
    
    /**
     * Clear all history.
     */
    void clear();
    
private:
    void trimToMemoryLimit();
    
    boost::circular_buffer<std::unique_ptr<HistoryEntry>> entries_;
    size_t current_index_ = 0;  // Points to next redo position
    
    HistoryConfig config_;
    size_t current_memory_ = 0;
};

} // namespace MapEditor::Domain::History
