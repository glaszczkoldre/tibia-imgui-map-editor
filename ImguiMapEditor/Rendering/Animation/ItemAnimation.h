#pragma once

#include "Domain/ItemType.h"
#include <cstdint>

namespace MapEditor {
namespace Rendering {

struct ItemAnimation {
    static int getPhase(const Domain::ItemType &item, int64_t global_ms,
                        int tile_x, int tile_y, int tile_z);

    static int getPhaseFromFrames(int frames, int64_t global_ms,
                                  const std::vector<std::pair<uint32_t, uint32_t>>* durations = nullptr,
                                  uint32_t total_dur = 0);

private:
    static int getPhaseDuration(const Domain::ItemType &item, int phase);
};

} // namespace Rendering
} // namespace MapEditor
