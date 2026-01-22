#pragma once
#include "Application/MapTabManager.h"
#include "ImGuiNotify.hpp"
#include "Rendering/Frame/RenderingManager.h"
#include "Services/ClientDataService.h"
#include "Services/Map/MapCleanupService.h"
#include "UI/Dialogs/ConfirmationDialog.h"
#include <spdlog/spdlog.h>

namespace MapEditor::Presentation {

/**
 * Handles cleanup confirmation dialog result processing.
 * Manages pending cleanup state and executes cleanup services.
 */
class CleanupController {
public:
  enum class CleanupType { None, InvalidItems, HouseItems };

  struct Context {
    AppLogic::MapTabManager *tab_manager = nullptr;
    Services::ClientDataService *client_data = nullptr;
    Rendering::RenderingManager *rendering_manager = nullptr;
    UI::ConfirmationDialog *dialog = nullptr;
  };

  /**
   * Request a cleanup operation.
   * Configures the pending state and shows the confirmation dialog with
   * appropriate text.
   */
  void requestCleanup(CleanupType type, UI::ConfirmationDialog *dialog);

  /**
   * Set the pending cleanup operation type directly (internal use).
   */
  void setPendingCleanup(CleanupType type) { pending_ = type; }

  /**
   * Get the current pending cleanup type.
   */
  CleanupType getPendingCleanup() const { return pending_; }

  /**
   * Process dialog result and execute cleanup if confirmed.
   */
  void processResult(const Context &ctx);

private:
  CleanupType pending_ = CleanupType::None;
};

} // namespace MapEditor::Presentation
