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
    class ScopedSeed {
    public:
        explicit ScopedSeed(uint32_t seed);
        ~ScopedSeed();

        ScopedSeed(const ScopedSeed&) = delete;
        ScopedSeed& operator=(const ScopedSeed&) = delete;

    private:
        std::mt19937 previous_rng_;
        bool previous_initialized_ = false;
    };

    /**
     * Select an index from items with weights.
     * 
     * @param weights Vector of weights (values > 0)
     * @return Selected index, or nullopt if weights are empty/all zero
     */
    static std::optional<size_t> select(const std::vector<uint32_t>& weights);
    
    /**
     * Get a random integer in the range [min, max] inclusive.
     */
    static uint32_t randomRange(uint32_t min, uint32_t max);

    /**
     * Shuffle a vector of indices in-place using the shared RNG.
     */
    static void shuffleIndices(std::vector<size_t>& indices);
    
private:
    static thread_local std::mt19937 rng_;
    static thread_local bool initialized_;
    
    static void ensureInitialized();
};

} // namespace MapEditor::Brushes
