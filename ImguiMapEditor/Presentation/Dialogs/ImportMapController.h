#pragma once
#include "Services/MapMergeService.h"
#include "Application/MapTabManager.h"
#include "IO/Otbm/OtbmReader.h"
#include "ImGuiNotify.hpp"
#include "Rendering/Frame/RenderingManager.h"
#include "Services/ClientDataService.h"
#include "UI/Dialogs/Import/ImportMapDialog.h"
#include <spdlog/spdlog.h>

namespace MapEditor::Presentation {

/**
 * Handles ImportMapDialog result processing.
 * Extracts import logic from Application::render() for better separation of
 * concerns.
 */
class ImportMapController {
public:
  struct Context {
    AppLogic::MapTabManager *tab_manager = nullptr;
    Services::ClientDataService *client_data = nullptr;
    Rendering::RenderingManager *rendering_manager = nullptr;
    UI::ImportMapDialog *dialog = nullptr;
  };

  /**
   * Process dialog result and perform map import if confirmed.
   */
  void processResult(const Context &ctx);

private:
  AppLogic::MapMergeService merge_service_;
};

} // namespace MapEditor::Presentation
