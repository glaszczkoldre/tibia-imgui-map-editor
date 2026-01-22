#pragma once
#include "Services/ClientDataService.h"
#include "Services/SecondaryClientData.h"
#include "Services/SpriteManager.h"
#include <functional>
#include <memory>
#include <spdlog/spdlog.h>

// Forward declarations
namespace MapEditor {
class AppStateManager;
namespace Rendering {
class RenderingManager;
}
namespace AppLogic {
class MapTabManager;
class MapOperationHandler;
class SessionLifecycleManager;
} // namespace AppLogic
namespace Presentation {
class WorkspaceController;
} // namespace Presentation
namespace UI {
class UnsavedChangesModal;
class PreferencesDialog;
} // namespace UI
} // namespace MapEditor

namespace MapEditor {

/**
 * Callbacks for external cleanup actions during version switch.
 * Allows ClientVersionManager to orchestrate without direct dependencies.
 */
struct VersionSwitchCallbacks {
  std::function<void()> on_release_renderer; // RenderingManager::release()
  std::function<void()> on_unbind_ui; // WorkspaceController::unbindSession()
  std::function<void()>
      on_clear_map_operations; // MapOperationHandler::setExistingResources(nullptr,...)
  std::function<void()> on_transition_to_welcome; // StateManager transition
  std::function<void()> on_notify_user;           // Toast notification
};

/**
 * Dependencies for building version switch callbacks.
 * Allows Application to pass pointers instead of lambda closures.
 */
struct VersionSwitchDependencies {
  Rendering::RenderingManager *rendering_manager = nullptr;
  Presentation::WorkspaceController *workspace_controller = nullptr;
  AppLogic::MapOperationHandler *map_operations = nullptr;
  UI::PreferencesDialog *preferences = nullptr;
  AppStateManager *state_manager = nullptr;
};

/**
 * Factory function to build standard version switch callbacks from
 * dependencies.
 */
VersionSwitchCallbacks
buildVersionSwitchCallbacks(const VersionSwitchDependencies &deps);

/**
 * Manages client data resources and version switching.
 * Owns: ClientDataService, SpriteManager, SecondaryClientData
 */
class ClientVersionManager {
public:
  // Resource ownership - take ownership of loaded resources
  void setClientData(std::unique_ptr<Services::ClientDataService> data);
  void setSpriteManager(std::unique_ptr<Services::SpriteManager> sprites);
  void
  setSecondaryClient(std::unique_ptr<Services::SecondaryClientData> secondary);

  // Accessors (non-owning pointers for external use)
  Services::ClientDataService *getClientData() const {
    return client_data_.get();
  }
  Services::SpriteManager *getSpriteManager() const {
    return sprite_manager_.get();
  }
  Services::SecondaryClientData *getSecondaryClient() const {
    return secondary_client_.get();
  }

  // Check if resources are loaded
  bool hasClientData() const { return client_data_ != nullptr; }
  bool hasSpriteManager() const { return sprite_manager_ != nullptr; }
  bool hasSecondaryClient() const { return secondary_client_ != nullptr; }

  /**
   * Update resources per frame.
   * Drives async loading and other resource maintenance tasks.
   */
  void update();

  // Clear secondary client
  void clearSecondaryClient();

  // Version switch - releases all resources
  void releaseAll();

  /**
   * Initiate client version switch.
   * If unsaved changes exist, shows modal and returns true (pending user
   * input). Otherwise performs switch immediately and returns false.
   * @return true if operation is pending user decision, false if completed
   */
  [[nodiscard]] bool
  initiateVersionSwitch(AppLogic::MapTabManager &tab_manager,
                        AppLogic::SessionLifecycleManager *lifecycle,
                        UI::UnsavedChangesModal &unsaved_modal,
                        AppLogic::MapOperationHandler &map_ops,
                        const VersionSwitchCallbacks &callbacks);

  /**
   * Perform the actual version switch cleanup.
   * Called directly when no unsaved changes, or after user confirms.
   */
  void performVersionSwitch(AppLogic::MapTabManager &tab_manager,
                            AppLogic::SessionLifecycleManager *lifecycle,
                            const VersionSwitchCallbacks &callbacks);

private:
  std::unique_ptr<Services::ClientDataService> client_data_;
  std::unique_ptr<Services::SpriteManager> sprite_manager_;
  std::unique_ptr<Services::SecondaryClientData> secondary_client_;
};

} // namespace MapEditor
