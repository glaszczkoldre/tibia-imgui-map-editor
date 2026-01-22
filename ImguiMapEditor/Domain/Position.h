#pragma once

#include <cstdint>
#include <functional>

namespace MapEditor {
namespace Domain {

/**
 * Represents a 3D position in the map (x, y, z)
 * - x, y: horizontal coordinates
 * - z: floor level (0-15, where 7 is ground level)
 */
struct Position {
    int32_t x = 0;
    int32_t y = 0;
    int16_t z = 7;  // Ground floor by default

    Position() = default;
    Position(int32_t x, int32_t y, int16_t z) : x(x), y(y), z(z) {}

    bool operator==(const Position& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const Position& other) const {
        return !(*this == other);
    }

    bool operator<(const Position& other) const {
        if (z != other.z) return z < other.z;
        if (y != other.y) return y < other.y;
        return x < other.x;
    }

    // Check if position is valid (within map bounds)
    bool isValid() const {
        return x >= 0 && y >= 0 && z >= 0 && z <= 15;
    }

    // Generate a unique hash for map storage
    uint64_t hash() const {
        // Boost-style hash combine
        size_t seed = 0;
        hash_combine(seed, x);
        hash_combine(seed, y);
        hash_combine(seed, z);
        return seed;
    }
    
    /**
     * Pack position into a single 64-bit value.
     * Layout:
     * - Bits 0-7:   z (8 bits)
     * - Bits 8-35:  y (28 bits)
     * - Bits 36-63: x (28 bits)
     *
     * Supports signed values for X/Y in range [-134,217,728, 134,217,727].
     * Z is truncated to 8 bits (0-255).
     */
    uint64_t pack() const {
        // Mask to 28 bits (0xFFFFFFF)
        uint64_t x_part = (static_cast<uint64_t>(x) & 0xFFFFFFF) << 36;
        uint64_t y_part = (static_cast<uint64_t>(y) & 0xFFFFFFF) << 8;
        uint64_t z_part = static_cast<uint64_t>(z) & 0xFF;
        return x_part | y_part | z_part;
    }
    
    // Unpack a position from a 64-bit value
    static Position unpack(uint64_t packed) {
        // Unpack Z (8 bits)
        int16_t z = static_cast<int16_t>(packed & 0xFF);

        // Unpack Y (28 bits) - use arithmetic shift for sign extension
        // Shift left 4 to move 28-bit sign to bit 31, then arithmetic right shift 4
        int32_t y = static_cast<int32_t>((packed >> 8) & 0xFFFFFFF);
        y = (y << 4) >> 4;

        // Unpack X (28 bits) - use arithmetic shift for sign extension
        int32_t x = static_cast<int32_t>((packed >> 36) & 0xFFFFFFF);
        x = (x << 4) >> 4;

        return Position(x, y, z);
    }

private:
    template <typename T>
    static void hash_combine(size_t& seed, const T& v) {
        std::hash<T> hasher;
        // 0x9e3779b9 is for 32-bit systems, 0x9e3779b97f4a7c15 for 64-bit.
        // Since we target 64-bit modern platforms for the editor, let's use the appropriate constant if size_t is 64-bit.
        // However, keeping it simple/standard with the 32-bit derived constant is often "good enough" unless collisions are high.
        // Let's stick to the standard Boost constant often cited.
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
};

// Floor constants
constexpr int16_t FLOOR_MIN = 0;
constexpr int16_t FLOOR_MAX = 15;
constexpr int16_t FLOOR_GROUND = 7;
constexpr int16_t FLOOR_SEA = 7;      // Sea level (same as ground)
constexpr int16_t FLOOR_UNDERGROUND_START = 8;

} // namespace Domain
} // namespace MapEditor

// Hash specialization for std::unordered_map
namespace std {
template<>
struct hash<MapEditor::Domain::Position> {
    size_t operator()(const MapEditor::Domain::Position& pos) const {
        return pos.hash();
    }
};
} // namespace std
