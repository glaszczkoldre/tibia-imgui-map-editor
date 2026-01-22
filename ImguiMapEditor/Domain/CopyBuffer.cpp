#include "CopyBuffer.h"
#include <limits>

namespace MapEditor::Domain {

void CopyBuffer::setTiles(std::vector<CopiedTile> tiles) {
    tiles_ = std::move(tiles);
}

void CopyBuffer::clear() {
    tiles_.clear();
}

int32_t CopyBuffer::getWidth() const {
    if (tiles_.empty()) return 0;
    
    int32_t min_x = std::numeric_limits<int32_t>::max();
    int32_t max_x = std::numeric_limits<int32_t>::min();
    
    for (const auto& ct : tiles_) {
        if (ct.relative_pos.x < min_x) min_x = ct.relative_pos.x;
        if (ct.relative_pos.x > max_x) max_x = ct.relative_pos.x;
    }
    
    return max_x - min_x + 1;
}

int32_t CopyBuffer::getHeight() const {
    if (tiles_.empty()) return 0;
    
    int32_t min_y = std::numeric_limits<int32_t>::max();
    int32_t max_y = std::numeric_limits<int32_t>::min();
    
    for (const auto& ct : tiles_) {
        if (ct.relative_pos.y < min_y) min_y = ct.relative_pos.y;
        if (ct.relative_pos.y > max_y) max_y = ct.relative_pos.y;
    }
    
    return max_y - min_y + 1;
}

Position CopyBuffer::getMinBound() const {
    if (tiles_.empty()) {
        return Position{0, 0, 0};
    }
    
    int32_t min_x = std::numeric_limits<int32_t>::max();
    int32_t min_y = std::numeric_limits<int32_t>::max();
    int16_t min_z = std::numeric_limits<int16_t>::max();
    
    for (const auto& ct : tiles_) {
        if (ct.relative_pos.x < min_x) min_x = ct.relative_pos.x;
        if (ct.relative_pos.y < min_y) min_y = ct.relative_pos.y;
        if (ct.relative_pos.z < min_z) min_z = ct.relative_pos.z;
    }
    
    return Position{min_x, min_y, min_z};
}

Position CopyBuffer::getMaxBound() const {
    if (tiles_.empty()) {
        return Position{0, 0, 0};
    }
    
    int32_t max_x = std::numeric_limits<int32_t>::min();
    int32_t max_y = std::numeric_limits<int32_t>::min();
    int16_t max_z = std::numeric_limits<int16_t>::min();
    
    for (const auto& ct : tiles_) {
        if (ct.relative_pos.x > max_x) max_x = ct.relative_pos.x;
        if (ct.relative_pos.y > max_y) max_y = ct.relative_pos.y;
        if (ct.relative_pos.z > max_z) max_z = ct.relative_pos.z;
    }
    
    return Position{max_x, max_y, max_z};
}

} // namespace MapEditor::Domain
