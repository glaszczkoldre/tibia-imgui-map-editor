#include "BrushController.h"
#include "BrushRegistry.h"
#include "Services/Autoborder/AutoborderEngine.h"
#include "Services/Autoborder/PlannedMutation.h"
#include "Services/BrushSettingsService.h"
#include "Services/Preview/PreviewService.h"
#include "Types/DoodadBrush.h"
#include "Types/DoodadPlacementPlanner.h"
#include "Types/WallBrush.h"
#include <spdlog/spdlog.h>

namespace MapEditor::Brushes {

void BrushController::beginStroke() {
  beginStroke(0);
}

void BrushController::beginStroke(uint32_t modifiers) {
  beginStroke(modifiers, false);
}

void BrushController::beginStroke(uint32_t modifiers, bool eraseMode) {
  if (!historyManager_ || !currentBrush_)
    return;

  altReplaceState_ = {};

  historyManager_->beginOperation(
      (eraseMode ? "Erase: " : "Brush: ") + currentBrushName_,
      eraseMode ? Domain::History::ActionType::Delete
                : Domain::History::ActionType::Draw,
      nullptr);

  strokeActive_ = true;
  strokeEraseMode_ = eraseMode;
  strokeModifiers_ = modifiers;
  paintedPositions_.clear();
  lastStrokePos_.reset();
  spdlog::debug("[BrushController] Started {} stroke",
                eraseMode ? "erase" : "draw");
}

DrawContext BrushController::createDrawContext(uint32_t modifiers,
                                               bool specialAction) const {
  DrawContext ctx;
  ctx.variation = variation_;
  ctx.modifiers = modifiers;
  ctx.isDragging = strokeActive_;
  ctx.specialAction = specialAction;
  ctx.forcePlace = (modifiers & Modifiers::Alt) != 0;
  ctx.brushSettings = brushSettingsService_;
  ctx.clientData = clientData_;
  ctx.brushRegistry = registry_;
  ctx.ownerBrushId =
      registry_ ? registry_->getBrushId(currentBrush_) : InvalidBrushId;
  ctx.altReplace = &altReplaceState_;
  return ctx;
}

void BrushController::paintTileDirect(const Domain::Position &pos,
                                      uint32_t modifiers,
                                      bool specialAction) {
  if (!map_ || !currentBrush_)
    return;

  if (!currentBrush_->canDraw(*map_, pos)) {
    return;
  }

  Domain::Tile *tile = map_->getOrCreateTile(pos);
  if (!tile)
    return;

  const auto ctx = createDrawContext(modifiers, specialAction);
  currentBrush_->draw(*map_, tile, ctx);
}

void BrushController::notifyTilesMutated(
    const std::vector<Domain::Position> &positions) const {
  if (onTilesMutated_ && !positions.empty()) {
    onTilesMutated_(positions);
  }
}

void BrushController::continueStroke(const Domain::Position &pos) {
  if (!strokeActive_ || !historyManager_ || !currentBrush_)
    return;

  switch (getActionFamily()) {
  case BrushActionFamily::GroundLike:
    continueGroundLikeStroke(pos);
    break;
  case BrushActionFamily::WallLike:
    continueWallLikeStroke(pos);
    break;
  case BrushActionFamily::DoorLike:
    continueDoorLikeStroke(pos);
    break;
  case BrushActionFamily::DoodadLike:
    continueDoodadLikeStroke(pos);
    break;
  case BrushActionFamily::PointLike:
    continuePointLikeStroke(pos);
    break;
  }
}

BrushController::BrushActionFamily BrushController::getActionFamily() const {
  if (!currentBrush_) {
    return BrushActionFamily::PointLike;
  }

  switch (currentBrush_->getType()) {
  case BrushType::Ground:
  case BrushType::OptionalBorder:
  case BrushType::Flag:
  case BrushType::Eraser:
    return BrushActionFamily::GroundLike;
  case BrushType::Wall:
  case BrushType::WallDecoration:
  case BrushType::Table:
  case BrushType::Carpet:
    return BrushActionFamily::WallLike;
  case BrushType::Door:
    return BrushActionFamily::DoorLike;
  case BrushType::Doodad:
    return BrushActionFamily::DoodadLike;
  case BrushType::Raw:
  case BrushType::Creature:
  case BrushType::Spawn:
  case BrushType::House:
  case BrushType::HouseExit:
  case BrushType::Waypoint:
  case BrushType::Placeholder:
    return BrushActionFamily::PointLike;
  }

  return BrushActionFamily::PointLike;
}

std::vector<Domain::Position>
BrushController::getBrushPositionsForCenter(const Domain::Position &center) const {
  if (!currentBrush_ || !brushSettingsService_) {
    return {center};
  }

  switch (getActionFamily()) {
  case BrushActionFamily::DoorLike:
  case BrushActionFamily::PointLike:
  case BrushActionFamily::DoodadLike:
    return {center};
  case BrushActionFamily::WallLike: {
    const bool forceSquare = currentBrush_ && (currentBrush_->getType() == BrushType::Spawn);
    auto allPositions = brushSettingsService_->getBrushPositions(center, forceSquare);
    if (allPositions.size() <= 1) {
      return allPositions;
    }

    if (currentBrush_ && (currentBrush_->getType() == BrushType::Carpet ||
                          currentBrush_->getType() == BrushType::Table)) {
      return allPositions;
    }

    auto allOffsets = brushSettingsService_->getBrushOffsets(forceSquare);

    auto hasOffset = [&](int dx, int dy) {
      for (const auto &[ox, oy] : allOffsets) {
        if (ox == dx && oy == dy)
          return true;
      }
      return false;
    };

    auto isPerimeter = [&](int dx, int dy) {
      return !hasOffset(dx - 1, dy) || !hasOffset(dx + 1, dy) ||
             !hasOffset(dx, dy - 1) || !hasOffset(dx, dy + 1);
    };

    std::vector<Domain::Position> perimeterPositions;
    for (const auto &[dx, dy] : allOffsets) {
      if (isPerimeter(dx, dy)) {
        perimeterPositions.emplace_back(center.x + dx, center.y + dy,
                                        center.z);
      }
    }
    return perimeterPositions.empty()
               ? std::vector<Domain::Position>{center}
               : perimeterPositions;
  }
  case BrushActionFamily::GroundLike:
    break;
  }

  const bool forceSquare = currentBrush_ && (currentBrush_->getType() == BrushType::Spawn);
  return brushSettingsService_->getBrushPositions(center, forceSquare);
}

std::vector<Domain::Position> BrushController::getPaintedStrokePositions() const {
  std::vector<Domain::Position> positions;
  positions.reserve(paintedPositions_.size());
  for (const auto &key : paintedPositions_) {
    positions.emplace_back(std::get<0>(key), std::get<1>(key),
                           static_cast<int16_t>(std::get<2>(key)));
  }
  std::sort(positions.begin(), positions.end());
  return positions;
}

bool BrushController::paintRecordedPosition(const Domain::Position &pos,
                                            uint32_t modifiers,
                                            bool specialAction) {
  if (!map_ || !historyManager_) {
    return false;
  }

  auto key = std::make_tuple(pos.x, pos.y, pos.z);
  if (paintedPositions_.contains(key)) {
    return false;
  }

  Services::Autoborder::PlacementIntent intent;
  intent.brush = currentBrush_;
  intent.context = createDrawContext(modifiers, specialAction);
  intent.mode = Services::Autoborder::PlacementMode::Draw;
  intent.positions = {pos};
  std::vector<Domain::Position> changedPositions;
  const auto plannedResult = Services::Autoborder::applyPlannedIntentWithHistory(
      autoborderEngine_, *map_, *historyManager_, intent, &changedPositions);
  if (plannedResult != Services::Autoborder::PlannedMutationResult::Unsupported) {
    if (plannedResult != Services::Autoborder::PlannedMutationResult::Applied) {
      return false;
    }
    paintedPositions_.insert(key);
    notifyTilesMutated(changedPositions);
    return true;
  }

  paintedPositions_.insert(key);
  historyManager_->recordTileBefore(pos, map_->getTile(pos));
  paintTileDirect(pos, modifiers, specialAction);
  return true;
}

bool BrushController::paintRecordedPositions(
    std::span<const Domain::Position> positions, uint32_t modifiers,
    bool specialAction) {
  if (positions.empty() || !map_ || !historyManager_ || !currentBrush_) {
    return false;
  }

  std::vector<Domain::Position> deduped;
  deduped.reserve(positions.size());
  bool anyNew = false;
  for (const auto &pos : positions) {
    if (paintedPositions_.insert(std::make_tuple(pos.x, pos.y, pos.z)).second) {
      deduped.push_back(pos);
      anyNew = true;
    }
  }
  if (!anyNew) {
    return false;
  }

  Services::Autoborder::PlacementIntent intent;
  intent.brush = currentBrush_;
  intent.context = createDrawContext(modifiers, specialAction);
  intent.mode = Services::Autoborder::PlacementMode::Draw;
  intent.positions = std::move(deduped);
  std::vector<Domain::Position> changedPositions;
  const auto plannedResult = Services::Autoborder::applyPlannedIntentWithHistory(
      autoborderEngine_, *map_, *historyManager_, intent, &changedPositions);
  if (plannedResult == Services::Autoborder::PlannedMutationResult::Applied) {
    notifyTilesMutated(changedPositions);
    return true;
  }

  if (plannedResult == Services::Autoborder::PlannedMutationResult::Unsupported) {
    for (const auto &pos : intent.positions) {
      historyManager_->recordTileBefore(pos, map_->getTile(pos));
      paintTileDirect(pos, modifiers, specialAction);
    }
    return true;
  }

  return false;
}

void BrushController::eraseRecordedPosition(const Domain::Position &pos) {
  if (!map_ || !historyManager_ || !currentBrush_) {
    return;
  }

  auto key = std::make_tuple(pos.x, pos.y, pos.z);
  if (paintedPositions_.contains(key)) {
    return;
  }

  Services::Autoborder::PlacementIntent intent;
  intent.brush = currentBrush_;
  intent.context = createDrawContext(strokeModifiers_);
  intent.mode = Services::Autoborder::PlacementMode::Erase;
  intent.positions = {pos};
  std::vector<Domain::Position> changedPositions;
  const auto plannedResult = Services::Autoborder::applyPlannedIntentWithHistory(
      autoborderEngine_, *map_, *historyManager_, intent, &changedPositions);
  if (plannedResult != Services::Autoborder::PlannedMutationResult::Unsupported) {
    if (plannedResult != Services::Autoborder::PlannedMutationResult::Applied) {
      return;
    }
    paintedPositions_.insert(key);
    notifyTilesMutated(changedPositions);
    return;
  }

  auto *tile = map_->getTile(pos);
  if (!tile) {
    return;
  }

  paintedPositions_.insert(key);
  historyManager_->recordTileBefore(pos, tile);
  currentBrush_->undraw(*map_, tile);
}



void BrushController::paintDoodadRecordedPosition(const Domain::Position &pos,
                                                  uint32_t modifiers) {
  if (!map_ || !historyManager_) {
    return;
  }

  auto *doodadBrush = dynamic_cast<DoodadBrush *>(currentBrush_);
  if (!doodadBrush) {
    paintRecordedPosition(pos, modifiers);
    return;
  }

  std::optional<uint32_t> seed;
  if (previewService_) {
    seed = previewService_->getCurrentSeed();
  }
  if (!seed) {
    seed = DoodadPlacementPlanner::buildSeed(
        *doodadBrush, pos, brushSettingsService_, static_cast<size_t>(variation_),
        true);
  }
  const auto plan = doodadBrush->buildPlacementPlan(
      pos, brushSettingsService_, static_cast<size_t>(variation_), map_,
      true, *seed);
  if (plan.layout.empty()) {
    return;
  }

  for (const auto &affectedPosition : plan.affectedPositions) {
    historyManager_->recordTileBefore(affectedPosition,
                                      map_->getTile(affectedPosition));
  }

  for (const auto &layoutTile : plan.layout) {
    const Domain::Position absolutePosition(
        pos.x + layoutTile.relativePosition.x, pos.y + layoutTile.relativePosition.y,
        static_cast<int16_t>(pos.z + layoutTile.relativePosition.z));
    auto key = std::make_tuple(absolutePosition.x, absolutePosition.y,
                               absolutePosition.z);
    if (paintedPositions_.contains(key)) {
      continue;
    }

    paintedPositions_.insert(key);
  }

  auto ctx = createDrawContext(modifiers);
  ctx.isDragging = strokeActive_;
  doodadBrush->applyPlacementPlan(*map_, pos, plan, ctx);
  notifyTilesMutated(plan.affectedPositions);

  if (previewService_) {
    previewService_->regenerate();
  }
}

void BrushController::eraseDoodadRecordedPosition(const Domain::Position &pos,
                                                  uint32_t modifiers) {
  if (!map_ || !historyManager_) {
    return;
  }

  auto *doodadBrush = dynamic_cast<DoodadBrush *>(currentBrush_);
  if (!doodadBrush) {
    eraseRecordedPosition(pos);
    return;
  }

  const DoodadBrush::EraseOptions eraseOptions{
      .matchingBrushOnly = true,
      .preserveComplexItems =
          !brushSettingsService_ ||
          brushSettingsService_->getEraserLeaveUniqueItems()};
  const auto forcePlace = (modifiers & Modifiers::Alt) != 0;
  const auto seed = DoodadPlacementPlanner::buildSeed(
      *doodadBrush, pos, brushSettingsService_, static_cast<size_t>(variation_),
      forcePlace);
  const auto plan = doodadBrush->buildErasePlan(
      pos, brushSettingsService_, static_cast<size_t>(variation_), map_,
      forcePlace, seed, eraseOptions);
  if (plan.positions.empty()) {
    return;
  }

  for (const auto &affectedPosition : plan.affectedPositions) {
    historyManager_->recordTileBefore(affectedPosition,
                                      map_->getTile(affectedPosition));
  }

  bool mutated = false;
  for (const auto &absolutePosition : plan.positions) {
    auto key = std::make_tuple(absolutePosition.x, absolutePosition.y,
                               absolutePosition.z);
    if (paintedPositions_.contains(key)) {
      continue;
    }

    auto *tile = map_->getTile(absolutePosition);
    if (!tile) {
      continue;
    }

    paintedPositions_.insert(key);
    doodadBrush->undraw(*map_, tile, eraseOptions);
    mutated = true;
  }
  if (mutated) {
    notifyTilesMutated(plan.affectedPositions);
    if (previewService_) {
      previewService_->regenerate();
    }
  }
}

void BrushController::paintExpandedCenter(const Domain::Position &center,
                                          uint32_t modifiers) {
  const auto expanded = getBrushPositionsForCenter(center);
  paintRecordedPositions(expanded, modifiers);
}

void BrushController::eraseExpandedCenter(const Domain::Position &center) {
  for (const auto &pos : getBrushPositionsForCenter(center)) {
    eraseRecordedPosition(pos);
  }
}

void BrushController::continueGroundLikeStroke(const Domain::Position &pos) {
  if (!lastStrokePos_.has_value()) {
    if (strokeEraseMode_) {
      eraseExpandedCenter(pos);
    } else {
      paintExpandedCenter(pos, strokeModifiers_);
    }
    lastStrokePos_ = pos;
    return;
  }

  for (const auto &linePos : getLinePositions(lastStrokePos_.value(), pos)) {
    if (strokeEraseMode_) {
      eraseExpandedCenter(linePos);
    } else {
      paintExpandedCenter(linePos, strokeModifiers_);
    }
  }

  lastStrokePos_ = pos;
}

void BrushController::continueWallLikeStroke(const Domain::Position &pos) {
  const bool altPressed = (strokeModifiers_ & Modifiers::Alt) != 0;
  const bool wallVariantShift =
      !strokeEraseMode_ && altPressed && currentBrush_ &&
      currentBrush_->getType() == BrushType::Wall &&
      getBrushPositionsForCenter(pos).size() == 1;

  if (wallVariantShift) {
    paintRecordedPosition(pos, strokeModifiers_, true);
    lastStrokePos_ = pos;
    return;
  }

  auto linePositions = !lastStrokePos_.has_value()
                           ? std::vector<Domain::Position>{pos}
                           : getLinePositions(lastStrokePos_.value(), pos);

  std::vector<Domain::Position> newPositions;
  for (const auto &linePos : linePositions) {
    std::vector<Domain::Position> footprint = (strokeEraseMode_ || !altPressed)
                                                  ? getBrushPositionsForCenter(linePos)
                                                  : std::vector<Domain::Position>{linePos};
    for (const auto &p : footprint) {
      auto key = std::make_tuple(p.x, p.y, p.z);
      if (!paintedPositions_.contains(key)) {
        newPositions.push_back(p);
      }
    }
  }

  std::vector<Domain::Position> uniqueNewPositions;
  std::unordered_set<std::tuple<int32_t, int32_t, int16_t>, PositionHash> localSeen;
  for (const auto &p : newPositions) {
    auto key = std::make_tuple(p.x, p.y, p.z);
    if (localSeen.insert(key).second) {
      uniqueNewPositions.push_back(p);
    }
  }

  if (uniqueNewPositions.empty()) {
    lastStrokePos_ = pos;
    return;
  }

  if (!map_ || !historyManager_ || !currentBrush_) {
    lastStrokePos_ = pos;
    return;
  }

  Services::Autoborder::PlacementIntent intent;
  intent.brush = currentBrush_;
  intent.context = createDrawContext(strokeModifiers_, false);
  intent.mode = strokeEraseMode_ ? Services::Autoborder::PlacementMode::Erase
                                 : Services::Autoborder::PlacementMode::Draw;
  intent.positions = std::move(uniqueNewPositions);

  std::vector<Domain::Position> changedPositions;
  const auto plannedResult = Services::Autoborder::applyPlannedIntentWithHistory(
      autoborderEngine_, *map_, *historyManager_, intent, &changedPositions);

  if (plannedResult == Services::Autoborder::PlannedMutationResult::Applied) {
    for (const auto &p : intent.positions) {
      paintedPositions_.insert(std::make_tuple(p.x, p.y, p.z));
    }
    notifyTilesMutated(changedPositions);
  } else if (plannedResult == Services::Autoborder::PlannedMutationResult::Unsupported) {
    for (const auto &p : intent.positions) {
      paintedPositions_.insert(std::make_tuple(p.x, p.y, p.z));
      historyManager_->recordTileBefore(p, map_->getTile(p));
      if (strokeEraseMode_) {
        currentBrush_->undraw(*map_, map_->getTile(p));
      } else {
        paintTileDirect(p, strokeModifiers_, false);
      }
    }
    if (!brushSettingsService_ || brushSettingsService_->getAutoBorder()) {
      for (const auto &p : intent.positions) {
        if (auto *wallBrush = dynamic_cast<const WallBrush *>(currentBrush_)) {
          wallBrush->rebuildAround(*map_, p);
        }
      }
    }
  }

  lastStrokePos_ = pos;
}

void BrushController::continueDoorLikeStroke(const Domain::Position &pos) {
  if (!lastStrokePos_.has_value()) {
    if (strokeEraseMode_) {
      eraseRecordedPosition(pos);
    } else {
      paintRecordedPosition(pos, strokeModifiers_);
    }
    lastStrokePos_ = pos;
    return;
  }

  for (const auto &linePos : getLinePositions(lastStrokePos_.value(), pos)) {
    if (strokeEraseMode_) {
      eraseRecordedPosition(linePos);
    } else {
      paintRecordedPosition(linePos, strokeModifiers_);
    }
  }

  lastStrokePos_ = pos;
}

void BrushController::continueDoodadLikeStroke(const Domain::Position &pos) {
  if (!lastStrokePos_.has_value()) {
    if (strokeEraseMode_) {
      eraseDoodadRecordedPosition(pos, strokeModifiers_);
    } else {
      paintDoodadRecordedPosition(pos, strokeModifiers_);
    }
    lastStrokePos_ = pos;
    return;
  }

  for (const auto &linePos : getLinePositions(lastStrokePos_.value(), pos)) {
    if (strokeEraseMode_) {
      eraseDoodadRecordedPosition(linePos, strokeModifiers_);
    } else {
      paintDoodadRecordedPosition(linePos, strokeModifiers_);
    }
  }

  lastStrokePos_ = pos;
}

void BrushController::continuePointLikeStroke(const Domain::Position &pos) {
  continueDoorLikeStroke(pos);
}

void BrushController::resetStrokeState() {
  altReplaceState_ = {};
  strokeActive_ = false;
  strokeEraseMode_ = false;
  paintedPositions_.clear();
  lastStrokePos_.reset();
  strokeModifiers_ = 0;
}

void BrushController::endStroke() {
  if (!strokeActive_ || !historyManager_) {
    resetStrokeState();
    return;
  }

  if (!paintedPositions_.empty()) {
    spdlog::debug("[BrushController] Ended stroke with {} tiles",
                  paintedPositions_.size());

    historyManager_->endOperation(map_, nullptr);
  } else {
    historyManager_->cancelOperation();
  }

  resetStrokeState();
}

std::vector<Domain::Position>
BrushController::getLinePositions(const Domain::Position &from,
                                  const Domain::Position &to) const {

  std::vector<Domain::Position> positions;

  int32_t x0 = from.x, y0 = from.y;
  int32_t x1 = to.x, y1 = to.y;
  int16_t z = from.z;

  int32_t dx = std::abs(x1 - x0);
  int32_t dy = -std::abs(y1 - y0);
  int32_t sx = x0 < x1 ? 1 : -1;
  int32_t sy = y0 < y1 ? 1 : -1;
  int32_t err = dx + dy;

  while (true) {
    positions.push_back({x0, y0, z});

    if (x0 == x1 && y0 == y1)
      break;

    int32_t e2 = 2 * err;
    if (e2 >= dy) {
      if (x0 == x1)
        break;
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      if (y0 == y1)
        break;
      err += dx;
      y0 += sy;
    }
  }

  return positions;
}

} // namespace MapEditor::Brushes
