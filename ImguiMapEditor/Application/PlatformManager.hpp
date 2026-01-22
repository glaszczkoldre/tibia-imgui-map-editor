#pragma once

#include "Controllers/WindowController.h"
#include "Platform/ImGuiBackend.h"
#include "Platform/PlatformCallbackRouter.h"
#include "Services/ConfigService.h"

namespace MapEditor {

class PlatformManager {
public:
  /// Default constructor
  PlatformManager();

  /// Destructor - ensures safe shutdown
  ~PlatformManager();

  // No copy/move
  PlatformManager(const PlatformManager &) = delete;
  PlatformManager &operator=(const PlatformManager &) = delete;

  /// Initialize platform components (Window, ImGui, NFD)
  /// Must be called after SettingsRegistry is loaded
  bool initialize(Services::ConfigService &config);

  /// Shutdown platform components
  void shutdown();

  /// Update window state. Returns false if frame should be skipped.
  bool update();

  /// Check if application should close (window close request)
  bool shouldClose() const;

  /// Save window state to config
  void saveWindowState(Services::ConfigService &config) const;

  // Accessors

  /// Get the main application window
  Platform::GlfwWindow &getWindow() { return window_controller_.getWindow(); }
  /// Get the main application window (const)
  const Platform::GlfwWindow &getWindow() const { return window_controller_.getWindow(); }

  /// Get the ImGui backend
  Platform::ImGuiBackend &getImGuiBackend() { return imgui_backend_; }
  /// Get the ImGui backend (const)
  const Platform::ImGuiBackend &getImGuiBackend() const { return imgui_backend_; }

  /// Get the platform callback router
  Platform::PlatformCallbackRouter &getCallbackRouter() { return callback_router_; }
  /// Get the platform callback router (const)
  const Platform::PlatformCallbackRouter &getCallbackRouter() const { return callback_router_; }

private:
  Core::WindowController window_controller_;
  Platform::ImGuiBackend imgui_backend_;
  Platform::PlatformCallbackRouter callback_router_;
  bool shutdown_ = false;
  bool nfd_initialized_ = false;
};

} // namespace MapEditor
