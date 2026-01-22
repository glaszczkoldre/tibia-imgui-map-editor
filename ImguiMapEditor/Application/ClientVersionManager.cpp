#include "ClientVersionManager.h"
#include "Application/AppStateManager.h"
#include "Controllers/WorkspaceController.h"
#include "ImGuiNotify.hpp"
#include "MapOperationHandler.h"
#include "MapTabManager.h"
#include "Rendering/Frame/RenderingManager.h"
#include "SessionLifecycleManager.h"
#include "UI/Dialogs/UnsavedChangesModal.h"
#include "UI/PreferencesDialog.h"

namespace MapEditor {

VersionSwitchCallbacks
buildVersionSwitchCallbacks(const VersionSwitchDependencies &deps) {
  return {.on_release_renderer =
              [rm = deps.rendering_manager]() {
                if (rm)
                  rm->release();
              },
          .on_unbind_ui =
              [wc = deps.workspace_controller, prefs = deps.preferences]() {
                if (wc) {
                  wc->unbindSession();
                }
                if (prefs) {
                  prefs->setSecondaryClientProvider(nullptr);
                }
              },
          .on_clear_map_operations =
              [ops = deps.map_operations]() {
                if (ops) {
                  ops->setExistingResources(nullptr, nullptr);
                }
              },
          .on_transition_to_welcome =
              [sm = deps.state_manager]() {
                if (sm) {
                  sm->transition(AppStateManager::State::Startup);
                }
              },
          .on_notify_user =
              []() {
                ImGui::InsertNotification(
                    {ImGuiToastType::Info, 2000, "Ready to open new map"});
              }};
}

void ClientVersionManager::setClientData(
    std::unique_ptr<Services::ClientDataService> data) {
  client_data_ = std::move(data);
  spdlog::info("ClientVersionManager: Client data set ({})",
               client_data_ ? "valid" : "null");
}

void ClientVersionManager::setSpriteManager(
    std::unique_ptr<Services::SpriteManager> sprites) {
  sprite_manager_ = std::move(sprites);
  spdlog::info("ClientVersionManager: Sprite manager set ({})",
               sprite_manager_ ? "valid" : "null");
}

void ClientVersionManager::setSecondaryClient(
    std::unique_ptr<Services::SecondaryClientData> secondary) {
  secondary_client_ = std::move(secondary);
  spdlog::info("ClientVersionManager: Secondary client set ({})",
               secondary_client_ ? "valid" : "null");
}

void ClientVersionManager::update() {
  // Drive async sprite loading
  if (sprite_manager_) {
    sprite_manager_->processAsyncLoads();
  }
}

void ClientVersionManager::clearSecondaryClient() {
  if (secondary_client_) {
    spdlog::info("ClientVersionManager: Clearing secondary client");
    secondary_client_->clear();
    secondary_client_.reset();
  }
}

void ClientVersionManager::releaseAll() {
  spdlog::info("ClientVersionManager: Releasing all resources");

  // Release in reverse order of typical usage
  clearSecondaryClient();

  // Destroy client data first, as it contains ItemTypes that hold raw pointers
  // (cached_sprite_region) to resources owned by SpriteManager (AtlasManager).
  // This ensures ItemTypes are destroyed before the memory they point to is invalid.
  if (client_data_) {
    client_data_.reset();
  }

  if (sprite_manager_) {
    sprite_manager_.reset();
  }

  spdlog::info("ClientVersionManager: All resources released");
}

bool ClientVersionManager::initiateVersionSwitch(
    AppLogic::MapTabManager &tab_manager,
    AppLogic::SessionLifecycleManager *lifecycle,
    UI::UnsavedChangesModal &unsaved_modal,
    AppLogic::MapOperationHandler &map_ops,
    const VersionSwitchCallbacks &callbacks) {
  // Check for unsaved changes
  if (tab_manager.hasUnsavedChanges()) {
    // Set up modal with save callback
    unsaved_modal.setSaveCallback(
        [&map_ops]() { map_ops.handleSaveAllMaps(); });

    // Show the modal - user will choose Save/Discard/Cancel
    unsaved_modal.show("All open maps");
    return true; // Pending user decision
  }

  // No unsaved changes - do the version switch immediately
  performVersionSwitch(tab_manager, lifecycle, callbacks);
  return false; // Switch completed
}

void ClientVersionManager::performVersionSwitch(
    AppLogic::MapTabManager &tab_manager,
    AppLogic::SessionLifecycleManager *lifecycle,
    const VersionSwitchCallbacks &callbacks) {
  spdlog::info("Switching client version...");

  // Close all tabs by moving them to the deferred destruction queue
  if (lifecycle) {
    lifecycle->queueSessionsForDestruction(tab_manager.extractAllSessions());
  }

  // Execute cleanup callbacks in order
  if (callbacks.on_release_renderer) {
    callbacks.on_release_renderer();
  }

  // Release all owned resources
  releaseAll();

  if (callbacks.on_unbind_ui) {
    callbacks.on_unbind_ui();
  }

  if (callbacks.on_clear_map_operations) {
    callbacks.on_clear_map_operations();
  }

  if (callbacks.on_transition_to_welcome) {
    callbacks.on_transition_to_welcome();
  }

  spdlog::info("Client version resources unloaded, ready for new version");

  if (callbacks.on_notify_user) {
    callbacks.on_notify_user();
  }
}

} // namespace MapEditor
