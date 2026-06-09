#include "AutoborderEngine.h"

#include "Brushes/Types/CarpetBrush.h"
#include "Brushes/Types/GroundBrush.h"
#include "Brushes/Types/TableBrush.h"
#include "Brushes/Types/WallBrush.h"
#include "Brushes/Behaviors/WeightedSelection.h"
#include "Domain/ChunkedMap.h"
#include "Domain/History/TileSnapshot.h"
#include "Domain/Tile.h"
#include <algorithm>
#include <span>
#include <string_view>
#include <unordered_set>

namespace MapEditor::Services::Autoborder {

namespace {

std::vector<Domain::Position>
dedupeAndSort(std::vector<Domain::Position> positions) {
  std::unordered_set<Domain::Position> seen;
  seen.reserve(positions.size());
  std::vector<Domain::Position> result;
  result.reserve(positions.size());

  for (const auto &pos : positions) {
    if (seen.insert(pos).second) {
      result.push_back(pos);
    }
  }

  std::sort(result.begin(), result.end());
  return result;
}

std::vector<Domain::Position>
expandByOne(std::span<const Domain::Position> centers) {
  std::vector<Domain::Position> positions;
  positions.reserve(centers.size() * 9);
  for (const auto &center : centers) {
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        positions.emplace_back(center.x + dx, center.y + dy, center.z);
      }
    }
  }
  return dedupeAndSort(std::move(positions));
}

bool sameTileState(const Domain::Tile *lhs, const Domain::Tile *rhs,
                   const Domain::Position &pos) {
  auto leftSnap = Domain::History::TileSnapshot::capture(lhs, pos);
  auto rightSnap = Domain::History::TileSnapshot::capture(rhs, pos);
  return leftSnap.data() == rightSnap.data();
}

constexpr uint32_t kFnvPrime = 16777619u;
constexpr uint32_t kFnvOffsetBasis = 2166136261u;

void mixSeed(uint32_t &seed, uint32_t value) noexcept {
  seed ^= value;
  seed *= kFnvPrime;
}

void mixSeed(uint32_t &seed, std::string_view value) noexcept {
  for (const auto ch : value) {
    mixSeed(seed, static_cast<uint8_t>(ch));
  }
}

uint32_t deterministicSeed(const PlacementIntent &intent) noexcept {
  uint32_t seed = kFnvOffsetBasis;
  if (intent.brush) {
    mixSeed(seed, intent.brush->getName());
    mixSeed(seed, static_cast<uint32_t>(intent.brush->getType()));
  }
  mixSeed(seed, static_cast<uint32_t>(intent.mode));
  mixSeed(seed, static_cast<uint32_t>(intent.context.variation));
  mixSeed(seed, intent.context.modifiers);
  mixSeed(seed, intent.context.specialAction ? 1u : 0u);
  mixSeed(seed, intent.context.forcePlace ? 1u : 0u);
  for (const auto &pos : intent.positions) {
    mixSeed(seed, static_cast<uint32_t>(pos.x));
    mixSeed(seed, static_cast<uint32_t>(pos.y));
    mixSeed(seed, static_cast<uint32_t>(pos.z));
  }
  return seed;
}

Domain::Tile *getOrCloneTile(Domain::ChunkedMap &scratchMap,
                             const Domain::ChunkedMap &sourceMap,
                             const Domain::Position &pos) {
  if (auto *tile = scratchMap.getTile(pos)) {
    return tile;
  }

  if (const auto *sourceTile = sourceMap.getTile(pos)) {
    scratchMap.setTile(pos, sourceTile->clone());
    return scratchMap.getTile(pos);
  }

  return nullptr;
}

void cloneTiles(Domain::ChunkedMap &scratchMap,
                const Domain::ChunkedMap &sourceMap,
                std::span<const Domain::Position> positions) {
  for (const auto &pos : positions) {
    if (const auto *tile = sourceMap.getTile(pos)) {
      scratchMap.setTile(pos, tile->clone());
    }
  }
}

class RadiusOneResolver : public AutoborderResolver {
public:
  [[nodiscard]] std::vector<Domain::Position>
  expandAffectedPositions(const PlacementIntent &intent) const override {
    return expandByOne(intent.positions);
  }
};

class WallResolver final : public RadiusOneResolver {
public:
  [[nodiscard]] bool canResolve(const PlacementIntent &intent) const override {
    return dynamic_cast<const MapEditor::Brushes::WallBrush *>(intent.brush) !=
           nullptr;
  }

  void applyIntent(Domain::ChunkedMap &scratchMap,
                   const Domain::ChunkedMap &sourceMap,
                   const PlacementIntent &intent) const override {
    const auto *wallBrush =
        dynamic_cast<const MapEditor::Brushes::WallBrush *>(intent.brush);
    if (!wallBrush) {
      return;
    }

    if (intent.mode == PlacementMode::ResolveOnly) {
      return;
    }

    for (const auto &pos : intent.positions) {
      if (intent.mode == PlacementMode::Draw) {
        if (!wallBrush->canDraw(sourceMap, pos)) {
          continue;
        }

        if (auto *tile = scratchMap.getOrCreateTile(pos)) {
          wallBrush->placeWallTile(*tile, intent.context);
        }
        continue;
      }

      if (auto *tile = getOrCloneTile(scratchMap, sourceMap, pos)) {
        wallBrush->eraseFromTile(*tile);
      }
    }
  }

  void resolve(Domain::ChunkedMap &scratchMap, const PlacementIntent &intent,
               const std::vector<Domain::Position> &affected) const override {
    const auto *wallBrush =
        dynamic_cast<const MapEditor::Brushes::WallBrush *>(intent.brush);
    if (!wallBrush) {
      return;
    }
    const bool skipsLiveWallResolve =
        intent.context.isDragging &&
        intent.brush->getType() == MapEditor::Brushes::BrushType::Wall;
    const bool skipsVariantResolve =
        intent.mode == PlacementMode::Draw && intent.context.specialAction;
    if (intent.mode != PlacementMode::ResolveOnly &&
        (skipsVariantResolve || skipsLiveWallResolve)) {
      return;
    }
    wallBrush->rebuildTiles(scratchMap, affected);
  }
};

class GroundResolver final : public RadiusOneResolver {
public:
  [[nodiscard]] bool canResolve(const PlacementIntent &intent) const override {
    return dynamic_cast<const MapEditor::Brushes::GroundBrush *>(intent.brush) !=
           nullptr;
  }

  void applyIntent(Domain::ChunkedMap &scratchMap,
                   const Domain::ChunkedMap &sourceMap,
                   const PlacementIntent &intent) const override {
    const auto *groundBrush =
        dynamic_cast<const MapEditor::Brushes::GroundBrush *>(intent.brush);
    if (!groundBrush) {
      return;
    }

    if (intent.mode == PlacementMode::ResolveOnly) {
      return;
    }

    for (const auto &pos : intent.positions) {
      if (intent.mode == PlacementMode::Draw) {
        if (!groundBrush->canDraw(sourceMap, pos)) {
          continue;
        }

        if (auto *tile = scratchMap.getOrCreateTile(pos)) {
          groundBrush->placeGroundTile(*tile, intent.context);
        }
        continue;
      }

      if (auto *tile = getOrCloneTile(scratchMap, sourceMap, pos)) {
        groundBrush->eraseFromTile(*tile);
      }
    }
  }

  void resolve(Domain::ChunkedMap &scratchMap, const PlacementIntent &intent,
               const std::vector<Domain::Position> &affected) const override {
    const auto *groundBrush =
        dynamic_cast<const MapEditor::Brushes::GroundBrush *>(intent.brush);
    if (!groundBrush) {
      return;
    }
    for (const auto &pos : affected) {
      groundBrush->rebuildTile(scratchMap, pos);
    }
  }
};

class CarpetResolver final : public RadiusOneResolver {
public:
  [[nodiscard]] bool canResolve(const PlacementIntent &intent) const override {
    return dynamic_cast<const MapEditor::Brushes::CarpetBrush *>(intent.brush) !=
           nullptr;
  }

  void applyIntent(Domain::ChunkedMap &scratchMap,
                   const Domain::ChunkedMap &sourceMap,
                   const PlacementIntent &intent) const override {
    const auto *carpetBrush =
        dynamic_cast<const MapEditor::Brushes::CarpetBrush *>(intent.brush);
    if (!carpetBrush) {
      return;
    }

    if (intent.mode == PlacementMode::ResolveOnly) {
      return;
    }

    for (const auto &pos : intent.positions) {
      if (intent.mode == PlacementMode::Draw) {
        if (!carpetBrush->canDraw(sourceMap, pos)) {
          continue;
        }

        if (auto *tile = scratchMap.getOrCreateTile(pos)) {
          carpetBrush->placeCenterTile(*tile, intent.context);
        }
        continue;
      }

      if (auto *tile = getOrCloneTile(scratchMap, sourceMap, pos)) {
        carpetBrush->eraseFromTile(*tile);
      }
    }
  }

  void resolve(Domain::ChunkedMap &scratchMap, const PlacementIntent &intent,
               const std::vector<Domain::Position> &) const override {
    const auto *carpetBrush =
        dynamic_cast<const MapEditor::Brushes::CarpetBrush *>(intent.brush);
    if (!carpetBrush) {
      return;
    }
    for (const auto &pos : intent.positions) {
      carpetBrush->rebuildAround(scratchMap, pos);
    }
  }
};

class TableResolver final : public RadiusOneResolver {
public:
  [[nodiscard]] bool canResolve(const PlacementIntent &intent) const override {
    return dynamic_cast<const MapEditor::Brushes::TableBrush *>(intent.brush) !=
           nullptr;
  }

  void applyIntent(Domain::ChunkedMap &scratchMap,
                   const Domain::ChunkedMap &sourceMap,
                   const PlacementIntent &intent) const override {
    const auto *tableBrush =
        dynamic_cast<const MapEditor::Brushes::TableBrush *>(intent.brush);
    if (!tableBrush) {
      return;
    }

    if (intent.mode == PlacementMode::ResolveOnly) {
      return;
    }

    for (const auto &pos : intent.positions) {
      if (intent.mode == PlacementMode::Draw) {
        if (!tableBrush->canDraw(sourceMap, pos)) {
          continue;
        }

        if (auto *tile = scratchMap.getOrCreateTile(pos)) {
          tableBrush->placeAloneTile(*tile, intent.context);
        }
        continue;
      }

      if (auto *tile = getOrCloneTile(scratchMap, sourceMap, pos)) {
        tableBrush->eraseFromTile(*tile);
      }
    }
  }

  void resolve(Domain::ChunkedMap &scratchMap, const PlacementIntent &intent,
               const std::vector<Domain::Position> &) const override {
    const auto *tableBrush =
        dynamic_cast<const MapEditor::Brushes::TableBrush *>(intent.brush);
    if (!tableBrush) {
      return;
    }
    for (const auto &pos : intent.positions) {
      tableBrush->rebuildAround(scratchMap, pos);
    }
  }
};

} // namespace

AutoborderEngine::AutoborderEngine() {
  resolvers_.push_back(std::make_unique<WallResolver>());
  resolvers_.push_back(std::make_unique<GroundResolver>());
  resolvers_.push_back(std::make_unique<CarpetResolver>());
  resolvers_.push_back(std::make_unique<TableResolver>());
}

AutoborderEngine::~AutoborderEngine() = default;

bool AutoborderEngine::canPlan(const PlacementIntent &intent) const {
  return intent.isValid() && findResolver(intent) != nullptr;
}

const AutoborderResolver *
AutoborderEngine::findResolver(const PlacementIntent &intent) const {
  for (const auto &resolver : resolvers_) {
    if (resolver->canResolve(intent)) {
      return resolver.get();
    }
  }
  return nullptr;
}

TileDiffList AutoborderEngine::plan(const Domain::ChunkedMap &map,
                                    const PlacementIntent &intent) const {
  const auto *resolver = findResolver(intent);
  if (!resolver) {
    return {};
  }

  auto affected = resolver->expandAffectedPositions(intent);
  auto readPositions = expandByOne(affected);

  Domain::ChunkedMap scratchMap;
  cloneTiles(scratchMap, map, readPositions);
  MapEditor::Brushes::WeightedSelection::ScopedSeed scopedSeed(
      deterministicSeed(intent));
  resolver->applyIntent(scratchMap, map, intent);
  resolver->resolve(scratchMap, intent, affected);

  TileDiffList diffs;
  diffs.reserve(affected.size());
  for (const auto &pos : affected) {
    const auto *before = map.getTile(pos);
    const auto *after = scratchMap.getTile(pos);
    if (sameTileState(before, after, pos)) {
      continue;
    }

    diffs.push_back(TileDiff{
        .position = pos,
        .after = after ? after->clone() : nullptr,
    });
  }

  return diffs;
}

} // namespace MapEditor::Services::Autoborder
