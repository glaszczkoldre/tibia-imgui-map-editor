#include "AppStateManager.h"
#include <spdlog/spdlog.h>

namespace MapEditor {

void AppStateManager::transition(State new_state) {
    if (new_state == current_state_) {
        return;
    }

    const char* state_names[] = { "Startup", "Editor" };
    spdlog::info("AppStateManager: {} -> {}", 
                 state_names[static_cast<int>(current_state_)],
                 state_names[static_cast<int>(new_state)]);

    current_state_ = new_state;
}

void AppStateManager::update() {
    switch (current_state_) {
        case State::Startup:
            if (startup_updater_) startup_updater_();
            break;
        case State::Editor:
            if (editor_updater_) editor_updater_();
            break;
    }
}

} // namespace MapEditor
