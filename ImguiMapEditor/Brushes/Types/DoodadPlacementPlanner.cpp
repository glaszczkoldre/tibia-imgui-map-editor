#include "DoodadPlacementPlanner.h"

#include "BrushUtils.h"
#include "Brushes/Behaviors/WeightedSelection.h"
#include "Brushes/BrushRegistry.h"
#include "Domain/ChunkedMap.h"
#include "Domain/ItemType.h"
#include "Domain/Tile.h"
#include "Services/BrushSettingsService.h"
#include "Services/ClientDataService.h"
#include "Utils/HashUtils.h"
#include "Utils/PositionUtils.h"
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>
#include <unordered_map>

namespace MapEditor::Brushes {

namespace {

uint32_t normalizedChance(uint32_t chance) { return chance == 0 ? 1u : chance; }

uint32_t totalSingleChance(const DoodadAlternative &alternative) {
  return std::accumulate(alternative.getSingleItems().begin(),
                         alternative.getSingleItems().end(), 0u,
                         [](uint32_t sum, const SingleItem &item) {
                           return sum + normalizedChance(item.chance);
                         });
}

uint32_t totalCompositeChance(const DoodadAlternative &alternative) {
  return std::accumulate(alternative.getComposites().begin(),
                         alternative.getComposites().end(), 0u,
                         [](uint32_t sum, const CompositeItem &item) {
                           return sum + normalizedChance(item.chance);
                         });
}

uint64_t encodeSkipKey(const Domain::Position &position,
                       DoodadBrush::PlacementSkipReason reason) {
  auto key = static_cast<uint64_t>(std::hash<Domain::Position>{}(position));
  key ^= static_cast<uint64_t>(reason) + 0x9e3779b97f4a7c15ull +
         (key << 6) + (key >> 2);
  return key;
}

Domain::Position absolutePosition(const Domain::Position &center,
                                  const Domain::Position &relative) {
  return {center.x + relative.x, center.y + relative.y,
          static_cast<int16_t>(center.z + relative.z)};
}

void appendLayoutTile(DoodadBrush::DoodadLayout &layout,
                      Services::Preview::PreviewTileData tile) {
  const auto existing = std::find_if(
      layout.begin(), layout.end(),
      [&tile](const Services::Preview::PreviewTileData &candidate) {
        return candidate.relativePosition == tile.relativePosition;
      });

  if (existing == layout.end()) {
    layout.push_back(std::move(tile));
    return;
  }

  existing->items.insert(existing->items.end(), tile.items.begin(),
                         tile.items.end());
}

std::vector<std::pair<int, int>>
getAnchors(bool oneSize,
           const Services::BrushSettingsService *brushSettings) {
  if (oneSize || !brushSettings) {
    return {{0, 0}};
  }

  auto anchors = brushSettings->getBrushOffsets();
  if (anchors.empty()) {
    anchors.emplace_back(0, 0);
  }
  return anchors;
}

DoodadBrush::DoodadLayout
buildSingleCandidate(const DoodadAlternative &alternative, int anchorX,
                     int anchorY) {
  DoodadBrush::DoodadLayout candidate;
  const auto item = alternative.selectRandomSingle();
  if (item.itemId == 0) {
    return candidate;
  }

  Services::Preview::PreviewTileData tile(anchorX, anchorY, 0);
  tile.addItem(item.itemId, static_cast<uint16_t>(item.subtype));
  candidate.push_back(std::move(tile));
  return candidate;
}

DoodadBrush::DoodadLayout
buildCompositeCandidate(const DoodadAlternative &alternative, int anchorX,
                        int anchorY) {
  DoodadBrush::DoodadLayout candidate;
  const auto *composite = alternative.selectRandomComposite();
  if (!composite) {
    return candidate;
  }

  for (const auto &offset : composite->tiles) {
    Services::Preview::PreviewTileData tile(anchorX + offset.dx,
                                            anchorY + offset.dy, offset.dz);
    for (const auto &item : offset.items) {
      if (item.itemId != 0) {
        tile.addItem(item.itemId, static_cast<uint16_t>(item.subtype));
      }
    }
    appendLayoutTile(candidate, std::move(tile));
  }

  return candidate;
}

DoodadBrush::DoodadLayout
buildRandomCandidate(const DoodadAlternative &alternative, uint32_t singleChance,
                     uint32_t compositeChance, int anchorX, int anchorY) {
  const auto totalChance = singleChance + compositeChance;
  const bool useComposite =
      compositeChance > 0 &&
      (singleChance == 0 ||
       WeightedSelection::randomRange(1, totalChance) > singleChance);
  return useComposite ? buildCompositeCandidate(alternative, anchorX, anchorY)
                      : buildSingleCandidate(alternative, anchorX, anchorY);
}

int calculateObjectCount(size_t area, float thickness) {
  const auto objectRange =
      static_cast<uint32_t>(static_cast<float>(std::max<size_t>(1, area)) *
                            std::max(0.0f, thickness));
  return static_cast<int>(
      std::max<uint32_t>(1, objectRange +
                                WeightedSelection::randomRange(0, objectRange)));
}

void appendSkip(std::vector<DoodadBrush::PlacementSkip> &skipped,
                std::unordered_set<uint64_t> &skippedKeys,
                const Domain::Position &position,
                DoodadBrush::PlacementSkipReason reason) {
  if (skippedKeys.insert(encodeSkipKey(position, reason)).second) {
    skipped.push_back({.position = position, .reason = reason});
  }
}

std::vector<Domain::Position>
dedupeAndSort(std::vector<Domain::Position> positions) {
  return Utils::dedupeAndSortPositions(std::move(positions));
}

} // namespace

DoodadBrush::PlacementPlan DoodadPlacementPlanner::buildPlan(
    const Request &request) {
  if (!request.seed) {
    return buildPlanUnseeded(request);
  }

  WeightedSelection::ScopedSeed scopedSeed(*request.seed);
  return buildPlanUnseeded(request);
}

DoodadBrush::DoodadLayout
DoodadPlacementPlanner::build(const Request &request) {
  return buildPlan(request).layout;
}

uint32_t DoodadPlacementPlanner::buildSeed(
    const DoodadBrush &brush, const Domain::Position &center,
    const Services::BrushSettingsService *brushSettings,
    size_t preferredVariation, bool forcePlace) {
  uint32_t seed = Utils::kFnvOffsetBasis;
  Utils::mixSeed(seed, brush.getName());
  Utils::mixSeed(seed, static_cast<uint32_t>(center.x));
  Utils::mixSeed(seed, static_cast<uint32_t>(center.y));
  Utils::mixSeed(seed, static_cast<uint32_t>(center.z));
  Utils::mixSeed(seed, static_cast<uint32_t>(preferredVariation));
  Utils::mixSeed(seed, forcePlace ? 1u : 0u);

  const auto anchors = getAnchors(brush.oneSize_, brushSettings);
  Utils::mixSeed(seed, static_cast<uint32_t>(anchors.size()));
  for (const auto &[x, y] : anchors) {
    Utils::mixSeed(seed, static_cast<uint32_t>(x));
    Utils::mixSeed(seed, static_cast<uint32_t>(y));
  }

  return seed;
}

std::optional<DoodadBrush::PlacementSkipReason>
DoodadPlacementPlanner::getSkipReason(
    const Request &request, const DoodadBrush::DoodadLayout &layout,
    const Services::Preview::PreviewTileData &tile) {
  const auto alreadyPlanned =
      std::ranges::any_of(layout, [&tile](const auto &existing) {
        return existing.relativePosition == tile.relativePosition;
      });
  if (alreadyPlanned) {
    return DoodadBrush::PlacementSkipReason::OccupiedInPlan;
  }

  if (!request.map || request.forcePlace) {
    return std::nullopt;
  }

  const auto targetPosition =
      absolutePosition(request.center, tile.relativePosition);
  const auto *targetTile = request.map->getTile(targetPosition);
  if (!request.brush.onBlocking_ &&
      Types::tileHasBlockingContents(targetTile)) {
    return DoodadBrush::PlacementSkipReason::BlockingTile;
  }
  if (!request.brush.onDuplicate_ &&
      request.brush.tileHasOwnItem(targetTile)) {
    return DoodadBrush::PlacementSkipReason::DuplicateOwnItem;
  }

  return std::nullopt;
}

bool DoodadPlacementPlanner::tryAppendCandidate(
    const Request &request, DoodadBrush::DoodadLayout &layout,
    std::vector<DoodadBrush::PlacementSkip> &skipped,
    std::unordered_set<uint64_t> &skippedKeys,
    std::unordered_set<int64_t> &occupied,
    const DoodadBrush::DoodadLayout &candidate) {
  if (candidate.empty()) {
    return false;
  }

  std::vector<DoodadBrush::PlacementSkip> candidateSkips;
  for (const auto &candidateTile : candidate) {
    if (occupied.contains(encodeDoodadPosition(candidateTile.relativePosition))) {
      candidateSkips.push_back(
          {.position = absolutePosition(request.center,
                                        candidateTile.relativePosition),
           .reason = DoodadBrush::PlacementSkipReason::OccupiedInPlan});
      continue;
    }

    if (const auto reason = getSkipReason(request, layout, candidateTile)) {
      candidateSkips.push_back(
          {.position = absolutePosition(request.center,
                                        candidateTile.relativePosition),
           .reason = *reason});
    }
  }

  if (!candidateSkips.empty()) {
    for (const auto &skip : candidateSkips) {
      appendSkip(skipped, skippedKeys, skip.position, skip.reason);
    }
    return false;
  }

  for (const auto &candidateTile : candidate) {
    occupied.insert(encodeDoodadPosition(candidateTile.relativePosition));
    appendLayoutTile(layout, candidateTile);
  }

  return true;
}

std::vector<DoodadRedoBorderTouch> DoodadPlacementPlanner::buildRedoTouches(
    const Request &request, const DoodadBrush::DoodadLayout &layout) {
  if (!request.brush.redoBorders_) {
    return {};
  }

  std::vector<DoodadRedoBorderTouch> touches;
  std::unordered_map<Domain::Position, size_t> touchIndexes;
  const auto *clientData = request.brush.registry_.getClientDataService();

  for (const auto &layoutTile : layout) {
    const auto absolute = absolutePosition(request.center,
                                           layoutTile.relativePosition);
    for (const auto &item : layoutTile.items) {
      if (item.itemId == 0) {
        continue;
      }

      const auto [it, inserted] =
          touchIndexes.emplace(absolute, touches.size());
      if (inserted) {
        touches.push_back({.position = absolute});
      }

      const Domain::ItemType *itemType =
          clientData
              ? clientData->getItemTypeByServerId(
                    static_cast<uint16_t>(item.itemId))
              : nullptr;
      auto &touch = touches[it->second];
      if (itemType && itemType->is_ground) {
        touch.placedGround = true;
      } else if (itemType && itemType->is_wall) {
        touch.placedWall = true;
      }
    }
  }

  return touches;
}

std::vector<Domain::Position> DoodadPlacementPlanner::buildAffectedPositions(
    const Request &request, const DoodadBrush::DoodadLayout &layout,
    const std::vector<DoodadRedoBorderTouch> &redoTouches) {
  std::vector<Domain::Position> positions;
  positions.reserve(layout.size() + redoTouches.size() * 9);

  for (const auto &layoutTile : layout) {
    positions.push_back(
        absolutePosition(request.center, layoutTile.relativePosition));
  }

  for (const auto &position : buildDoodadRedoBorderPositions(redoTouches)) {
    positions.push_back(position);
  }

  return dedupeAndSort(std::move(positions));
}

DoodadBrush::PlacementPlan
DoodadPlacementPlanner::buildPlanUnseeded(const Request &request) {
  auto result = buildLayoutUnseeded(request);
  auto redoTouches = buildRedoTouches(request, result.layout);
  auto affectedPositions =
      buildAffectedPositions(request, result.layout, redoTouches);
  return {.layout = std::move(result.layout),
          .redoTouches = std::move(redoTouches),
          .skipped = std::move(result.skipped),
          .affectedPositions = std::move(affectedPositions)};
}

DoodadPlacementPlanner::LayoutBuildResult
DoodadPlacementPlanner::buildLayoutUnseeded(const Request &request) {
  LayoutBuildResult result;
  const auto *alternative =
      request.brush.selectAlternative(request.preferredVariation);
  if (!alternative) {
    return result;
  }

  const auto anchors = getAnchors(request.brush.oneSize_, request.brushSettings);
  const auto singleChance = totalSingleChance(*alternative);
  const auto compositeChance = totalCompositeChance(*alternative);
  const auto scatterMode = !request.brush.oneSize_ && anchors.size() > 1;
  const auto maxAttempts =
      std::max<size_t>(1, alternative->getSingleItems().size() +
                              alternative->getComposites().size()) *
      2;

  std::unordered_set<int64_t> occupied;
  std::unordered_set<uint64_t> skippedKeys;
  if (scatterMode) {
    const auto objectCount =
        calculateObjectCount(anchors.size(), request.brush.thickness_);

    for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex) {
      for (size_t attempt = 0; attempt < 5; ++attempt) {
        const auto anchorIndex =
            WeightedSelection::randomRange(0,
                                           static_cast<uint32_t>(anchors.size() - 1));
        const auto &[anchorX, anchorY] = anchors[anchorIndex];
        auto candidate = buildRandomCandidate(*alternative, singleChance,
                                              compositeChance, anchorX, anchorY);
        if (tryAppendCandidate(request, result.layout, result.skipped,
                               skippedKeys, occupied, candidate)) {
          break;
        }
      }
    }

    return result;
  }

  for (const auto &[anchorX, anchorY] : anchors) {
    for (size_t attempt = 0; attempt < maxAttempts; ++attempt) {
      auto candidate = buildRandomCandidate(*alternative, singleChance,
                                            compositeChance, anchorX, anchorY);
      if (tryAppendCandidate(request, result.layout, result.skipped,
                             skippedKeys, occupied, candidate)) {
        break;
      }

      if (attempt + 1 == maxAttempts) {
        candidate = singleChance > 0
                        ? buildSingleCandidate(*alternative, anchorX, anchorY)
                        : buildCompositeCandidate(*alternative, anchorX,
                                                  anchorY);
        tryAppendCandidate(request, result.layout, result.skipped, skippedKeys,
                           occupied, candidate);
      }
    }
  }

  return result;
}

} // namespace MapEditor::Brushes
