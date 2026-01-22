#include "Rendering/Frame/RenderState.h"
#include <spdlog/spdlog.h>

namespace MapEditor::Rendering {

RenderState::RenderState(Services::ClientDataService* client_data) {
    light_manager = std::make_unique<LightManager>(client_data);
    if (!light_manager->initialize()) {
        spdlog::warn("Failed to initialize LightManager for RenderState");
    }
}

RenderState::~RenderState() = default;

void RenderState::invalidateAll() {
    chunk_cache.invalidateAll();
    if (light_manager) {
        light_manager->invalidateAll();
    }
    overlay_collector.clear();
}

void RenderState::invalidateChunk(int32_t chunk_x, int32_t chunk_y, int8_t floor) {
    chunk_cache.invalidate(chunk_x, chunk_y, floor);
}

void RenderState::invalidateLight(int32_t x, int32_t y) {
    if (light_manager) {
        light_manager->invalidateTile(x, y);
    }
}

} // namespace MapEditor::Rendering
