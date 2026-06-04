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
    int mod = raw_phase % cycle;
    return mod >= frames ? cycle - mod : mod;
}

int ItemAnimation::getPhase(const Domain::ItemType &item, int64_t global_ms,
                             int tile_x, int tile_y, int tile_z) {
    int frames = std::max<int>(item.frames, 1);
    if (frames <= 1)
        return 0;

    bool is_async = !item.frame_durations.empty() && item.animation_mode == 0;
    int tile_offset = is_async ? (tile_x * 17 + tile_y * 31 + tile_z * 7) : 0;

    int start_phase = 0;
    if (item.start_frame == 255 && is_async) {
        start_phase = (tile_x * 17 + tile_y * 31 + tile_z * 7) % frames;
    } else if (item.start_frame > 0) {
        start_phase = item.start_frame % frames;
    }

    if (item.loop_count == -1) {
        // Ping-pong: apply start_phase to raw phase BEFORE reflection
        if (item.frame_durations.empty()) {
            int tick = static_cast<int>(global_ms / 500);
            return getPingPongPhase(tick + tile_offset + start_phase, frames);
        }

        // Build explicit ping-pong timeline from per-phase durations
        const int num_phases = static_cast<int>(item.frame_durations.size());
        if (num_phases == 0) return start_phase;

        // Per-phase durations (stack allocation — clamp to prevent overflow)
        constexpr int MAX_PHASES = 32;
        const int effective_phases = std::min<int>(num_phases, MAX_PHASES);
        int phase_durs[MAX_PHASES];
        int cycle_length = 0;
        for (int i = 0; i < effective_phases; ++i) {
            phase_durs[i] = getPhaseDuration(item, i);
            cycle_length += phase_durs[i];
        }

        // Ping-pong timeline: [0, 1, ..., N-1, N-2, ..., 1] (omit duplicate endpoints)
        constexpr int MAX_TIMELINE = MAX_PHASES * 2 - 2;
        int timeline[MAX_TIMELINE];
        int timeline_len = 0;
        for (int i = 0; i < effective_phases; ++i)
            timeline[timeline_len++] = i;
        for (int i = effective_phases - 2; i >= 1; --i)
            timeline[timeline_len++] = i;

        // Backward portion cycle length
        int backward_length = 0;
        for (int i = effective_phases - 2; i >= 1; --i)
            backward_length += phase_durs[i];
        cycle_length += backward_length;

        if (cycle_length <= 0) return start_phase;

        // Start offset: sum durations of first start_phase phases
        int start_offset = 0;
        for (int i = 0; i < start_phase && i < effective_phases; ++i)
            start_offset += phase_durs[i];

        int elapsed = static_cast<int>(
            (global_ms + static_cast<int64_t>(tile_offset) * 500 + start_offset) % cycle_length);

        // Walk the timeline to find the active phase
        for (int i = 0; i < timeline_len; ++i) {
            int dur = phase_durs[timeline[i]];
            if (elapsed < dur)
                return timeline[i] % frames;
            elapsed -= dur;
        }
        return 0;
    }

    // Forward loop
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
