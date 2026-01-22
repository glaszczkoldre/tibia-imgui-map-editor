#include "Application/PlatformManager.hpp"
#include <nfd.hpp>
#include <spdlog/spdlog.h>

namespace MapEditor {

PlatformManager::PlatformManager() = default;

PlatformManager::~PlatformManager() { shutdown(); }

bool PlatformManager::initialize(Services::ConfigService &config) {
  if (!window_controller_.initialize(config))
    return false;

  // Pass ImGui ini path to store layout in same folder as config.json
  if (!imgui_backend_.initialize(window_controller_.getWindow(),
                                 config.getImGuiIniPath().c_str()))
    return false;

  if (NFD::Init() != NFD_OKAY) {
    spdlog::error("Failed to initialize Native File Dialog.");
    return false;
  }
  nfd_initialized_ = true;

  return true;
}

void PlatformManager::shutdown() {
  if (shutdown_) {
    return;
  }
  shutdown_ = true;

  if (nfd_initialized_) {
    NFD::Quit();
    nfd_initialized_ = false;
  }
  imgui_backend_.shutdown();
  window_controller_.shutdown();
}

bool PlatformManager::update() {
  return window_controller_.update();
}

bool PlatformManager::shouldClose() const {
  return window_controller_.shouldClose();
}

void PlatformManager::saveWindowState(Services::ConfigService &config) const {
  window_controller_.saveState(config);
}

} // namespace MapEditor
