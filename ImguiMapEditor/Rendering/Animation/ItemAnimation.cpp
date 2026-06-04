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

int ItemAnimation::getPingPongPhase(int raw_phase, int frames) {
    if (frames <= 1) return 0;
    int cycle = frames * 2 - 2;
    if (cycle <= 0) return 0;
    int mod = raw_phase % cycle;
    return mod >= frames ? cycle - mod : mod;
}

int ItemAnimation::getPhase(const Domain::ItemType &item, int64_t global_ms,
                             int tile_x, int tile_y, int tile_z) {
    int frames = std::max<int>(item.frames, 1);
    if (frames <= 1)
        return 0;

    // animation_mode: 0=async (per-tile offset), 1=sync (global clock)
    bool is_async = !item.frame_durations.empty() && item.animation_mode == 0;
    int tile_offset = is_async ? (tile_x * 17 + tile_y * 31 + tile_z * 7) : 0;

    // start_frame: 255=random per instance (async only), >0=fixed phase
    int start_phase = 0;
    if (item.start_frame == 255 && is_async) {
        start_phase = (tile_x * 17 + tile_y * 31 + tile_z * 7) % frames;
    } else if (item.start_frame > 0) {
        start_phase = item.start_frame % frames;
    }

    // Ping-pong (loop_count == -1)
    if (item.loop_count == -1) {
        if (item.frame_durations.empty()) {
            int tick = static_cast<int>(global_ms / 500);
            return getPingPongPhase(tick + tile_offset + start_phase, frames);
        }
        int total = static_cast<int>(item.total_duration);
        if (total <= 0) return start_phase;
        int cycle_total = total * 2;
        int elapsed = static_cast<int>((global_ms + static_cast<int64_t>(tile_offset) * 500) % cycle_total);
        if (elapsed >= total)
            elapsed = cycle_total - elapsed;
        for (int phase = 0; phase < static_cast<int>(item.frame_durations.size()); ++phase) {
            int dur = getPhaseDuration(item, phase);
            if (elapsed < dur)
                return (phase + start_phase) % frames;
            elapsed -= dur;
        }
        return start_phase;
    }

    // Forward loop (loop_count == 0 or > 0)
    if (item.frame_durations.empty()) {
        int tick = static_cast<int>(global_ms / 500);
        return (tick + tile_offset + start_phase) % frames;
    }

    int total = static_cast<int>(item.total_duration);
    if (total <= 0)
        return start_phase;

    int64_t elapsed_ms = global_ms + static_cast<int64_t>(tile_offset) * 500;
    int elapsed = static_cast<int>(elapsed_ms % total);

    for (int phase = 0; phase < static_cast<int>(item.frame_durations.size()); ++phase) {
        int dur = getPhaseDuration(item, phase);
        if (elapsed < dur)
            return (phase + start_phase) % frames;
        elapsed -= dur;
    }

    return start_phase;
}

int ItemAnimation::getPhaseDuration(const Domain::ItemType &item, int phase) {
    if (phase < 0 || phase >= static_cast<int>(item.frame_durations.size()))
        return 500;
    const auto &d = item.frame_durations[phase];
    return static_cast<int>((d.first + d.second) / 2);
}

} // namespace Rendering
} // namespace MapEditor
