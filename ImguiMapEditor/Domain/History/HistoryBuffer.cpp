#include "HistoryBuffer.h"
#include <spdlog/spdlog.h>

namespace MapEditor::Domain::History {

HistoryBuffer::HistoryBuffer(const HistoryConfig& config)
    : entries_(config.max_entries)
    , config_(config) {
}

void HistoryBuffer::push(std::unique_ptr<HistoryEntry> entry) {
    if (!entry || !entry->hasChanges()) {
        return;
    }
    
    // Compress the entry
    entry->compress(config_.enable_compression);
    
    // Clear any redo entries (entries after current position)
    while (entries_.size() > current_index_) {
        current_memory_ -= entries_.back()->memsize();
        entries_.pop_back();
    }
    
    // Track memory
    size_t entry_size = entry->memsize();
    current_memory_ += entry_size;
    
    // Add new entry
    entries_.push_back(std::move(entry));
    current_index_ = entries_.size();
    
    // Trim if over memory limit
    trimToMemoryLimit();
    
    spdlog::debug("[History] Pushed entry, {} entries, {} bytes", 
                  entries_.size(), current_memory_);
}

HistoryEntry* HistoryBuffer::moveBack() {
    if (!canUndo()) {
        return nullptr;
    }
    
    --current_index_;
    return entries_[current_index_].get();
}

HistoryEntry* HistoryBuffer::moveForward() {
    if (!canRedo()) {
        return nullptr;
    }
    
    HistoryEntry* entry = entries_[current_index_].get();
    ++current_index_;
    return entry;
}

std::string HistoryBuffer::getUndoDescription() const {
    if (!canUndo()) {
        return "";
    }
    return entries_[current_index_ - 1]->getDescription();
}

std::string HistoryBuffer::getRedoDescription() const {
    if (!canRedo()) {
        return "";
    }
    return entries_[current_index_]->getDescription();
}

void HistoryBuffer::clear() {
    entries_.clear();
    current_index_ = 0;
    current_memory_ = 0;
}

void HistoryBuffer::trimToMemoryLimit() {
    // Remove oldest entries if over memory limit
    while (current_memory_ > config_.max_memory_bytes && !entries_.empty()) {
        size_t oldest_size = entries_.front()->memsize();
        entries_.pop_front();
        current_memory_ -= oldest_size;
        
        // Adjust current index
        if (current_index_ > 0) {
            --current_index_;
        }
    }
}

} // namespace MapEditor::Domain::History
