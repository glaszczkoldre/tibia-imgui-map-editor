#include "BrushController.h"
#include "BrushRegistry.h"
#include "Domain/Item.h"
#include "Services/BrushSettingsService.h"
#include "Services/Autoborder/AutoborderEngine.h"
#include "Services/Autoborder/PlannedMutation.h"
#include "Services/ClientDataService.h"
#include "Services/Preview/BrushPreviewFactory.h"
#include "Services/Preview/PreviewService.h"
#include "Types/GroundBrush.h"
#include "Types/DoodadBrush.h"
#include "Types/DoodadPlacementPlanner.h"
#include "Types/WallBrush.h"
#include "Types/DoorBrush.h"
#include "Types/RawBrush.h"
#include <cmath>
#include <spdlog/spdlog.h>

namespace MapEditor::Brushes {



BrushController::~BrushController() noexcept = default;

void BrushController::initialize(
    Domain::ChunkedMap *map, Domain::History::HistoryManager *historyManager,
    Services::ClientDataService *clientData) {
  map_ = map;
  historyManager_ = historyManager;
  clientData_ = clientData;
  spdlog::debug("[BrushController] Initialized with map, history manager, and "
                "client data");
}

void BrushController::setBrushRegistry(BrushRegistry *registry) {
  registry_ = registry;
  if (!registry_) {
    normalDoorBrush_.reset();
    normalAltDoorBrush_.reset();
    lockedDoorBrush_.reset();
    questDoorBrush_.reset();
    magicDoorBrush_.reset();
    archwayBrush_.reset();
    windowBrush_.reset();
    hatchWindowBrush_.reset();
    return;
  }

  registry_->registerExternalBrush(&spawnBrush_);
  registry_->registerExternalBrush(&pzBrush_);
  registry_->registerExternalBrush(&noPvpBrush_);
  registry_->registerExternalBrush(&noLogoutBrush_);
  registry_->registerExternalBrush(&pvpZoneBrush_);
  registry_->registerExternalBrush(&eraserBrush_);
  registry_->registerExternalBrush(&houseBrush_);
  registry_->registerExternalBrush(&houseExitBrush_);
  registry_->registerExternalBrush(&waypointBrush_);
  registry_->registerExternalBrush(&optionalBorderBrush_);
  optionalBorderBrush_.setBrushRegistry(registry_);

  normalDoorBrush_ = std::make_unique<DoorBrush>("Normal Door", 0,
                                                  DoorType::Normal, *registry_);
  normalAltDoorBrush_ = std::make_unique<DoorBrush>(
      "Normal Alt Door", 0, DoorType::NormalAlt, *registry_);
  lockedDoorBrush_ = std::make_unique<DoorBrush>("Locked Door", 0,
                                                  DoorType::Locked, *registry_);
  questDoorBrush_ = std::make_unique<DoorBrush>("Quest Door", 0,
                                                 DoorType::Quest, *registry_);
  magicDoorBrush_ = std::make_unique<DoorBrush>("Magic Door", 0,
                                                 DoorType::Magic, *registry_);
  archwayBrush_ = std::make_unique<DoorBrush>("Archway", 0,
                                               DoorType::Archway, *registry_);
  windowBrush_ = std::make_unique<DoorBrush>("Window", 0,
                                              DoorType::Window, *registry_);
  hatchWindowBrush_ = std::make_unique<DoorBrush>("Hatch Window", 0,
                                                   DoorType::HatchWindow,
                                                   *registry_);
  registry_->registerExternalBrush(normalDoorBrush_.get());
  registry_->registerExternalBrush(normalAltDoorBrush_.get());
  registry_->registerExternalBrush(lockedDoorBrush_.get());
  registry_->registerExternalBrush(questDoorBrush_.get());
  registry_->registerExternalBrush(magicDoorBrush_.get());
  registry_->registerExternalBrush(archwayBrush_.get());
  registry_->registerExternalBrush(windowBrush_.get());
  registry_->registerExternalBrush(hatchWindowBrush_.get());
}

void BrushController::setBrush(IBrush *brush) {
  if (!brush) {
    clearBrush();
    return;
  }

  currentBrush_ = brush;
  currentBrushName_ = brush->getName();
  const auto maxVar = static_cast<int>(currentBrush_->getMaxVariation());
  if (variation_ < 0 || variation_ >= maxVar) {
    variation_ = maxVar > 0 ? maxVar - 1 : 0;
  }
  currentBrush_->setVariation(static_cast<size_t>(variation_));
  lastBrushSelection_ = captureCurrentSelection();

  refreshPreviewProvider();

  if (onBrushActivated_) {
    onBrushActivated_();
  }

  spdlog::info("[BrushController] Set brush: {}", brush->getName());
}

void BrushController::refreshPreviewProvider() {
  if (!previewService_) {
    return;
  }

  if (!currentBrush_) {
    previewService_->clearPreview();
    return;
  }

  if (!previewFactory_) {
    previewService_->clearPreview();
    spdlog::warn("[BrushController] No preview factory available");
    return;
  }

  auto provider =
      previewFactory_->createProvider(currentBrush_, brushSettingsService_, map_);
  if (provider) {
    previewService_->setProvider(std::move(provider));
  } else {
    previewService_->clearPreview();
  }
}

void BrushController::clearBrush() {
  if (currentBrush_) {
    lastBrushSelection_ = captureCurrentSelection();
  }

  currentBrush_ = nullptr;
  currentBrushName_.clear();

  if (previewService_) {
    previewService_->clearPreview();
  }

  spdlog::debug("[BrushController] Brush cleared");
}

bool BrushController::restoreLastBrush() {
  return lastBrushSelection_.has_value() &&
         applyResolvedSelection(*lastBrushSelection_);
}

bool BrushController::toggleSelectionTool() {
  if (hasBrush()) {
    clearBrush();
    return true;
  }

  return restoreLastBrush();
}

void BrushController::activateSpawnBrush() {
  setBrush(&spawnBrush_);
  spdlog::info("[BrushController] Spawn brush activated");
}

std::optional<uint32_t> BrushController::getCurrentItemId() const {
  if (auto *rawBrush = dynamic_cast<RawBrush *>(currentBrush_)) {
    return rawBrush->getItemId();
  }
  return std::nullopt;
}

bool BrushController::selectBrushFromTile(const Domain::Tile &tile,
                                          BrushPickMode mode,
                                          const Domain::Item *preferredItem) {
  const auto selection = resolveBrushFromTile(tile, mode, preferredItem);
  return selection.has_value() && applyResolvedSelection(*selection);
}



bool BrushController::applyResolvedSelection(
    const ResolvedBrushSelection &selection) {
  if (selection.rawItemId.has_value()) {
    if (!registry_) {
      return false;
    }

    if (auto *rawBrush = registry_->getOrCreateRAWBrush(*selection.rawItemId)) {
      variation_ = 0;
      rawBrush->setVariation(0);
      setBrush(rawBrush);
      return true;
    }
  }

  if (!selection.brush) {
    return false;
  }

  if (selection.brush == &houseBrush_ && selection.houseId.has_value()) {
    houseBrush_.setHouseId(*selection.houseId);
  }

  if (selection.brush == &houseExitBrush_ &&
      selection.houseExitHouseId.has_value()) {
    houseExitBrush_.setHouseId(*selection.houseExitHouseId);
  }

  if (selection.brush == &waypointBrush_ && selection.waypointName.has_value()) {
    waypointBrush_.setWaypointName(*selection.waypointName);
  }

  variation_ = std::max(0, selection.variation);
  if (selection.brush) {
    selection.brush->setVariation(static_cast<size_t>(variation_));
  }

  setBrush(selection.brush);
  return true;
}

ResolvedBrushSelection BrushController::captureCurrentSelection() const {
  ResolvedBrushSelection selection;
  selection.brush = currentBrush_;
  selection.displayName = currentBrush_ ? currentBrush_->getName() : std::string{};
  selection.variation = variation_;

  if (!currentBrush_) {
    return selection;
  }

  if (const auto *rawBrush = dynamic_cast<const RawBrush *>(currentBrush_)) {
    selection.rawItemId = rawBrush->getItemId();
  }

  if (currentBrush_ == &houseBrush_) {
    selection.houseId = houseBrush_.getHouseId();
  }

  if (currentBrush_ == &houseExitBrush_) {
    selection.houseExitHouseId = houseExitBrush_.getHouseId();
  }

  if (currentBrush_ == &waypointBrush_) {
    selection.waypointName = waypointBrush_.getWaypointName();
  }

  return selection;
}

} // namespace MapEditor::Brushes
