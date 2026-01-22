#include "DragPreviewProvider.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Creature.h"
#include "Domain/Item.h"
#include "Domain/Spawn.h"
#include "Domain/Tile.h"

namespace MapEditor::Services::Preview {

DragPreviewProvider::DragPreviewProvider(
    const Selection::SelectionService &selectionService,
    Domain::ChunkedMap *map, const Domain::Position &dragStartPos)
    : selection_service_(&selectionService), map_(map),
      dragStartPos_(dragStartPos), currentPos_(dragStartPos) {
  buildPreview();
}

bool DragPreviewProvider::isActive() const {
  return selection_service_ != nullptr && !selection_service_->isEmpty() &&
         map_ != nullptr;
}

Domain::Position DragPreviewProvider::getAnchorPosition() const {
  return currentPos_;
}

const std::vector<PreviewTileData> &DragPreviewProvider::getTiles() const {
  return tiles_;
}

PreviewBounds DragPreviewProvider::getBounds() const { return bounds_; }

void DragPreviewProvider::updateCursorPosition(const Domain::Position &cursor) {
  currentPos_ = cursor;
}

void DragPreviewProvider::buildPreview() {
  tiles_.clear();
  bounds_ = PreviewBounds{};

  if (!selection_service_ || selection_service_->isEmpty() || !map_) {
    return;
  }

  bool first = true;
  auto entries = selection_service_->getAllEntries();

  for (const auto &entry : entries) {
    // Use entity type from new SelectionEntry
    auto type = entry.getType();

    if (type == Domain::Selection::EntityType::Ground) {
      // Ground selection means whole tile - add all items on tile
      addTileItems(entry.getPosition());
    } else if (type == Domain::Selection::EntityType::Item) {
      // Item selection - add single item
      if (entry.entity_ptr) {
        auto *item = static_cast<const Domain::Item *>(entry.entity_ptr);
        addSingleItem(entry.getPosition(), item);
      }
    } else if (type == Domain::Selection::EntityType::Creature) {
      // Creature selection - add creature outfit
      if (entry.entity_ptr) {
        auto *creature = static_cast<const Domain::Creature *>(entry.entity_ptr);
        addCreature(entry.getPosition(), creature);
      }
    } else if (type == Domain::Selection::EntityType::Spawn) {
      // Spawn selection - add spawn indicator
      addSpawn(entry.getPosition());
    }
  }

  // Build bounds from tiles
  for (const auto &tile : tiles_) {
    if (first) {
      bounds_.minX = bounds_.maxX = tile.relativePosition.x;
      bounds_.minY = bounds_.maxY = tile.relativePosition.y;
      bounds_.minZ = bounds_.maxZ = tile.relativePosition.z;
      first = false;
    } else {
      bounds_.expand(tile.relativePosition);
    }
  }
}

void DragPreviewProvider::addTileItems(const Domain::Position &pos) {
  auto *tile = map_->getTile(pos);
  if (!tile)
    return;

  Domain::Position relPos;
  relPos.x = pos.x - dragStartPos_.x;
  relPos.y = pos.y - dragStartPos_.y;
  relPos.z = static_cast<int16_t>(pos.z - dragStartPos_.z);

  PreviewTileData previewTile;
  previewTile.relativePosition = relPos;

  // Add ground
  if (tile->hasGround()) {
    const auto *ground = tile->getGround();
    if (ground) {
      previewTile.addItem(ground->getServerId(),
                          static_cast<uint16_t>(ground->getSubtype()));
    }
  }

  // Add all items
  for (const auto &itemPtr : tile->getItems()) {
    if (itemPtr) {
      previewTile.addItem(itemPtr->getServerId(),
                          static_cast<uint16_t>(itemPtr->getSubtype()));
    }
  }

  if (!previewTile.empty()) {
    tiles_.push_back(std::move(previewTile));
  }
}

void DragPreviewProvider::addSingleItem(const Domain::Position &pos,
                                        const Domain::Item *item) {
  if (!item)
    return;

  Domain::Position relPos;
  relPos.x = pos.x - dragStartPos_.x;
  relPos.y = pos.y - dragStartPos_.y;
  relPos.z = static_cast<int16_t>(pos.z - dragStartPos_.z);

  // Check if tile already exists at this position
  for (auto &existingTile : tiles_) {
    if (existingTile.relativePosition.x == relPos.x &&
        existingTile.relativePosition.y == relPos.y &&
        existingTile.relativePosition.z == relPos.z) {
      existingTile.addItem(item->getServerId(),
                           static_cast<uint16_t>(item->getSubtype()));
      return;
    }
  }

  // Create new tile
  PreviewTileData previewTile;
  previewTile.relativePosition = relPos;
  previewTile.addItem(item->getServerId(),
                      static_cast<uint16_t>(item->getSubtype()));
  tiles_.push_back(std::move(previewTile));
}

void DragPreviewProvider::addCreature(const Domain::Position &pos,
                                      const Domain::Creature *creature) {
  if (!creature)
    return;

  Domain::Position relPos;
  relPos.x = pos.x - dragStartPos_.x;
  relPos.y = pos.y - dragStartPos_.y;
  relPos.z = static_cast<int16_t>(pos.z - dragStartPos_.z);

  // Check if tile already exists at this position
  for (auto &existingTile : tiles_) {
    if (existingTile.relativePosition.x == relPos.x &&
        existingTile.relativePosition.y == relPos.y &&
        existingTile.relativePosition.z == relPos.z) {
      existingTile.creature_name = creature->name;
      return;
    }
  }

  // Create new tile with creature name
  PreviewTileData previewTile;
  previewTile.relativePosition = relPos;
  previewTile.creature_name = creature->name;
  tiles_.push_back(std::move(previewTile));
}

void DragPreviewProvider::addSpawn(const Domain::Position &pos) {
  Domain::Position relPos;
  relPos.x = pos.x - dragStartPos_.x;
  relPos.y = pos.y - dragStartPos_.y;
  relPos.z = static_cast<int16_t>(pos.z - dragStartPos_.z);

  // Check if tile already exists at this position
  for (auto &existingTile : tiles_) {
    if (existingTile.relativePosition.x == relPos.x &&
        existingTile.relativePosition.y == relPos.y &&
        existingTile.relativePosition.z == relPos.z) {
      existingTile.has_spawn = true;
      return;
    }
  }

  // Create new tile with spawn indicator
  PreviewTileData previewTile;
  previewTile.relativePosition = relPos;
  previewTile.has_spawn = true;
  tiles_.push_back(std::move(previewTile));
}

} // namespace MapEditor::Services::Preview
