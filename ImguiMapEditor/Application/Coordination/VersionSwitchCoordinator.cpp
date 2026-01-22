#include "Application/Coordination/VersionSwitchCoordinator.h"
#include "Application/AppStateManager.h"
#include "Application/MapOperationHandler.h"
#include "Application/MapTabManager.h"
#include "Application/SessionLifecycleManager.h"
#include "Controllers/WorkspaceController.h"
#include "Rendering/Frame/RenderingManager.h"
#include "UI/Dialogs/UnsavedChangesModal.h"
#include "UI/PreferencesDialog.h"
#include "ImGuiNotify.hpp"

namespace MapEditor::Coordination {

VersionSwitchCoordinator::VersionSwitchCoordinator(Dependencies deps)
    : deps_(deps) {}

bool VersionSwitchCoordinator::initiateSwitch() {
  return deps_.version_manager.initiateVersionSwitch(
      deps_.tab_manager, &deps_.session_lifecycle, deps_.unsaved_changes,
      deps_.map_operations, buildCallbacks());
}

void VersionSwitchCoordinator::performSwitch() {
  deps_.version_manager.performVersionSwitch(
      deps_.tab_manager, &deps_.session_lifecycle, buildCallbacks());
}

VersionSwitchCallbacks VersionSwitchCoordinator::buildCallbacks() {
  return MapEditor::buildVersionSwitchCallbacks(
      {.rendering_manager = &deps_.rendering_manager,
       .workspace_controller = &deps_.workspace_controller,
       .map_operations = &deps_.map_operations,
       .preferences = &deps_.preferences,
       .state_manager = &deps_.state_manager});
}

} // namespace MapEditor::Coordination
