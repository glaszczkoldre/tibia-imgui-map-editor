#include "ImportMapController.h"
#include "../NotificationHelper.h"
#include "Rendering/Frame/RenderState.h"

namespace MapEditor::Presentation {

void ImportMapController::processResult(const Context &ctx) {
  if (!ctx.dialog || !ctx.tab_manager)
    return;

  auto result = ctx.dialog->render();
  if (result != UI::ImportMapDialog::Result::Confirmed)
    return;

  const auto &options = ctx.dialog->getOptions();
  auto *import_session = ctx.tab_manager->getActiveSession();

  if (!import_session) {
    spdlog::warn("Import failed: no active session");
    return;
  }

  auto read_result = IO::OtbmReader::read(options.source_path, ctx.client_data);

  if (!read_result.success) {
    showError(std::string("Failed to read source map: ") + read_result.error);
    return;
  }

  AppLogic::MapMergeService::MergeOptions merge_opts;
  merge_opts.offset = options.offset;
  merge_opts.overwrite_existing = options.overwrite_existing;

  auto merge_result =
      merge_service_.merge(*import_session, *read_result.map, merge_opts);

  if (merge_result.success) {
    showSuccess("Imported " + std::to_string(merge_result.tiles_merged) +
                " tiles");
    if (ctx.rendering_manager && import_session) {
      if (auto *state =
              ctx.rendering_manager->getRenderState(import_session->getID())) {
        state->invalidateAll();
      }
    }
  } else {
    showError(std::string("Import failed: ") + merge_result.error);
  }
}

} // namespace MapEditor::Presentation
