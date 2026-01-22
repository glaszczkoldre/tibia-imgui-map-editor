#include "Rendering/Frame/RenderingManager.h"
#include "Rendering/Frame/RenderState.h"
#include "Rendering/Map/MapRenderer.h"
#include "Services/SpriteManager.h"
#include <spdlog/spdlog.h>

namespace MapEditor::Rendering {

std::unique_ptr<MapRenderer>
RenderingManager::createRenderer(Services::ClientDataService *client_data,
                                 Services::SpriteManager *sprite_manager) {
  spdlog::info("[RenderingManager] createRenderer() FACTORY called:");
  spdlog::info("  - client_data: {}", client_data ? "valid" : "null");
  spdlog::info("  - sprite_manager: {}", sprite_manager ? "valid" : "null");

  auto renderer = std::make_unique<MapRenderer>(client_data, sprite_manager);
  spdlog::info(
      "[RenderingManager] MapRenderer constructed, calling initialize()...");
  renderer->initialize();
  spdlog::info(
      "[RenderingManager] MapRenderer initialized - returning unique_ptr");
  return renderer;
}

void RenderingManager::setRenderer(std::unique_ptr<MapRenderer> renderer,
                                   Services::SpriteManager *sprite_manager) {
  renderer_ = std::move(renderer);
  spdlog::info("RenderingManager: Renderer set ({})",
               renderer_ ? "valid" : "null");

  // NOTE: Cache invalidation on sprite load is NOT wired here.
  // Instead, chunks with missing sprites should not be marked as cached.
  // See ChunkRenderingStrategy::generateCachedChunk for the fix.
}

void RenderingManager::invalidateCache() {
  // Renderer is stateless, so we only need to invalidate session states.

  // Invalidate all session caches too
  for (auto &[id, state] : session_states_) {
    if (state) {
      state->invalidateAll();
    }
  }
  spdlog::debug("RenderingManager: All caches invalidated");
}

void RenderingManager::release() {
  if (renderer_) {
    spdlog::info("RenderingManager: Releasing renderer");
    renderer_.reset();
  }
  // Clear all session states
  session_states_.clear();
}

RenderState *
RenderingManager::createRenderState(SessionID session_id,
                                    Services::ClientDataService *client_data) {
  spdlog::info("RenderingManager: Creating RenderState for session {}",
               session_id);
  auto state = std::make_unique<RenderState>(client_data);
  auto *ptr = state.get();
  session_states_[session_id] = std::move(state);
  return ptr;
}

void RenderingManager::destroyRenderState(SessionID session_id) {
  spdlog::info("RenderingManager: Destroying RenderState for session {}",
               session_id);
  session_states_.erase(session_id);
}

RenderState *RenderingManager::getRenderState(SessionID session_id) const {
  auto it = session_states_.find(session_id);
  if (it != session_states_.end()) {
    return it->second.get();
  }
  return nullptr;
}

} // namespace MapEditor::Rendering
