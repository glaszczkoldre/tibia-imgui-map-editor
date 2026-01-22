#include "CleanupController.h"
#include "../NotificationHelper.h"
#include "Rendering/Frame/RenderState.h"
#include <spdlog/spdlog.h>

namespace MapEditor::Presentation {

void CleanupController::requestCleanup(CleanupType type,
                                       UI::ConfirmationDialog *dialog) {
  if (!dialog)
    return;

  pending_ = type;

  const char *title = nullptr;
  const char *message = nullptr;

  switch (type) {
  case CleanupType::InvalidItems:
    title = "Remove Invalid Items";
    message = "This will remove all items with IDs that don't exist in the "
              "client data.\n\n"
              "WARNING: This action CANNOT be undone!";
    break;

  case CleanupType::HouseItems:
    title = "Remove House Items";
    message = "This will remove all moveable items from tiles that belong to "
              "houses.\n\n"
              "WARNING: This action CANNOT be undone!";
    break;

  case CleanupType::None:
    return;
  }

  if (title && message) {
    dialog->show(title, message, "Remove Items");
  }
}

void CleanupController::processResult(const Context &ctx) {
  if (!ctx.dialog || !ctx.tab_manager)
    return;

  auto result = ctx.dialog->render();

  if (result == UI::ConfirmationDialog::Result::Cancelled) {
    pending_ = CleanupType::None;
    return;
  }

  if (result != UI::ConfirmationDialog::Result::Confirmed)
    return;

  auto *session = ctx.tab_manager->getActiveSession();
  spdlog::info("Cleanup confirmed, session={}, map={}", (void *)session,
               session ? (void *)session->getMap() : nullptr);

  if (!session || !session->getMap()) {
    pending_ = CleanupType::None;
    return;
  }

  if (!ctx.client_data) {
    spdlog::error("No client_data_service for cleanup operation");
    pending_ = CleanupType::None;
    return;
  }

  Services::CleanupResult cleanup_result;

  switch (pending_) {
  case CleanupType::InvalidItems:
    spdlog::info("Running cleanInvalidItems...");
    cleanup_result = Services::MapCleanupService::cleanInvalidItems(
        *session->getMap(), *ctx.client_data);
    spdlog::info("cleanInvalidItems: processed {} tiles, removed {} items",
                 cleanup_result.tiles_processed, cleanup_result.items_removed);
    showSuccess("Removed " + std::to_string(cleanup_result.items_removed) +
                " invalid items from " +
                std::to_string(cleanup_result.tiles_processed) + " tiles");
    break;

  case CleanupType::HouseItems:
    spdlog::info("Running cleanHouseItems...");
    cleanup_result = Services::MapCleanupService::cleanHouseItems(
        *session->getMap(), *ctx.client_data);
    spdlog::info("cleanHouseItems: removed {} items",
                 cleanup_result.items_removed);
    showSuccess("Removed " + std::to_string(cleanup_result.items_removed) +
                " moveable items from house tiles");
    break;

  case CleanupType::None:
    spdlog::warn("No pending cleanup operation");
    break;
  }

  if (cleanup_result.items_removed > 0 || cleanup_result.tiles_removed > 0) {
    session->getMap()->markChanged();
    if (ctx.rendering_manager) {
      if (auto *state =
              ctx.rendering_manager->getRenderState(session->getID())) {
        state->invalidateAll();
      }
    }
  }

  pending_ = CleanupType::None;
}

} // namespace MapEditor::Presentation
