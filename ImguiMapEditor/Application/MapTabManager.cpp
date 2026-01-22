#include "MapTabManager.h"
#include "Domain/MapInstance.h"
#include "Rendering/Frame/RenderingManager.h"
#include <spdlog/spdlog.h>

namespace MapEditor::AppLogic {

// Static session ID counter
static Rendering::RenderingManager::SessionID s_next_session_id = 1;

MapTabManager::MapTabManager() : clipboard_(copybuffer_) {}

MapTabManager::~MapTabManager() {
  if (rendering_manager_) {
    for (const auto &session : sessions_) {
      rendering_manager_->destroyRenderState(session->getID());
    }
  }
}

int MapTabManager::openMap(std::unique_ptr<Domain::ChunkedMap> map,
                           const std::filesystem::path &path) {
  if (!rendering_manager_) {
    spdlog::warn("MapTabManager: RenderingManager not set!");
  }

  // 1. Create Document (Domain)
  auto document =
      std::make_unique<Domain::MapInstance>(std::move(map), client_data_);
  if (!path.empty()) {
    document->setFilePath(path);
  }

  // 2. Generate Session ID
  auto session_id = s_next_session_id++;

  // 3. Create Render State (Presentation)
  if (rendering_manager_) {
    rendering_manager_->createRenderState(session_id, client_data_);
  }

  // 4. Create Session (App Logic View)
  auto session =
      std::make_unique<EditorSession>(std::move(document), session_id);

  // Wire up modification callback for cache invalidation
  if (on_session_modified_) {
    session->setOnModifiedCallback(on_session_modified_);
  }

  sessions_.push_back(std::move(session));
  int new_index = static_cast<int>(sessions_.size()) - 1;

  // Auto-activate the new tab
  setActiveTab(new_index);

  return new_index;
}

int MapTabManager::createNewMap(uint16_t width, uint16_t height,
                                uint32_t version) {
  auto map = std::make_unique<Domain::ChunkedMap>();
  map->createNew(width, height, version);
  return openMap(std::move(map));
}

void MapTabManager::closeTab(int index) {
  if (index < 0 || index >= static_cast<int>(sessions_.size())) {
    return;
  }

  // Calculate new active index BEFORE erasing
  int old_active = active_index_;
  int new_active = active_index_;

  if (sessions_.size() == 1) {
    // Last tab being closed
    new_active = -1;
  } else if (index == active_index_) {
    // Closing active tab - switch to previous or next
    new_active = (index > 0) ? index - 1 : 0;
  } else if (index < active_index_) {
    // Closing tab before active - active shifts down
    new_active = active_index_ - 1;
  }
  // else: closing tab after active - no change

  // Capture session ID before destroying session
  auto session_id = sessions_[index]->getID();

  // Now erase session
  sessions_.erase(sessions_.begin() + index);

  // Destroy render state AFTER session is gone (or before? Doesn't matter as
  // they are decoupled now)
  if (rendering_manager_) {
    rendering_manager_->destroyRenderState(session_id);
  }

  active_index_ = new_active;

  // Call callback with adjusted indices (skip saving if we closed the active
  // tab)
  if (on_tab_changed_) {
    on_tab_changed_(-1, active_index_); // -1 means don't try to save old state
  }
}

std::vector<std::unique_ptr<EditorSession>>
MapTabManager::extractAllSessions() {
  // Note: This method does not check for unsaved changes. The caller must
  // ensure users are prompted before calling this (as done in
  // Application::changeClientVersion).
  std::vector<std::unique_ptr<EditorSession>> extracted;
  extracted.reserve(sessions_.size());

  // We do NOT destroy render states here, assuming caller wants to keep them
  // alive alongside the extracted sessions, OR caller will destroy everything.
  // Actually, if sessions are extracted, MapTabManager no longer tracks them.
  // This allows deferred destruction. RenderingManager still holds the states.
  // Caller must eventually ensure states are cleaned up via
  // RenderingManager::release() if resetting app, or if closing sessions
  // manually.

  for (auto &session : sessions_) {
    extracted.push_back(std::move(session));
  }

  int old_active = active_index_;
  sessions_.clear();
  active_index_ = -1;

  if (on_tab_changed_) {
    on_tab_changed_(old_active, active_index_);
  }

  return extracted;
}

std::unique_ptr<EditorSession> MapTabManager::extractSession(int index) {
  if (index < 0 || index >= static_cast<int>(sessions_.size())) {
    spdlog::warn("MapTabManager::extractSession - invalid index {}", index);
    return nullptr;
  }

  int old_active = active_index_;
  int new_active = active_index_;

  // Calculate new active index
  if (sessions_.size() == 1) {
    new_active = -1;
  } else if (index == active_index_) {
    new_active = (index > 0) ? index - 1 : 0;
  } else if (index < active_index_) {
    new_active = active_index_ - 1;
  }

  // Extract the session (move ownership out)
  std::unique_ptr<EditorSession> extracted = std::move(sessions_[index]);

  // Erase the empty slot
  sessions_.erase(sessions_.begin() + index);
  active_index_ = new_active;

  // Fire callback
  if (on_tab_changed_) {
    on_tab_changed_(-1, active_index_);
  }

  return extracted;
}

void MapTabManager::setActiveTab(int index) {
  if (index < 0 || index >= static_cast<int>(sessions_.size())) {
    return;
  }

  if (active_index_ != index) {
    int old_index = active_index_;
    active_index_ = index;
    if (on_tab_changed_) {
      on_tab_changed_(old_index, active_index_);
    }
  }
}

EditorSession *MapTabManager::getActiveSession() {
  if (active_index_ < 0 ||
      active_index_ >= static_cast<int>(sessions_.size())) {
    return nullptr;
  }
  return sessions_[active_index_].get();
}

const EditorSession *MapTabManager::getActiveSession() const {
  if (active_index_ < 0 ||
      active_index_ >= static_cast<int>(sessions_.size())) {
    return nullptr;
  }
  return sessions_[active_index_].get();
}

EditorSession *MapTabManager::getSession(int index) {
  if (index < 0 || index >= static_cast<int>(sessions_.size())) {
    return nullptr;
  }
  return sessions_[index].get();
}

const EditorSession *MapTabManager::getSession(int index) const {
  if (index < 0 || index >= static_cast<int>(sessions_.size())) {
    return nullptr;
  }
  return sessions_[index].get();
}

bool MapTabManager::hasUnsavedChanges() const {
  for (const auto &session : sessions_) {
    if (session->isModified()) {
      return true;
    }
  }
  return false;
}

} // namespace MapEditor::AppLogic
