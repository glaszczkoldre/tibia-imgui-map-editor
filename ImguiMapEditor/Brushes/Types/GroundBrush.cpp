#include "GroundBrush.h"

#include "Brushes/BrushRegistry.h"
#include "BrushUtils.h"
#include "Brushes/Behaviors/WeightedSelection.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Item.h"
#include "Domain/Tile.h"
#include "Services/ClientDataService.h"
#include "Services/BrushSettingsService.h"
#include <algorithm>
#include <cctype>
#include <limits>
#include <ranges>

namespace MapEditor::Brushes {

namespace {

[[nodiscard]] bool shouldSkipAltGroundPlacement(const BrushRegistry &registry,
                                                const Domain::Tile &tile,
                                                const DrawContext &ctx) {
  const bool altPressed = (ctx.modifiers & Modifiers::Alt) != 0;
  if (!altPressed) {
    if (ctx.altReplace) {
      *ctx.altReplace = {};
    }
    return false;
  }

  if (!ctx.altReplace) {
    return false;
  }

  if (!ctx.altReplace->active || !ctx.isDragging) {
    ctx.altReplace->active = true;
    ctx.altReplace->emptyOnly = !tile.hasGround();
    ctx.altReplace->replaceBrush = nullptr;

    if (!ctx.altReplace->emptyOnly) {
      ctx.altReplace->replaceBrush = GroundBrush::resolveGroundBrush(registry, tile);
      if (!ctx.altReplace->replaceBrush) {
        *ctx.altReplace = {};
        return true;
      }
    }
  }

  if (ctx.altReplace->emptyOnly) {
    return tile.hasGround();
  }

  const auto *currentBrush = GroundBrush::resolveGroundBrush(registry, tile);
  if (!currentBrush) {
    return true;
  }

  return currentBrush != ctx.altReplace->replaceBrush;
}

} // namespace

const GroundBrush *GroundBrush::resolveGroundBrush(const BrushRegistry &registry,
                                                   const Domain::Tile &tile) {
  if (tile.getGroundBrushId() != InvalidBrushId) {
    if (const auto *brush =
            dynamic_cast<const GroundBrush *>(registry.getBrushById(tile.getGroundBrushId()))) {
      return brush;
    }
  }

  const auto *ground = tile.getGround();
  if (!ground) {
    return nullptr;
  }

  if (ground->getOwnerBrushId() != InvalidBrushId) {
    if (const auto *brush = dynamic_cast<const GroundBrush *>(
            registry.getBrushById(ground->getOwnerBrushId()))) {
      return brush;
    }
  }

  for (auto *brush : registry.getBrushesForItem(ground->getServerId())) {
    if (const auto *groundBrush = dynamic_cast<const GroundBrush *>(brush);
        groundBrush && groundBrush->hasDrawableGroundItem(ground->getServerId())) {
      return groundBrush;
    }
  }

  return nullptr;
}

std::string GroundBrush::normalizeName(const std::string &value) {
  std::string normalized = value;
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return normalized;
}

GroundBrush::GroundBrush(std::string name, uint32_t lookId, BrushRegistry &registry)
    : BrushBase(std::move(name), lookId, true), registry_(registry) {}

void GroundBrush::draw(Domain::ChunkedMap &map, Domain::Tile *tile,
                       const DrawContext &ctx) {
  if (!tile) {
    return;
  }

  if (!placeGroundTile(*tile, ctx)) {
    return;
  }

  if (!ctx.brushSettings || ctx.brushSettings->getAutoBorder()) {
    rebuildAround(map, tile->getPosition());
  }
}

void GroundBrush::undraw(Domain::ChunkedMap &map, Domain::Tile *tile) {
  if (!tile) {
    return;
  }

  eraseFromTile(*tile);
  rebuildAround(map, tile->getPosition());
}

bool GroundBrush::ownsItem(const Domain::Item *item) const {
  return item && ownedItemIds_.contains(item->getServerId());
}

void GroundBrush::addGroundItem(uint16_t itemId, uint32_t chance) {
  groundItems_.emplace_back(itemId, chance);
  ownedItemIds_.insert(itemId);
  registry_.registerItemBinding(itemId, this);
  if (lookId_ == 0) {
    lookId_ = itemId;
  }
}

void GroundBrush::addFriend(const std::string &name) {
  if (!name.empty()) {
    friendNames_.insert(normalizeName(name));
  }
  hateFriends_ = false;
}

void GroundBrush::addEnemy(const std::string &name) {
  if (!name.empty()) {
    friendNames_.insert(normalizeName(name));
  }
  hateFriends_ = true;
}

void GroundBrush::clearFriends() {
  friendNames_.clear();
  hateFriends_ = false;
}

void GroundBrush::addBorderRule(BorderRule rule) {
  registry_.registerBorderBlockMetadata(rule.block);

  const auto registerOwnedItem = [this](uint32_t itemId) {
    if (itemId == 0 || itemId > std::numeric_limits<uint16_t>::max()) {
      return;
    }

    const auto ownedItemId = static_cast<uint16_t>(itemId);
    ownedItemIds_.insert(ownedItemId);
    registry_.registerItemBinding(ownedItemId, this);
  };

  for (size_t i = 0; i < BorderBlock::kEdgeTypeCount; ++i) {
    const auto edge = static_cast<EdgeType>(i);
    if (!rule.block.hasItemsFor(edge)) {
      continue;
    }
    for (const auto &[itemId, _] : rule.block.getItems(edge)) {
      registerOwnedItem(itemId);
    }
  }

  for (const auto &specificCase : rule.block.getSpecificCases()) {
    registerOwnedItem(specificCase.getToReplaceId());
    registerOwnedItem(specificCase.getWithId());
  }

  borderRules_.push_back(std::move(rule));
}

void GroundBrush::clearBorderRules() {
  auto collectBorderRuleItemIds = [](const BorderRule &rule,
                                     std::unordered_set<uint16_t> &itemIds) {
    const auto collectItemId = [&itemIds](uint32_t itemId) {
      if (itemId == 0 || itemId > std::numeric_limits<uint16_t>::max()) {
        return;
      }
      itemIds.insert(static_cast<uint16_t>(itemId));
    };

    for (size_t i = 0; i < BorderBlock::kEdgeTypeCount; ++i) {
      const auto edge = static_cast<EdgeType>(i);
      if (!rule.block.hasItemsFor(edge)) {
        continue;
      }
      for (const auto &[itemId, _] : rule.block.getItems(edge)) {
        collectItemId(itemId);
      }
    }

    for (const auto &specificCase : rule.block.getSpecificCases()) {
      collectItemId(specificCase.getToReplaceId());
      collectItemId(specificCase.getWithId());
    }
  };

  std::unordered_set<uint16_t> clearedItemIds;
  for (const auto &rule : borderRules_) {
    collectBorderRuleItemIds(rule, clearedItemIds);
  }

  borderRules_.clear();

  for (const auto itemId : clearedItemIds) {
    const bool isGroundItem = std::ranges::any_of(
        groundItems_, [itemId](const auto &entry) { return entry.first == itemId; });
    const bool isOptionalItem =
        optionalBorder_ &&
        ([this, itemId] {
          for (size_t i = 0; i < BorderBlock::kEdgeTypeCount; ++i) {
            const auto edge = static_cast<EdgeType>(i);
            if (!optionalBorder_->hasItemsFor(edge)) {
              continue;
            }
            for (const auto &[optionalItemId, _] :
                 optionalBorder_->getItems(edge)) {
              if (optionalItemId == itemId) {
                return true;
              }
            }
          }

          for (const auto &specificCase : optionalBorder_->getSpecificCases()) {
            if (specificCase.getToReplaceId() == itemId ||
                specificCase.getWithId() == itemId) {
              return true;
            }
          }

          return false;
        })();
    if (isGroundItem || isOptionalItem) {
      continue;
    }

    ownedItemIds_.erase(itemId);
    registry_.unregisterItemBinding(itemId, this);
  }
}

void GroundBrush::setOptionalBorder(BorderBlock border, bool soloOptional) {
  soloOptionalBorder_ = soloOptional;
  registry_.registerBorderBlockMetadata(border);

  const auto registerOwnedItem = [this](uint32_t itemId) {
    if (itemId == 0 || itemId > std::numeric_limits<uint16_t>::max()) {
      return;
    }

    const auto ownedItemId = static_cast<uint16_t>(itemId);
    ownedItemIds_.insert(ownedItemId);
    registry_.registerItemBinding(ownedItemId, this);
  };

  for (size_t i = 0; i < BorderBlock::kEdgeTypeCount; ++i) {
    const auto edge = static_cast<EdgeType>(i);
    if (!border.hasItemsFor(edge)) {
      continue;
    }
    for (const auto &[itemId, _] : border.getItems(edge)) {
      registerOwnedItem(itemId);
    }
  }

  for (const auto &specificCase : border.getSpecificCases()) {
    registerOwnedItem(specificCase.getToReplaceId());
    registerOwnedItem(specificCase.getWithId());
  }

  optionalBorder_ = std::move(border);
}

uint16_t GroundBrush::getPreviewItemId() const {
  return groundItems_.empty() ? 0 : groundItems_.front().first;
}

bool GroundBrush::placeGroundTile(Domain::Tile &tile,
                                  const DrawContext &ctx) const {
  if (groundItems_.empty()) {
    return false;
  }

  if (shouldSkipAltGroundPlacement(registry_, tile, ctx)) {
    return false;
  }

  const uint16_t itemId = selectWeightedItem(groundItems_);
  if (itemId == 0) {
    return false;
  }

  uint16_t placementItemId = itemId;
  if (const auto *metadata = registry_.getBorderItemMetadata(itemId);
      metadata && metadata->groundEquivalent != 0) {
    placementItemId = metadata->groundEquivalent;
  }

  auto groundItem = Types::createTypedItem(ctx, placementItemId);
  tile.setGround(std::move(groundItem));
  tile.setGroundBrushId(ctx.ownerBrushId);
  return true;
}

void GroundBrush::eraseFromTile(Domain::Tile &tile) const {
  if (const auto *ground = tile.getGround(); ground && ownsItem(ground)) {
    tile.removeGround();
  }
  tile.setGroundBrushId(InvalidBrushId);

  tile.removeItemsIf([this](const Domain::Item *item) {
    return item && isBorderItem(item->getServerId());
  });
  tile.setOptionalBorder(false);
}

void GroundBrush::rebuildAround(Domain::ChunkedMap &map,
                                const Domain::Position &center) const {
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      rebuildTile(map, {center.x + dx, center.y + dy, center.z});
    }
  }
}

void GroundBrush::rebuildTile(Domain::ChunkedMap &map,
                              const Domain::Position &pos) const {
  auto *tile = map.getTile(pos);
  bool createdTile = false;
  if (!tile || !tile->hasGround()) {
    bool canBuildOuterZilch = false;
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0) continue;
        const auto *neighborTile = map.getTile(pos.x + dx, pos.y + dy, pos.z);
        const auto *neighborBrush =
            neighborTile ? resolveGroundBrush(registry_, *neighborTile) : nullptr;
        if (neighborBrush && neighborBrush->hasOuterZilchBorderRule()) {
          canBuildOuterZilch = true;
          break;
        }
      }
      if (canBuildOuterZilch) break;
    }

    if (!canBuildOuterZilch) {
      return;
    }

    if (!tile) {
      tile = map.getOrCreateTile(pos);
      createdTile = tile != nullptr;
    }

    if (!tile) {
      return;
    }
  }

  const auto *brush = resolveGroundBrush(registry_, *tile);
  (brush ? brush : this)->updateBorderItems(map, *tile);

  if (createdTile && tile->isEmpty()) {
    map.removeTile(pos);
  }
}

bool GroundBrush::connectsTo(const GroundBrush *other) const {
  if (other == nullptr) {
    return false;
  }
  if (other == this) {
    return true;
  }

  const auto otherName = normalizeName(other->getName());
  const bool listed =
      friendNames_.contains("all") || friendNames_.contains(otherName);
  return listed ? !hateFriends_ : hateFriends_;
}

bool GroundBrush::hasDrawableGroundItem(uint16_t itemId) const {
  return std::ranges::any_of(groundItems_, [itemId](const auto &entry) {
    return entry.first == itemId && entry.second > 0;
  });
}

bool GroundBrush::hasOuterZilchBorderRule() const {
  return std::ranges::any_of(borderRules_, [](const BorderRule &rule) {
    return rule.outer && rule.targetNone;
  });
}

uint16_t GroundBrush::selectWeightedItem(
    const std::vector<std::pair<uint16_t, uint32_t>> &items) const {
  if (items.empty()) {
    return 0;
  }

  std::vector<uint32_t> weights;
  weights.reserve(items.size());
  std::vector<uint16_t> ids;
  ids.reserve(items.size());
  for (const auto &[id, chance] : items) {
    if (chance == 0) {
      continue;
    }
    weights.push_back(chance);
    ids.push_back(id);
  }

  if (weights.empty()) {
    return 0;
  }

  const auto selected = WeightedSelection::select(registry_.getRng(), weights);
  return selected ? ids[*selected] : ids.front();
}

bool GroundBrush::isBorderItem(uint16_t itemId) const {
  return ownedItemIds_.contains(itemId) && !hasDrawableGroundItem(itemId);
}

} // namespace MapEditor::Brushes
