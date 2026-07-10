#include "BrushSystem.h"
#include "Services/ConfigService.h"

namespace MapEditor {
namespace Brushes {

BrushSystem::BrushSystem()
    : tileset_service_(registry_) {
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
}

void BrushSystem::saveSettings() {
  if (config_service_) {
    settings_service_.saveToConfig(*config_service_);
  }
}

} // namespace Brushes
} // namespace MapEditor
