#include "BrushController.h"
#include "BrushRegistry.h"
#include "Services/BrushSettingsService.h"
#include "Types/DoodadBrush.h"
#include "Types/RawBrush.h"

namespace MapEditor::Brushes {

bool BrushController::applyBrush(const Domain::Position &pos) {
  return applyBrush(pos, 0);
}

bool BrushController::applyBrush(const Domain::Position &pos,
                                uint32_t modifiers) {
  if (!map_ || !historyManager_ || !currentBrush_) {
    return false;
  }

  strokeModifiers_ = modifiers;

  if (strokeActive_) {
    continueStroke(pos);
    return true;
  }

  historyManager_->beginOperation("Brush: " + currentBrushName_,
                                  Domain::History::ActionType::Draw, nullptr);
  paintedPositions_.clear();
  lastStrokePos_.reset();

  switch (getActionFamily()) {
  case BrushActionFamily::GroundLike:
    paintExpandedCenter(pos, modifiers);
    break;
  case BrushActionFamily::WallLike:
    if (currentBrush_->getType() == BrushType::Wall &&
        (modifiers & Modifiers::Alt) != 0 &&
        getBrushPositionsForCenter(pos).size() == 1) {
      paintRecordedPosition(pos, modifiers, true);
    } else {
      paintExpandedCenter(pos, modifiers);
    }
    break;
  case BrushActionFamily::DoorLike:
  case BrushActionFamily::PointLike:
    paintRecordedPosition(pos, modifiers);
    break;
  case BrushActionFamily::DoodadLike:
    paintDoodadRecordedPosition(pos, modifiers);
    break;
  }

  const bool changed = !paintedPositions_.empty();
  if (changed) {
    historyManager_->endOperation(map_, nullptr);
  } else {
    historyManager_->cancelOperation();
  }
  paintedPositions_.clear();
  lastStrokePos_.reset();

  return changed;
}

bool BrushController::eraseBrush(const Domain::Position &pos) {
  return eraseBrush(pos, 0);
}

bool BrushController::eraseBrush(const Domain::Position &pos,
                                 uint32_t modifiers) {
  if (!map_ || !historyManager_ || !currentBrush_) {
    return false;
  }

  Domain::Tile *tile = map_->getTile(pos);
  if (!tile) {
    return false;
  }

  historyManager_->beginOperation("Erase: " + currentBrushName_,
                                  Domain::History::ActionType::Delete, nullptr);
  strokeModifiers_ = modifiers;
  paintedPositions_.clear();
  lastStrokePos_.reset();

  eraseRecordedPosition(pos);

  const bool changed = !paintedPositions_.empty();
  if (changed) {
    historyManager_->endOperation(map_, nullptr);
  } else {
    historyManager_->cancelOperation();
  }

  paintedPositions_.clear();
  lastStrokePos_.reset();

  return changed;
}

bool BrushController::refreshCurrentBrush() {
  if (!currentBrush_) {
    return false;
  }

  return applyResolvedSelection(captureCurrentSelection());
}

void BrushController::cycleBrushVariation(int delta) {
  if (!currentBrush_ || delta == 0) {
    return;
  }

  const auto maxVariation = static_cast<int>(currentBrush_->getMaxVariation());
  if (maxVariation == 0) {
    variation_ = 0;
    return;
  }

  variation_ += delta;
  while (variation_ < 0) {
    variation_ += maxVariation;
  }
  while (variation_ >= maxVariation) {
    variation_ -= maxVariation;
  }

  currentBrush_->setVariation(static_cast<size_t>(variation_));
  refreshCurrentBrush();
}

void BrushController::setBrushVariation(int variation) {
  variation_ = std::max(0, variation);
  if (currentBrush_) {
    currentBrush_->setVariation(static_cast<size_t>(variation_));
  }
  refreshCurrentBrush();
}

void BrushController::setBrushThickness(float thickness) {
  if (auto *doodadBrush = dynamic_cast<DoodadBrush *>(currentBrush_)) {
    doodadBrush->setThickness(std::clamp(thickness, 0.0f, 1.0f));
    refreshCurrentBrush();
  }
}

float BrushController::getBrushThickness() const {
  if (const auto *doodadBrush = dynamic_cast<const DoodadBrush *>(currentBrush_)) {
    return doodadBrush->getThickness();
  }
  return 1.0f;
}

void BrushController::adjustBrushSize(int delta) {
  if (delta == 0) {
    return;
  }

  if (brushSettingsService_) {
    brushSettingsService_->setStandardSize(
        brushSettingsService_->getStandardSize() + delta);
    return;
  }

  setBrushSize(brushSize_ + delta);
}

bool BrushController::storeBrushSlot(size_t slot) {
  if (slot >= brushHotkeys_.size() || !currentBrush_) {
    return false;
  }

  brushHotkeys_[slot] = captureCurrentSelection();
  return true;
}

bool BrushController::recallBrushSlot(size_t slot) {
  if (slot >= brushHotkeys_.size() || !brushHotkeys_[slot].has_value()) {
    return false;
  }

  return applyResolvedSelection(*brushHotkeys_[slot]);
}

bool BrushController::usesPreciseMutationNotifications() const {
  if (!currentBrush_ || !onTilesMutated_) {
    return false;
  }

  switch (currentBrush_->getType()) {
  case BrushType::Ground:
  case BrushType::Wall:
  case BrushType::WallDecoration:
  case BrushType::Table:
  case BrushType::Carpet:
  case BrushType::Doodad:
    return true;
  case BrushType::OptionalBorder:
  case BrushType::Flag:
  case BrushType::Eraser:
  case BrushType::Door:
  case BrushType::Raw:
  case BrushType::Creature:
  case BrushType::Spawn:
  case BrushType::House:
  case BrushType::HouseExit:
  case BrushType::Waypoint:
  case BrushType::Placeholder:
    return false;
  }
}

} // namespace MapEditor::Brushes
