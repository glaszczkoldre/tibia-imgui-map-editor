#pragma once

#include <functional>

namespace MapEditor {

/**
 * Manages application state transitions.
 * Simplified 2-state machine: Startup ↔ Editor
 * 
 * Startup: StartupDialog shown, no map loaded
 * Editor: Map editing mode, at least one map open
 */
class AppStateManager {
public:
    enum class State {
        Startup,  // Was WelcomeScreen - StartupDialog handles everything
        Editor
    };

    using UpdateFn = std::function<void()>;

    AppStateManager() = default;
    ~AppStateManager() = default;

    // Non-copyable, non-movable (owns callbacks)
    AppStateManager(const AppStateManager&) = delete;
    AppStateManager& operator=(const AppStateManager&) = delete;

    // State access
    [[nodiscard]] State current() const noexcept { return current_state_; }
    [[nodiscard]] bool isInState(State s) const noexcept { return current_state_ == s; }

    // State transitions
    void transition(State new_state);

    // Register state-specific update logic
    void setStartupUpdater(UpdateFn fn) { startup_updater_ = std::move(fn); }
    void setEditorUpdater(UpdateFn fn) { editor_updater_ = std::move(fn); }

    // Execute the appropriate update function for current state
    void update();

private:
    State current_state_ = State::Startup;
    UpdateFn startup_updater_;
    UpdateFn editor_updater_;
};

} // namespace MapEditor
