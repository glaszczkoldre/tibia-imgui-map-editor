#include "WeightedSelection.h"
#include <algorithm>
#include <numeric>

namespace MapEditor::Brushes {

thread_local std::mt19937 WeightedSelection::rng_;
thread_local bool WeightedSelection::initialized_ = false;

WeightedSelection::ScopedSeed::ScopedSeed(uint32_t seed)
    : previous_rng_(rng_), previous_initialized_(initialized_) {
    rng_.seed(seed);
    initialized_ = true;
}

WeightedSelection::ScopedSeed::~ScopedSeed() {
    rng_ = previous_rng_;
    initialized_ = previous_initialized_;
}

void WeightedSelection::ensureInitialized() {
    if (!initialized_) {
        std::random_device rd;
        rng_.seed(rd());
        initialized_ = true;
    }
}

std::optional<size_t> WeightedSelection::select(const std::vector<uint32_t>& weights) {
    if (weights.empty()) {
        return std::nullopt;
    }
    
    uint32_t totalWeight = std::accumulate(weights.begin(), weights.end(), 0u);
    if (totalWeight == 0) {
        return std::nullopt;
    }
    
    ensureInitialized();
    std::uniform_int_distribution<uint32_t> dist(0, totalWeight - 1);
    uint32_t roll = dist(rng_);
    
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

uint32_t WeightedSelection::randomRange(uint32_t min, uint32_t max) {
    if (min >= max) {
        return min;
    }
    
    ensureInitialized();
    std::uniform_int_distribution<uint32_t> dist(min, max);
    return dist(rng_);
}

void WeightedSelection::shuffleIndices(std::vector<size_t>& indices) {
    ensureInitialized();
    std::shuffle(indices.begin(), indices.end(), rng_);
}

} // namespace MapEditor::Brushes
