#pragma once

#include <vector>
#include <random>
#include <optional>
#include <cstdint>

namespace MapEditor::Brushes {

/**
 * Utility for weighted random selection.
 * 
 * Used by brushes with multiple weighted options.
 * Provides thread-safe random number generation.
 */
class WeightedSelection {
public:
    /**
     * Select an index from items with weights.
     * 
     * @param rng Reference to random number engine
     * @param weights Vector of weights (values > 0)
     * @return Selected index, or nullopt if weights are empty/all zero
     */
    static std::optional<size_t> select(std::mt19937 &rng, const std::vector<uint32_t>& weights);
    
    /**
     * Get a random integer in the range [min, max] inclusive.
     */
    static uint32_t randomRange(std::mt19937 &rng, uint32_t min, uint32_t max);

    /**
     * Shuffle a vector of indices in-place using the provided RNG.
     */
    static void shuffleIndices(std::mt19937 &rng, std::vector<size_t>& indices);
};

} // namespace MapEditor::Brushes
