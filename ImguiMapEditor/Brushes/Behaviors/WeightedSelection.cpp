#include "WeightedSelection.h"
#include <numeric>

namespace MapEditor::Brushes {

std::optional<size_t> WeightedSelection::select(std::mt19937 &rng, const std::vector<uint32_t>& weights) {
    if (weights.empty()) {
        return std::nullopt;
    }
    
    uint32_t totalWeight = std::accumulate(weights.begin(), weights.end(), 0u);
    if (totalWeight == 0) {
        return std::nullopt;
    }
    
    std::uniform_int_distribution<uint32_t> dist(0, totalWeight - 1);
    uint32_t roll = dist(rng);
    
    uint32_t cumulative = 0;
    for (size_t i = 0; i < weights.size(); ++i) {
        cumulative += weights[i];
        if (roll < cumulative) {
            return i;
        }
    }
    
    // Fallback to last item (shouldn't happen if weights are correct)
    return weights.size() - 1;
}

uint32_t WeightedSelection::randomRange(std::mt19937 &rng, uint32_t min, uint32_t max) {
    if (min >= max) {
        return min;
    }
    
    std::uniform_int_distribution<uint32_t> dist(min, max);
    return dist(rng);
}

void WeightedSelection::shuffleIndices(std::mt19937 &rng, std::vector<size_t>& indices) {
    std::shuffle(indices.begin(), indices.end(), rng);
}

} // namespace MapEditor::Brushes
