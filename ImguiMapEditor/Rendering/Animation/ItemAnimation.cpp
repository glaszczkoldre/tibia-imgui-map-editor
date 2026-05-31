#include "ItemAnimation.h"
#include <algorithm>

namespace MapEditor {
namespace Rendering {

int ItemAnimation::getPhaseFromFrames(int frames, int64_t global_ms,
                                       const std::vector<std::pair<uint32_t, uint32_t>>* durations,
                                       uint32_t total_dur) {
    if (frames <= 1)
        return 0;

    if (durations && total_dur > 0) {
        int elapsed = static_cast<int>(global_ms % total_dur);
        for (int phase = 0; phase < static_cast<int>(durations->size()); ++phase) {
            int dur = static_cast<int>(((*durations)[phase].first + (*durations)[phase].second) / 2);
            if (elapsed < dur)
                return phase % frames;
            elapsed -= dur;
        }
        return 0;
    }

    int tick = static_cast<int>(global_ms / 500);
    return tick % frames;
}

int ItemAnimation::getPhase(const Domain::ItemType &item, int64_t global_ms,
                             int tile_x, int tile_y, int tile_z) {
    int frames = std::max<int>(item.frames, 1);
    if (frames <= 1)
        return 0;

    bool per_instance = !item.frame_durations.empty() && item.animation_mode == 0;
    int tile_offset = per_instance ? (tile_x * 17 + tile_y * 31 + tile_z * 7) : 0;

    if (item.frame_durations.empty()) {
        int tick = static_cast<int>(global_ms / 500);
        return (tick + tile_offset) % frames;
    }

    int total = static_cast<int>(item.total_duration);
    if (total <= 0)
        return 0;

    int64_t elapsed_ms = global_ms + static_cast<int64_t>(tile_offset) * 500;
    int elapsed = static_cast<int>(elapsed_ms % total);

    for (int phase = 0; phase < static_cast<int>(item.frame_durations.size()); ++phase) {
        int dur = getPhaseDuration(item, phase);
        if (elapsed < dur)
            return phase % frames;
        elapsed -= dur;
    }

    return 0;
}

int ItemAnimation::getPhaseDuration(const Domain::ItemType &item, int phase) {
    if (phase < 0 || phase >= static_cast<int>(item.frame_durations.size()))
        return 500;
    const auto &d = item.frame_durations[phase];
    return (d.first + d.second) / 2;
}

} // namespace Rendering
} // namespace MapEditor
