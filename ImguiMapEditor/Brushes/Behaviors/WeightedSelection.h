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
     * @param weights Vector of weights (values > 0)
     * @return Selected index, or nullopt if weights are empty/all zero
     */
    static std::optional<size_t> select(const std::vector<uint32_t>& weights);
    
    /**
     * Thickness probability check.
     * Returns true if placement should proceed based on thickness value.
     * 
     * @param thickness Value between 0.0 and 1.0
     * @return true if placement should proceed
     */
    static bool passesThicknessCheck(float thickness);
    
    /**
     * Get a random integer in the range [min, max] inclusive.
     */
    static uint32_t randomRange(uint32_t min, uint32_t max);
    
    /**
     * Get a random float in the range [0.0, 1.0).
     */
    static float randomFloat();
    
private:
    static thread_local std::mt19937 rng_;
    static thread_local bool initialized_;
    
    static void ensureInitialized();
};

} // namespace MapEditor::Brushes
