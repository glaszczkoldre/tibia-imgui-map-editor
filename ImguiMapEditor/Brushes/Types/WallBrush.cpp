#include "WallBrush.h"

#include "Brushes/BrushRegistry.h"
#include "Brushes/Types/BrushUtils.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Item.h"
#include "Domain/Tile.h"
#include "Services/ClientDataService.h"
#include "Services/Brushes/WallLookupService.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <functional>
#include <unordered_set>

namespace MapEditor::Brushes {

namespace {

std::string normalizeName(const std::string &value) {
  std::string normalized = value;
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return normalized;
}

constexpr std::array<std::tuple<int, int, WallNeighbor>, 4> kWallNeighbors{{
    {0, -1, WallNeighbor::North},
    {-1, 0, WallNeighbor::West},
    {1, 0, WallNeighbor::East},
    {0, 1, WallNeighbor::South},
}};

template <typename Fn>
bool visitWallRedirectChain(const WallBrush &root, Fn &&fn) {
  std::vector<const WallBrush *> pending{&root};
  std::unordered_set<const WallBrush *> visited;

  while (!pending.empty()) {
    const auto *brush = pending.back();
    pending.pop_back();
    if (!brush || !visited.insert(brush).second) {
      continue;
    }

    if (std::invoke(fn, *brush)) {
      return true;
    }

    for (const auto *redirectBrush : brush->getRedirectBrushes()) {
      if (redirectBrush && !visited.contains(redirectBrush)) {
        pending.push_back(redirectBrush);
      }
    }
  }

  return false;
}


Domain::Item *resolveDoorItem(Domain::Tile &tile,
                              const Domain::Item *preferredItem,
                              const WallBrush &brush) {
  if (preferredItem) {
    for (const auto &item : tile.getItems()) {
      if (item.get() == preferredItem &&
          brush.findDoorForItem(item->getServerId()).has_value()) {
        return item.get();
      }
    }
  }

  for (auto it = tile.getItems().rbegin(); it != tile.getItems().rend(); ++it) {
    if (*it && brush.findDoorForItem((*it)->getServerId()).has_value()) {
      return it->get();
    }
  }

  return nullptr;
}

template <typename Resolver>
void updateConsecutiveDecorations(Domain::Tile &tile, Domain::Item *baseItem,
                                  BrushRegistry &registry,
                                  Resolver &&resolveItemId) {
  if (!baseItem) {
    return;
  }

  size_t baseIndex = tile.getItemCount();
  for (size_t index = 0; index < tile.getItemCount(); ++index) {
    if (tile.getItem(index) == baseItem) {
      baseIndex = index;
      break;
    }
  }

  if (baseIndex == tile.getItemCount()) {
    return;
  }

  std::vector<const Domain::Item *> itemsToRemove;
  for (size_t index = baseIndex + 1; index < tile.getItemCount(); ++index) {
    auto *item = tile.getItem(index);
    if (!item) {
      continue;
    }

    const auto *itemBrush = WallBrush::resolveWallBrushForItem(*item, registry);
    auto *decorationBrush = dynamic_cast<const WallBrush *>(itemBrush);
    if (!decorationBrush ||
        decorationBrush->getType() != BrushType::WallDecoration) {
      break;
    }

    if (const auto itemId = std::invoke(resolveItemId, *decorationBrush, *item);
        itemId != 0) {
      const auto ownerBrushId =
          item->getOwnerBrushId() != InvalidBrushId
              ? item->getOwnerBrushId()
              : registry.getBrushId(decorationBrush);
      Types::updateItemVisuals(*item, registry, itemId, ownerBrushId);
      continue;
    }

    itemsToRemove.push_back(item);
  }

  if (!itemsToRemove.empty()) {
    tile.removeItemsIf([&itemsToRemove](const Domain::Item *item) {
      return std::find(itemsToRemove.begin(), itemsToRemove.end(), item) !=
             itemsToRemove.end();
    });
  }
}

} // namespace

const WallBrush *WallBrush::resolveWallBrushForItem(const Domain::Item &item,
                                                     BrushRegistry &registry) {
  if (item.getOwnerBrushId() != InvalidBrushId) {
    if (const auto *brush =
            dynamic_cast<const WallBrush *>(registry.getBrushById(item.getOwnerBrushId()))) {
      return brush;
    }
  }

  for (auto *brush : registry.getBrushesForItem(item.getServerId())) {
    if (const auto *wallBrush = dynamic_cast<const WallBrush *>(brush)) {
      return wallBrush;
    }
  }

  return nullptr;
}

WallBrush::WallBrush(std::string name, uint32_t lookId, BrushRegistry &registry)
    : BrushBase(std::move(name), lookId, true), registry_(registry),
      normalizedName_(normalizeName(getName())) {}

void WallBrush::draw(Domain::ChunkedMap &map, Domain::Tile *tile,
                     const DrawContext &ctx) {
  if (!tile) {
    return;
  }

  const auto placement = placeWallTile(*tile, ctx);
  if (!placement.changed) {
    return;
  }

  if (placement.requiresRebuild) {
    rebuildAround(map, tile->getPosition());
  }
}

void WallBrush::undraw(Domain::ChunkedMap &map, Domain::Tile *tile) {
  if (!tile) {
    return;
  }
  eraseFromTile(*tile);
  rebuildAround(map, tile->getPosition());
}

WallBrush::DirectPlacementResult
WallBrush::placeWallTile(Domain::Tile &tile, const DrawContext &ctx) const {
  if (ctx.specialAction) {
    for (const auto &item : tile.getItems()) {
      if (!item) {
        continue;
      }

      const auto *itemBrush = resolveWallBrushForItem(*item, registry_);
      if (itemBrush != this) {
        continue;
      }

      const auto replacementId = findNextWallVariant(item->getServerId());
      if (!replacementId.has_value()) {
        return {};
      }

      Types::updateItemVisuals(*item, registry_, *replacementId,
                               ctx.ownerBrushId != InvalidBrushId
                                   ? ctx.ownerBrushId
                                   : item->getOwnerBrushId());
      tile.markDirty();
      return {.changed = true, .requiresRebuild = false};
    }
  }

  tile.removeItemsIf([this](const Domain::Item *item) { return ownsItem(item); });
  if (const auto itemId = selectWallItem(WallAlign::Horizontal); itemId != 0) {
    tile.addItem(Types::createTypedItem(ctx, itemId));
  }
  const bool deferNeighborRebuild =
      ctx.isDragging && !ctx.specialAction;
  return {.changed = true, .requiresRebuild = !deferNeighborRebuild};
}

void WallBrush::eraseFromTile(Domain::Tile &tile) const {
  tile.removeItemsIf([this](const Domain::Item *item) { return ownsItem(item); });
}

bool WallBrush::ownsItem(const Domain::Item *item) const {
  return item && ownedItemIds_.contains(item->getServerId());
}

void WallBrush::addWallItem(WallAlign align, uint16_t itemId, uint32_t chance) {
  wallNodes_[static_cast<size_t>(align)].addItem(itemId, chance);
  itemAlignments_[itemId] = align;
  ownedItemIds_.insert(itemId);
  registry_.registerItemBinding(itemId, this);
  if (lookId_ == 0) {
    lookId_ = itemId;
  }
}

void WallBrush::addDoorItem(WallAlign align, DoorNode door) {
  door.alignment = align;
  for (const auto itemId : door.items) {
    doorNodesByItemId_.try_emplace(itemId, door);
    itemAlignments_[itemId] = align;
    ownedItemIds_.insert(itemId);
    registry_.registerItemBinding(itemId, this);
    if (lookId_ == 0) {
      lookId_ = itemId;
    }
  }
  doorNodes_[static_cast<size_t>(align)].push_back(std::move(door));
}

void WallBrush::addRedirectName(const std::string &name) {
  redirectNames_.insert(normalizeName(name));
  redirectBrushesCacheDirty_ = true;
}

void WallBrush::addFriendName(const std::string &name) {
  friendNames_.insert(normalizeName(name));
}

void WallBrush::addWallHateMeItem(uint16_t itemId) {
  wallHateMeItems_.insert(itemId);
}

uint16_t WallBrush::getPreviewItemId() const {
  if (const auto itemId = selectWallItem(WallAlign::Horizontal); itemId != 0) {
    return itemId;
  }
  return lookId_;
}

std::optional<WallAlign> WallBrush::getAlignmentForItem(uint16_t itemId) const {
  return findAlignmentForItem(itemId);
}

uint16_t WallBrush::getWallItemForAlign(WallAlign align) const {
  return selectWallItem(align);
}

std::optional<DoorNode> WallBrush::getDoorItemForAlign(WallAlign align,
                                                       DoorType type, bool open,
                                                       bool preferLocked) const {
  return selectDoorItem(align, type, open, preferLocked);
}

void WallBrush::rebuildAround(Domain::ChunkedMap &map,
                              const Domain::Position &center) const {
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      rebuildTile(map, {center.x + dx, center.y + dy, center.z});
    }
  }
}

void WallBrush::rebuildTiles(
    Domain::ChunkedMap &map, std::span<const Domain::Position> positions) const {
  for (const auto &pos : positions) {
    rebuildTile(map, pos);
  }
}

void WallBrush::rebuildNeighbors(Domain::ChunkedMap &map,
                                 const Domain::Position &center) const {
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) {
        continue;
      }

      rebuildTile(map, {center.x + dx, center.y + dy, center.z});
    }
  }
}

bool WallBrush::canApplyDoor(const Domain::Tile &tile, DoorType type, bool open,
                             bool preferLocked) const {
  for (const auto &item : tile.getItems()) {
    if (!item) {
      continue;
    }

    const auto *itemBrush = resolveWallBrushForItem(*item, registry_);
    if (!itemBrush || itemBrush->getType() == BrushType::WallDecoration ||
        !connectsTo(itemBrush)) {
      continue;
    }

    const auto alignment = findAlignmentForItem(item->getServerId());
    if (!alignment.has_value()) {
      continue;
    }

    if (selectDoorItem(*alignment, type, open, preferLocked).has_value()) {
      return true;
    }
  }

  return false;
}

bool WallBrush::applyDoor(Domain::ChunkedMap &map, Domain::Tile &tile,
                          DoorType type, bool open, bool preferLocked,
                          BrushId ownerBrushId) const {
  const auto pos = tile.getPosition();
  const auto resolvedOwnerBrushId =
      ownerBrushId != InvalidBrushId ? ownerBrushId : registry_.getBrushId(this);
  auto *baseWallItem = [&]() -> Domain::Item * {
    for (const auto &item : tile.getItems()) {
      if (!item) {
        continue;
      }

      const auto *itemBrush = resolveWallBrushForItem(*item, registry_);
      if (!itemBrush || itemBrush->getType() == BrushType::WallDecoration ||
          !connectsTo(itemBrush)) {
        continue;
      }

      if (const auto alignment = findAlignmentForItem(item->getServerId());
          alignment && selectDoorItem(*alignment, type, open, preferLocked).has_value()) {
        return item.get();
      }
    }

    return nullptr;
  }();

  if (!baseWallItem) {
    return false;
  }

  const auto alignment = findAlignmentForItem(baseWallItem->getServerId());
  if (!alignment.has_value()) {
    return false;
  }

  const auto baseDoor = selectDoorItem(*alignment, type, open, preferLocked);
  if (!baseDoor.has_value()) {
    return false;
  }

  Types::updateItemVisuals(*baseWallItem, registry_,
                           static_cast<uint16_t>(baseDoor->getItem()),
                           resolvedOwnerBrushId);

  updateConsecutiveDecorations(
      tile, baseWallItem, registry_,
      [alignment = *alignment, type, open, preferLocked](
          const WallBrush &decorationBrush, const Domain::Item &) -> uint16_t {
        if (const auto decorationDoor =
                decorationBrush.getDoorItemForAlign(alignment, type, open,
                                                    preferLocked);
            decorationDoor.has_value()) {
          return static_cast<uint16_t>(decorationDoor->getItem());
        }

        return 0;
      });

  tile.markDirty();
  rebuildAround(map, pos);
  return true;
}

bool WallBrush::removeDoor(Domain::ChunkedMap &map, Domain::Tile &tile,
                           const Domain::Item *preferredItem) const {
  auto *targetItem = resolveDoorItem(tile, preferredItem, *this);
  if (!targetItem) {
    return false;
  }

  const auto currentDoor = findDoorForItem(targetItem->getServerId());
  if (!currentDoor) {
    return false;
  }

  const auto alignment =
      findAlignmentForItem(targetItem->getServerId()).value_or(currentDoor->alignment);
  const auto replacementId = selectWallItem(alignment);
  if (replacementId == 0) {
    return false;
  }

  const auto ownerBrushId = targetItem->getOwnerBrushId() != InvalidBrushId
                                ? targetItem->getOwnerBrushId()
                                : registry_.getBrushId(this);
  Types::updateItemVisuals(*targetItem, registry_, replacementId, ownerBrushId);

  updateConsecutiveDecorations(
      tile, targetItem, registry_,
      [alignment](const WallBrush &decorationBrush,
                  const Domain::Item &) -> uint16_t {
        return decorationBrush.getWallItemForAlign(alignment);
      });

  tile.markDirty();
  rebuildAround(map, tile.getPosition());
  return true;
}

bool WallBrush::switchDoor(Domain::ChunkedMap &map, Domain::Tile &tile,
                           const Domain::Item *preferredItem,
                           bool preferLocked) const {
  auto *targetItem = resolveDoorItem(tile, preferredItem, *this);

  if (!targetItem) {
    return false;
  }

  const auto currentDoor = findDoorForItem(targetItem->getServerId());
  if (!currentDoor) {
    return false;
  }

  const auto alignment =
      findAlignmentForItem(targetItem->getServerId()).value_or(currentDoor->alignment);
  const auto replacement =
      selectDoorItem(alignment, currentDoor->type, !currentDoor->isOpen, preferLocked);
  if (!replacement || replacement->getItem() == 0) {
    return false;
  }

  const auto replacementId = static_cast<uint16_t>(replacement->getItem());
  Types::updateItemVisuals(*targetItem, registry_, replacementId,
                           targetItem->getOwnerBrushId());

  updateConsecutiveDecorations(
      tile, targetItem, registry_,
      [alignment, replacement, preferLocked](const WallBrush &decorationBrush,
                                             const Domain::Item &) -> uint16_t {
        if (const auto replacementDoor =
                decorationBrush.getDoorItemForAlign(alignment, replacement->type,
                                                    replacement->isOpen,
                                                    preferLocked);
            replacementDoor.has_value()) {
          return static_cast<uint16_t>(replacementDoor->getItem());
        }

        return 0;
      });

  tile.markDirty();
  rebuildAround(map, tile.getPosition());
  return true;
}

std::optional<DoorNode> WallBrush::findDoorForItem(uint16_t itemId) const {
  std::optional<DoorNode> foundDoor;
  visitWallRedirectChain(*this, [&](const WallBrush &brush) {
    if (const auto it = brush.doorNodesByItemId_.find(itemId);
        it != brush.doorNodesByItemId_.end()) {
      foundDoor = it->second;
      return true;
    }
    return false;
  });
  return foundDoor;
}

const std::vector<const WallBrush *> &WallBrush::getRedirectBrushes() const {
  if (!redirectBrushesCacheDirty_) {
    return redirectBrushesCache_;
  }

  redirectBrushesCache_.clear();
  redirectBrushesCacheDirty_ = false;
  if (redirectNames_.empty()) {
    return redirectBrushesCache_;
  }

  for (auto *brush : registry_.getAllBrushes()) {
    auto *wallBrush = dynamic_cast<const WallBrush *>(brush);
    if (!wallBrush || wallBrush == this) {
      continue;
    }

    if (redirectNames_.contains(normalizeName(wallBrush->getName()))) {
      redirectBrushesCache_.push_back(wallBrush);
    }
  }
  return redirectBrushesCache_;
}

bool WallBrush::isWallGroupItem(uint16_t itemId) const {
  for (auto *brush : registry_.getBrushesForItem(itemId)) {
    auto *wallBrush = dynamic_cast<const WallBrush *>(brush);
    if (wallBrush && connectsTo(wallBrush)) {
      return true;
    }
  }

  return findAlignmentForItem(itemId).has_value();
}

std::optional<WallAlign> WallBrush::findAlignmentForItem(uint16_t itemId) const {
  std::optional<WallAlign> alignment;
  visitWallRedirectChain(*this, [&](const WallBrush &brush) {
    if (const auto it = brush.itemAlignments_.find(itemId);
        it != brush.itemAlignments_.end()) {
      alignment = it->second;
      return true;
    }
    return false;
  });
  return alignment;
}

std::optional<uint16_t> WallBrush::findNextWallVariant(uint16_t currentItemId) const {
  const auto alignment = findAlignmentForItem(currentItemId);
  if (!alignment.has_value()) {
    return std::nullopt;
  }

  std::vector<uint16_t> candidates;
  visitWallRedirectChain(*this, [&](const WallBrush &brush) {
    for (const auto &[candidateId, _] :
         brush.wallNodes_[static_cast<size_t>(*alignment)].getItems()) {
      if (candidateId == 0 ||
          std::find(candidates.begin(), candidates.end(), candidateId) !=
              candidates.end()) {
        continue;
      }

      candidates.push_back(static_cast<uint16_t>(candidateId));
    }

    return false;
  });

  if (candidates.size() <= 1) {
    return std::nullopt;
  }

  const auto currentIt =
      std::find(candidates.begin(), candidates.end(), currentItemId);
  if (currentIt == candidates.end()) {
    return candidates.front();
  }

  auto nextIt = std::next(currentIt);
  if (nextIt == candidates.end()) {
    nextIt = candidates.begin();
  }

  return *nextIt;
}

std::optional<WallAlign>
WallBrush::findTileAlignment(const Domain::Tile &tile) const {
  for (const auto &item : tile.getItems()) {
    if (!item) {
      continue;
    }
    if (const auto alignment = findAlignmentForItem(item->getServerId())) {
      return alignment;
    }
  }
  return std::nullopt;
}

uint16_t WallBrush::selectWallItem(WallAlign align) const {
  uint16_t itemId = 0;
  visitWallRedirectChain(*this, [&](const WallBrush &brush) {
    itemId = brush.wallNodes_[static_cast<size_t>(align)].getRandomItem();
    return itemId != 0;
  });
  return itemId;
}

std::optional<DoorNode> WallBrush::selectDoorItem(WallAlign align,
                                                  DoorType type, bool open,
                                                  bool preferLocked) const {
  std::optional<DoorNode> bestMatch;
  int bestRank = -1;

  visitWallRedirectChain(*this, [&](const WallBrush &brush) {
    const auto &doors = brush.doorNodes_[static_cast<size_t>(align)];

    for (const auto &door : doors) {
      if (door.type != type) {
        continue;
      }

      const int rank = (door.isOpen == open)
                           ? ((!preferLocked || door.isLocked) ? 3 : 2)
                           : 1;
      if (rank > bestRank) {
        bestMatch = door;
        bestRank = rank;
        if (bestRank == 3) {
          return true;
        }
      }
    }

    return false;
  });

  return bestMatch;
}

void WallBrush::rebuildTile(Domain::ChunkedMap &map,
                            const Domain::Position &pos) const {
  auto *tile = map.getTile(pos);
  if (!tile || !tileHasWallGroup(tile)) {
    return;
  }

  DoorType currentDoorType = DoorType::Undefined;
  bool isOpen = false;
  bool isLocked = false;
  std::optional<WallAlign> currentAlignment;
  BrushId currentOwnerBrushId = registry_.getBrushId(this);
  Domain::Item *baseItem = nullptr;
  std::vector<Domain::Item *> duplicateBaseItems;
  for (const auto &item : tile->getItems()) {
    if (!item) {
      continue;
    }

    const auto *itemBrush = resolveWallBrushForItem(*item, registry_);
    if (!itemBrush || itemBrush->getType() == BrushType::WallDecoration ||
        !connectsTo(itemBrush)) {
      continue;
    }

    if (!baseItem) {
      baseItem = item.get();
      currentAlignment = findAlignmentForItem(item->getServerId());
    } else {
      duplicateBaseItems.push_back(item.get());
    }

    if (const auto door = findDoorForItem(item->getServerId())) {
      currentDoorType = door->type;
      isOpen = door->isOpen;
      isLocked = door->isLocked;
      if (item->getOwnerBrushId() != InvalidBrushId) {
        currentOwnerBrushId = item->getOwnerBrushId();
      }
    }
  }

  if (!baseItem) {
    return;
  }

  // Compute neighbor bitmask (same encoding as RME: N=1, W=2, E=4, S=8)
  WallNeighbor neighbors = WallNeighbor::None;
  for (const auto &[dx, dy, bit] : kWallNeighbors) {
    const auto *neighborTile = map.getTile(pos.x + dx, pos.y + dy, pos.z);
    if (tileHasWallGroup(neighborTile)) {
      neighbors |= bit;
    }
  }

  // Two-pass alignment: try full table first, then half table (RME parity)
  static const Services::Brushes::WallLookupService lookupService;
  const auto fullAlign = lookupService.getFullType(neighbors);
  const auto halfAlign = lookupService.getHalfType(neighbors);

  auto resolvedAlignment = fullAlign;
  uint16_t replacementId = 0;

  // If the tile has a door, try to find a matching door item
  if (currentDoorType != DoorType::Undefined) {
    if (currentAlignment) {
      if (const auto door =
              selectDoorItem(*currentAlignment, currentDoorType, isOpen, isLocked)) {
        replacementId = static_cast<uint16_t>(door->getItem());
        resolvedAlignment = *currentAlignment;
      }
    }

    if (replacementId == 0) {
      if (const auto door =
              selectDoorItem(fullAlign, currentDoorType, isOpen, isLocked)) {
        replacementId = static_cast<uint16_t>(door->getItem());
        resolvedAlignment = fullAlign;
      }
    }

    // Half table fallback for doors
    if (replacementId == 0 && halfAlign != fullAlign) {
      if (const auto door =
              selectDoorItem(halfAlign, currentDoorType, isOpen, isLocked)) {
        replacementId = static_cast<uint16_t>(door->getItem());
        resolvedAlignment = halfAlign;
      }
    }
  }

  // Wall item selection with two-pass fallback
  if (replacementId == 0) {
    replacementId = selectWallItem(fullAlign);
    resolvedAlignment = fullAlign;
  }
  if (replacementId == 0 && halfAlign != fullAlign) {
    replacementId = selectWallItem(halfAlign);
    resolvedAlignment = halfAlign;
  }
  if (replacementId == 0) {
    replacementId = selectWallItem(WallAlign::Horizontal);
    resolvedAlignment = WallAlign::Horizontal;
  }

  if (replacementId == 0) {
    return;
  }

  Types::updateItemVisuals(*baseItem, registry_, replacementId,
                           currentOwnerBrushId);

  updateConsecutiveDecorations(
      *tile, baseItem, registry_,
      [resolvedAlignment, currentDoorType, isOpen, isLocked](
          const WallBrush &decorationBrush, const Domain::Item &) -> uint16_t {
        if (currentDoorType != DoorType::Undefined) {
          if (const auto decorationDoor =
                  decorationBrush.getDoorItemForAlign(resolvedAlignment,
                                                      currentDoorType, isOpen,
                                                      isLocked);
              decorationDoor.has_value()) {
            return static_cast<uint16_t>(decorationDoor->getItem());
          }

          return 0;
        }

        return decorationBrush.getWallItemForAlign(resolvedAlignment);
      });

  if (!duplicateBaseItems.empty()) {
    tile->removeItemsIf([&duplicateBaseItems](const Domain::Item *item) {
      return std::find(duplicateBaseItems.begin(), duplicateBaseItems.end(),
                       item) != duplicateBaseItems.end();
    });
  }

  tile->markDirty();
}

bool WallBrush::connectsTo(const IBrush *brush) const {
  if (brush == this) {
    return true;
  }

  const auto *wallBrush = dynamic_cast<const WallBrush *>(brush);
  if (!wallBrush) {
    return false;
  }

  // Redirect friends (bidirectional)
  if (redirectNames_.contains(wallBrush->normalizedName_) ||
      wallBrush->redirectNames_.contains(normalizedName_)) {
    return true;
  }
  // Non-redirect friends (bidirectional)
  if (friendNames_.contains(wallBrush->normalizedName_) ||
      wallBrush->friendNames_.contains(normalizedName_)) {
    return true;
  }
  return false;
}

bool WallBrush::tileHasWallGroup(const Domain::Tile *tile) const {
  if (!tile) {
    return false;
  }

  for (const auto &item : tile->getItems()) {
    if (!item) {
      continue;
    }
    // WallHateMe items break wall connections
    if (wallHateMeItems_.contains(item->getServerId())) {
      continue;
    }
    if (isWallGroupItem(item->getServerId())) {
      return true;
    }
  }
  return false;
}

} // namespace MapEditor::Brushes
