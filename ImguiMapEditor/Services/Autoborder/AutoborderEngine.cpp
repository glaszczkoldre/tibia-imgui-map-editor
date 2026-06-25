#include "AutoborderEngine.h"

#include "Brushes/Types/CarpetBrush.h"
#include "Brushes/Types/GroundBrush.h"
#include "Brushes/Types/TableBrush.h"
#include "Brushes/Types/WallBrush.h"
#include "Brushes/Behaviors/WeightedSelection.h"
#include "Brushes/BrushRegistry.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Tile.h"
#include "Utils/HashUtils.h"
#include "Utils/PositionUtils.h"
#include <algorithm>
#include <span>
#include <string_view>

namespace MapEditor::Services::Autoborder {

namespace {

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
  return Utils::dedupeAndSortPositions(std::move(positions));
}

bool sameTileState(const Domain::Tile *lhs, const Domain::Tile *rhs,
                   const Domain::Position &/*pos*/) {
  if (lhs == rhs) {
    return true;
  }
  if (!lhs || !rhs) {
    return false;
  }
  return lhs->hasSameState(rhs);
}

uint32_t deterministicSeed(const PlacementIntent &intent) noexcept {
  uint32_t seed = Utils::kFnvOffsetBasis;
  if (intent.brush) {
    Utils::mixSeed(seed, intent.brush->getName());
    Utils::mixSeed(seed, static_cast<uint32_t>(intent.brush->getType()));
  }
  Utils::mixSeed(seed, static_cast<uint32_t>(intent.mode));
  Utils::mixSeed(seed, static_cast<uint32_t>(intent.context.variation));
  Utils::mixSeed(seed, intent.context.modifiers);
  Utils::mixSeed(seed, intent.context.specialAction ? 1u : 0u);
  Utils::mixSeed(seed, intent.context.forcePlace ? 1u : 0u);
  for (const auto &pos : intent.positions) {
    Utils::mixSeed(seed, static_cast<uint32_t>(pos.x));
    Utils::mixSeed(seed, static_cast<uint32_t>(pos.y));
    Utils::mixSeed(seed, static_cast<uint32_t>(pos.z));
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
               const std::vector<Domain::Position> &affected) const override {
    const auto *carpetBrush =
        dynamic_cast<const MapEditor::Brushes::CarpetBrush *>(intent.brush);
    if (!carpetBrush) {
      return;
    }
    // `affected` is the 3x3 expansion around every painted position.
    // Walking it once guarantees that adjacent carpet tiles re-align
    // themselves the same way they do in RME's `Map::doCarpets()` pass.
    for (const auto &pos : affected) {
      carpetBrush->rebuildTile(scratchMap, pos);
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
               const std::vector<Domain::Position> &affected) const override {
    const auto *tableBrush =
        dynamic_cast<const MapEditor::Brushes::TableBrush *>(intent.brush);
    if (!tableBrush) {
      return;
    }
    for (const auto &pos : affected) {
      tableBrush->rebuildTile(scratchMap, pos);
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
  if (intent.context.brushRegistry) {
    intent.context.brushRegistry->getRng().seed(deterministicSeed(intent));
  }
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
