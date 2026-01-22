#pragma once
#include <algorithm>
#include <limits>
#include <cmath>

namespace MapEditor {
namespace Math {

// Safely cast float to int, clamping to int limits to avoid Undefined Behavior
inline int safe_float_to_int(float value) {
    if (std::isnan(value)) return 0;

    // We use constants that are guaranteed to be exactly representable as floats
    // and strictly less than INT_MAX/INT_MIN to avoid rounding errors causing UB during cast.
    // 2^31 - 128 is a safe upper bound for float representation of int32 max
    static constexpr float MAX_SAFE = 2147483520.0f;
    static constexpr float MIN_SAFE = -2147483520.0f;

    // Clamp first
    float clamped = std::clamp(value, MIN_SAFE, MAX_SAFE);

    // Now safe to cast
    return static_cast<int>(clamped);
}

} // namespace Math
} // namespace MapEditor
