#include "BrushSystem.h"
#include "Services/ConfigService.h"
#include <filesystem>
#include <spdlog/spdlog.h>

namespace MapEditor {
namespace Brushes {

BrushSystem::BrushSystem()
    : tileset_service_(registry_),
      brush_size_panel_(&settings_service_, &controller_,
                        [this]() { saveBrushes(); }) {
  // Wire the settings service to the controller for multi-tile painting
  controller_.setBrushSettingsService(&settings_service_);
  controller_.setBrushRegistry(&registry_);
  // Wire the preview factory for creating preview providers
  controller_.setPreviewFactory(&preview_factory_);
}

BrushSystem::~BrushSystem() noexcept = default;

void BrushSystem::setConfigService(Services::ConfigService *configService) {
  if (!configService)
    return;
  config_service_ = configService;
  settings_service_.loadFromConfig(*config_service_);

  // Get brush file path from config directory
  // ConfigService stores at: %APPDATA%/TibiaMapEditor/config.json
  // We store brushes at: %APPDATA%/TibiaMapEditor/custom_brushes.json
  auto configPath = std::filesystem::path("."); // Will be overridden

  // Try to get path from getImGuiIniPath (same directory)
  const auto &iniPath = configService->getImGuiIniPath();
  if (!iniPath.empty()) {
    configPath = std::filesystem::path(iniPath).parent_path();
  } else {
    spdlog::warn("ImGui INI path is empty, custom brushes will be saved to "
                 "current directory");
  }

  brushPath_ = (configPath / "custom_brushes.json").string();

  // Load existing brushes
  settings_service_.loadCustomBrushes(brushPath_);
}

void BrushSystem::saveBrushes() {
  if (!brushPath_.empty()) {
    settings_service_.saveCustomBrushes(brushPath_);
  }
}

void BrushSystem::saveSettings() {
  if (config_service_) {
    settings_service_.saveToConfig(*config_service_);
  }
}

} // namespace Brushes
} // namespace MapEditor
