#pragma once

#include <cstdint>

namespace MapEditor {
namespace Rendering {

/**
 * Pre-calculated animation tick values for a single frame.
 * Eliminates per-item clock reads and divisions.
 */
struct AnimationTicks {
    int64_t tick_500ms = 0;
    int64_t tick_250ms = 0;
    int64_t tick_100ms = 0;
    
    static AnimationTicks calculate(int64_t frame_time_ms) {
        AnimationTicks ticks;
        ticks.tick_500ms = frame_time_ms / 500;
        ticks.tick_250ms = frame_time_ms / 250;
        ticks.tick_100ms = frame_time_ms / 100;
        return ticks;
    }
};

} // namespace Rendering
} // namespace MapEditor
