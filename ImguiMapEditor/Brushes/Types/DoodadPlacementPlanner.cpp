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

uint32_t totalSingleChance(const DoodadAlternative &alternative) {
  return std::accumulate(alternative.getSingleItems().begin(),
                         alternative.getSingleItems().end(), 0u,
                         [](uint32_t sum, const SingleItem &item) {
                           return sum + item.chance;
                         });
}

uint32_t totalCompositeChance(const DoodadAlternative &alternative) {
  return std::accumulate(alternative.getComposites().begin(),
                         alternative.getComposites().end(), 0u,
                         [](uint32_t sum, const CompositeItem &item) {
                           return sum + item.chance;
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
buildSingleCandidate(const DoodadAlternative &alternative, std::mt19937 &rng, int anchorX,
                     int anchorY) {
  DoodadBrush::DoodadLayout candidate;
  const auto item = alternative.selectRandomSingle(rng);
  if (item.itemId == 0) {
    return candidate;
  }

  Services::Preview::PreviewTileData tile(anchorX, anchorY, 0);
  tile.addItem(item.itemId, static_cast<uint16_t>(item.subtype));
  candidate.push_back(std::move(tile));
  return candidate;
}

DoodadBrush::DoodadLayout
buildCompositeCandidate(const DoodadAlternative &alternative, std::mt19937 &rng, int anchorX,
                        int anchorY) {
  DoodadBrush::DoodadLayout candidate;
  const auto *composite = alternative.selectRandomComposite(rng);
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
buildRandomCandidate(const DoodadAlternative &alternative, std::mt19937 &rng, uint32_t singleChance,
                     uint32_t compositeChance, int anchorX, int anchorY) {
  const auto totalChance = singleChance + compositeChance;
  const bool useComposite =
      compositeChance > 0 &&
      (singleChance == 0 ||
       WeightedSelection::randomRange(rng, 1, totalChance) > singleChance);
  return useComposite ? buildCompositeCandidate(alternative, rng, anchorX, anchorY)
                      : buildSingleCandidate(alternative, rng, anchorX, anchorY);
}

int calculateObjectCount(size_t area, float thickness, std::mt19937 &rng) {
  const auto objectRange =
      static_cast<uint32_t>(static_cast<float>(std::max<size_t>(1, area)) *
                            std::max(0.0f, thickness));
  return static_cast<int>(
      std::max<uint32_t>(1, objectRange +
                                WeightedSelection::randomRange(rng, 0, objectRange)));
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
  uint32_t seedVal = request.seed ? *request.seed : buildSeed(request.brush, request.center, request.brushSettings, request.preferredVariation, request.forcePlace);
  auto rawStamp = generateRawStamp(request.brush, request.brushSettings, request.preferredVariation, seedVal);

  DoodadBrush::PlacementPlan plan;
  std::unordered_set<int64_t> occupiedAbs;

  for (const auto &tile : rawStamp) {
    const auto absPos = absolutePosition(request.center, tile.relativePosition);

    if (request.map && !request.forcePlace) {
      const auto *targetTile = request.map->getTile(absPos);
      if (!request.brush.onBlocking_ && Types::tileHasBlockingContents(targetTile)) {
        plan.skipped.push_back({.position = absPos, .reason = DoodadBrush::PlacementSkipReason::BlockingTile});
        continue;
      }
      if (!request.brush.onDuplicate_ && request.brush.tileHasOwnItem(targetTile)) {
        plan.skipped.push_back({.position = absPos, .reason = DoodadBrush::PlacementSkipReason::DuplicateOwnItem});
        continue;
      }
    }

    if (occupiedAbs.insert(encodeDoodadPosition(absPos)).second) {
      plan.layout.push_back(tile);
    } else {
      plan.skipped.push_back({.position = absPos, .reason = DoodadBrush::PlacementSkipReason::OccupiedInPlan});
    }
  }

  plan.redoTouches = buildRedoTouches(request, plan.layout);
  plan.affectedPositions = buildAffectedPositions(request, plan.layout, plan.redoTouches);
  return plan;
}

DoodadBrush::DoodadLayout
DoodadPlacementPlanner::generateRawStamp(
    const DoodadBrush &brush,
    const Services::BrushSettingsService *brushSettings,
    size_t preferredVariation,
    std::optional<uint32_t> seed) {
  std::mt19937 rng;
  if (seed) {
    rng.seed(*seed);
  } else {
    std::random_device rd;
    rng.seed(rd());
  }

  const auto *alternative = brush.selectAlternative(preferredVariation);
  if (!alternative) {
    return {};
  }

  const auto anchors = getAnchors(brush.isOneSize(), brushSettings);
  const auto singleChance = totalSingleChance(*alternative);
  const auto compositeChance = totalCompositeChance(*alternative);
  const auto scatterMode = !brush.isOneSize() && anchors.size() > 1;

  DoodadBrush::DoodadLayout rawStamp;
  std::unordered_set<int64_t> occupied;

  if (scatterMode) {
    const auto objectCount = calculateObjectCount(anchors.size(), brush.getThickness(), rng);
    for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex) {
      const auto anchorIndex = WeightedSelection::randomRange(rng, 0, static_cast<uint32_t>(anchors.size() - 1));
      const auto &[anchorX, anchorY] = anchors[anchorIndex];
      auto candidate = buildRandomCandidate(*alternative, rng, singleChance, compositeChance, anchorX, anchorY);
      
      bool overlap = false;
      for (const auto &tile : candidate) {
        if (occupied.contains(encodeDoodadPosition(tile.relativePosition))) {
          overlap = true;
          break;
        }
      }
      if (!overlap) {
        for (const auto &tile : candidate) {
          occupied.insert(encodeDoodadPosition(tile.relativePosition));
          appendLayoutTile(rawStamp, tile);
        }
      }
    }
  } else {
    for (const auto &[anchorX, anchorY] : anchors) {
      auto candidate = buildRandomCandidate(*alternative, rng, singleChance, compositeChance, anchorX, anchorY);
      bool overlap = false;
      for (const auto &tile : candidate) {
        if (occupied.contains(encodeDoodadPosition(tile.relativePosition))) {
          overlap = true;
          break;
        }
      }
      if (!overlap) {
        for (const auto &tile : candidate) {
          occupied.insert(encodeDoodadPosition(tile.relativePosition));
          appendLayoutTile(rawStamp, tile);
        }
      }
    }
  }
  return rawStamp;
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

  const auto anchors = getAnchors(brush.isOneSize(), brushSettings);
  Utils::mixSeed(seed, static_cast<uint32_t>(anchors.size()));
  for (const auto &[x, y] : anchors) {
    Utils::mixSeed(seed, static_cast<uint32_t>(x));
    Utils::mixSeed(seed, static_cast<uint32_t>(y));
  }

  return seed;
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
      if (itemType && itemType->isGround()) {
        touch.placedGround = true;
      } else if (itemType && itemType->isWall()) {
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

} // namespace MapEditor::Brushes
