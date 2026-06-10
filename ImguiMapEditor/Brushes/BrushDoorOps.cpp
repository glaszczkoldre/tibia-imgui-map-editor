#include "BrushController.h"
#include "BrushRegistry.h"
#include "Brushes/Helpers/DoorResolveUtils.h"
#include "Domain/Item.h"
#include "Services/BrushSettingsService.h"
#include "Services/ClientDataService.h"
#include "Types/DoorBrush.h"
#include "Types/WallBrush.h"

namespace MapEditor::Brushes {

void BrushController::activateNormalDoorBrush() {
  if (normalDoorBrush_) {
    setBrush(normalDoorBrush_.get());
  }
}

void BrushController::activateNormalAltDoorBrush() {
  if (normalAltDoorBrush_) {
    setBrush(normalAltDoorBrush_.get());
  } else {
    activateNormalDoorBrush();
  }
}

void BrushController::activateLockedDoorBrush() {
  if (lockedDoorBrush_) {
    setBrush(lockedDoorBrush_.get());
  }
}

void BrushController::activateQuestDoorBrush() {
  if (questDoorBrush_) {
    setBrush(questDoorBrush_.get());
  }
}

void BrushController::activateMagicDoorBrush() {
  if (magicDoorBrush_) {
    setBrush(magicDoorBrush_.get());
  }
}

void BrushController::activateArchwayBrush() {
  if (archwayBrush_) {
    setBrush(archwayBrush_.get());
  }
}

void BrushController::activateWindowBrush() {
  if (windowBrush_) {
    setBrush(windowBrush_.get());
  }
}

void BrushController::activateHatchWindowBrush() {
  if (hatchWindowBrush_) {
    setBrush(hatchWindowBrush_.get());
  } else {
    activateWindowBrush();
  }
}

bool BrushController::canSwitchDoorAt(const Domain::Position &pos,
                                      const Domain::Item *preferredItem) const {
  if (!map_ || !registry_) {
    return false;
  }

  auto *tile = map_->getTile(pos);
  const auto resolved = resolveDoorTarget(tile, registry_, preferredItem);
  return resolved.wallBrush != nullptr && resolved.item != nullptr;
}

bool BrushController::switchDoorAt(const Domain::Position &pos,
                                   const Domain::Item *preferredItem) {
  if (!map_ || !historyManager_ || !registry_) {
    return false;
  }

  auto *tile = map_->getTile(pos);
  const auto resolved = resolveDoorTarget(tile, registry_, preferredItem);
  if (!tile || !resolved.wallBrush || !resolved.item) {
    return false;
  }

  historyManager_->beginOperation("Switch door",
                                  Domain::History::ActionType::Other, nullptr);
  historyManager_->recordTileBefore(pos, tile);

  const bool preferLocked =
      brushSettingsService_ && brushSettingsService_->getLockDoors();
  if (!resolved.wallBrush->switchDoor(*map_, *tile, resolved.item,
                                      preferLocked)) {
    historyManager_->cancelOperation();
    return false;
  }

  historyManager_->endOperation(map_, nullptr);
  return true;
}

std::optional<bool>
BrushController::getDoorOpenStateAt(const Domain::Position &pos,
                                    const Domain::Item *preferredItem) const {
  if (!map_ || !registry_) {
    return std::nullopt;
  }

  auto *tile = const_cast<Domain::Tile *>(map_->getTile(pos));
  const auto resolved = resolveDoorTarget(tile, registry_, preferredItem);
  if (!resolved.wallBrush || !resolved.item) {
    return std::nullopt;
  }

  if (const auto door = resolved.wallBrush->findDoorForItem(resolved.item->getServerId())) {
    return door->isOpen;
  }

  return std::nullopt;
}

bool BrushController::canRotateItemAt(const Domain::Position &pos,
                                      const Domain::Item *preferredItem) const {
  if (!map_) {
    return false;
  }

  const auto *tile = map_->getTile(pos);
  const auto *item = resolveTileItem(tile, preferredItem);
  const auto *type = item ? item->getType() : nullptr;
  return type && type->rotateTo != 0;
}

bool BrushController::rotateItemAt(const Domain::Position &pos,
                                   const Domain::Item *preferredItem) {
  if (!map_ || !historyManager_) {
    return false;
  }

  auto *tile = map_->getTile(pos);
  auto *item = resolveMutableTileItem(tile, preferredItem);
  const auto *type = item ? item->getType() : nullptr;
  if (!tile || !item || !type || type->rotateTo == 0) {
    return false;
  }

  historyManager_->beginOperation("Rotate item",
                                  Domain::History::ActionType::Other, nullptr);
  historyManager_->recordTileBefore(pos, tile);

  const auto rotatedId = type->rotateTo;
  item->setServerId(rotatedId);
  if (clientData_) {
    if (const auto *rotatedType = clientData_->getItemTypeByServerId(rotatedId)) {
      item->setType(rotatedType);
      item->setClientId(rotatedType->client_id);
    } else {
      item->setType(nullptr);
      item->setClientId(0);
    }
  }

  tile->markDirty();
  historyManager_->endOperation(map_, nullptr);
  return true;
}

DoorBrush *BrushController::getDoorBrushForType(DoorType type) const {
  switch (type) {
    case DoorType::Normal:
      return normalDoorBrush_.get();
    case DoorType::NormalAlt:
      return normalAltDoorBrush_ ? normalAltDoorBrush_.get()
                                 : normalDoorBrush_.get();
    case DoorType::Locked:
      return lockedDoorBrush_.get();
    case DoorType::Quest:
      return questDoorBrush_.get();
    case DoorType::Magic:
      return magicDoorBrush_.get();
    case DoorType::Archway:
      return archwayBrush_.get();
    case DoorType::Window:
      return windowBrush_.get();
    case DoorType::HatchWindow:
      return hatchWindowBrush_ ? hatchWindowBrush_.get() : windowBrush_.get();
    case DoorType::Undefined:
      return nullptr;
    }
}

} // namespace MapEditor::Brushes
