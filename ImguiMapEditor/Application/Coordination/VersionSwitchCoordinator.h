#pragma once

#include "Application/ClientVersionManager.h"
#include <memory>

// Forward declarations
namespace MapEditor {
class AppStateManager;
namespace AppLogic {
class MapOperationHandler;
class MapTabManager;
class SessionLifecycleManager;
} // namespace AppLogic
namespace Rendering {
class RenderingManager;
}
namespace Presentation {
class WorkspaceController;
}
namespace UI {
class PreferencesDialog;
class UnsavedChangesModal;
} // namespace UI
} // namespace MapEditor

namespace MapEditor::Coordination {

/**
 * Coordinates the process of switching client versions.
 * Wraps ClientVersionManager and provides the necessary dependencies and
 * callbacks from the Application layer.
 */
class VersionSwitchCoordinator {
public:
  struct Dependencies {
    ClientVersionManager &version_manager;
    AppLogic::MapTabManager &tab_manager;
    AppLogic::SessionLifecycleManager &session_lifecycle;
    Rendering::RenderingManager &rendering_manager;
    AppLogic::MapOperationHandler &map_operations;
    Presentation::WorkspaceController &workspace_controller;
    AppStateManager &state_manager;
    UI::PreferencesDialog &preferences;
    UI::UnsavedChangesModal &unsaved_changes;
  };

  explicit VersionSwitchCoordinator(Dependencies deps);

  /**
   * Initiate the version switch process.
   * If unsaved changes exist, shows a modal.
   * If no unsaved changes, performs the switch immediately.
   * @return true if the operation is pending (waiting for user input), false if
   * completed.
   */
  bool initiateSwitch();

  /**
   * Perform the switch immediately (e.g. after user confirmation).
   */
  void performSwitch();

private:
  Dependencies deps_;

  /**
   * Build the callbacks structure required by ClientVersionManager.
   */
  VersionSwitchCallbacks buildCallbacks();
};

} // namespace MapEditor::Coordination
