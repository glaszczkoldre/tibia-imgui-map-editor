#include "BrushRegistry.h"
#include "Types/RawBrush.h"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace MapEditor::Brushes {

// ========== Brush Management ==========

void BrushRegistry::addBrush(std::unique_ptr<IBrush> brush) {
    if (!brush) return;
    
    std::string name = brush->getName();
    if (named_brushes_.find(name) != named_brushes_.end()) {
        spdlog::warn("[BrushRegistry] Overwriting existing brush with name: {}", name);
    }
    
    named_brushes_[name] = std::move(brush);
}

IBrush* BrushRegistry::getBrush(const std::string& name) const {
    auto it = named_brushes_.find(name);
    if (it != named_brushes_.end()) {
        return it->second.get();
    }
    return nullptr;
}

IBrush* BrushRegistry::getOrCreateRAWBrush(uint16_t itemId) {
    auto it = raw_brushes_.find(itemId);
    if (it != raw_brushes_.end()) {
        return it->second.get();
    }
    
    auto brush = std::make_unique<RawBrush>(itemId);
    auto ptr = brush.get();
    raw_brushes_[itemId] = std::move(brush);
    return ptr;
}

void BrushRegistry::clear() {
    named_brushes_.clear();
    raw_brushes_.clear();
}

} // namespace MapEditor::Brushes
