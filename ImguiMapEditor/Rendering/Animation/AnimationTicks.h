#pragma once

#include <cstdint>

namespace MapEditor {
namespace Rendering {

struct AnimationTicks {
    int64_t global_ms = 0;

    static AnimationTicks calculate(int64_t frame_time_ms) {
        return { frame_time_ms };
    }
};

} // namespace Rendering
} // namespace MapEditor
