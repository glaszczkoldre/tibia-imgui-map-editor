#include "PaletteWindowManager.h"

#include "../../Domain/Palette/Palette.h"
#include "PaletteWindow.h"
#include "Services/AppSettings.h"

#include <spdlog/spdlog.h>

namespace MapEditor::UI {

using namespace Domain::Palette;

PaletteWindowManager::PaletteWindowManager() = default;
PaletteWindowManager::~PaletteWindowManager() = default;

void PaletteWindowManager::initialize(
    Services::ClientDataService *clientData,
    Services::SpriteManager *spriteManager,
    Brushes::BrushController *brushController,
    Domain::Tileset::TilesetRegistry &tilesetRegistry,
    Domain::Palette::PaletteRegistry &paletteRegistry,
    Services::AppSettings *appSettings) {
  clientData_ = clientData;
  spriteManager_ = spriteManager;
  brushController_ = brushController;
  tilesetRegistry_ = &tilesetRegistry;
  paletteRegistry_ = &paletteRegistry;
  if (appSettings) {
    appSettings_ = appSettings;
  }

  // Initialize any existing windows
  for (auto &[name, window] : paletteWindows_) {
    window->initialize(clientData_, spriteManager_, brushController_,
                       *tilesetRegistry_, *paletteRegistry_, appSettings_);
  }
}

void PaletteWindowManager::setAppSettings(Services::AppSettings *appSettings) {
  appSettings_ = appSettings;
}

void PaletteWindowManager::createPaletteWindow(const std::string &paletteName) {
  auto window = std::make_unique<PaletteWindow>(paletteName);

  if (clientData_ && spriteManager_ && brushController_ && tilesetRegistry_ &&
      paletteRegistry_) {
    window->initialize(clientData_, spriteManager_, brushController_,
                       *tilesetRegistry_, *paletteRegistry_, appSettings_);
  }

  paletteWindows_[paletteName] = std::move(window);
  spdlog::debug("[PaletteWindowManager] Created window for palette: {}",
                paletteName);
}

void PaletteWindowManager::openPaletteWindow(const std::string &paletteName) {
  auto it = paletteWindows_.find(paletteName);
  if (it == paletteWindows_.end()) {
    createPaletteWindow(paletteName);
    it = paletteWindows_.find(paletteName);
  }

  if (it != paletteWindows_.end()) {
    it->second->setVisible(true);
  }
}

void PaletteWindowManager::togglePaletteWindow(const std::string &paletteName) {
  auto it = paletteWindows_.find(paletteName);
  if (it == paletteWindows_.end()) {
    createPaletteWindow(paletteName);
    it = paletteWindows_.find(paletteName);
    if (it != paletteWindows_.end()) {
      it->second->setVisible(true);
    }
  } else {
    it->second->toggleVisible();
  }
}

bool PaletteWindowManager::isPaletteWindowVisible(
    const std::string &paletteName) const {
  auto it = paletteWindows_.find(paletteName);
  return it != paletteWindows_.end() && it->second->isVisible();
}

void PaletteWindowManager::renderAllWindows() {
  for (auto &[name, window] : paletteWindows_) {
    window->render();
  }
}

void PaletteWindowManager::saveState() {
  if (!appSettings_) {
    return;
  }

  // Save open palette names as comma-separated string
  std::string openPalettes;
  for (const auto &[name, window] : paletteWindows_) {
    if (window->isVisible()) {
      if (!openPalettes.empty()) {
        openPalettes += ",";
      }
      openPalettes += name;
    }
  }

  appSettings_->openPaletteNames = openPalettes;
  spdlog::debug("[PaletteWindowManager] Saved state: {}", openPalettes);
}

void PaletteWindowManager::restoreState() {
  if (!appSettings_) {
    return;
  }

  const std::string &openPalettes = appSettings_->openPaletteNames;
  if (openPalettes.empty()) {
    return;
  }

  spdlog::debug("[PaletteWindowManager] Restoring state: {}", openPalettes);

  // Parse comma-separated palette names
  size_t pos = 0;
  size_t prevPos = 0;
  while ((pos = openPalettes.find(',', prevPos)) != std::string::npos) {
    std::string name = openPalettes.substr(prevPos, pos - prevPos);
    if (!name.empty()) {
      openPaletteWindow(name);
    }
    prevPos = pos + 1;
  }
  // Last (or only) name
  if (prevPos < openPalettes.size()) {
    std::string name = openPalettes.substr(prevPos);
    if (!name.empty()) {
      openPaletteWindow(name);
    }
  }
}

} // namespace MapEditor::UI
