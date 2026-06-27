#include "EraserBrush.h"
#include "Domain/ChunkedMap.h"
#include "Domain/Tile.h"
#include "Domain/ItemType.h"
#include "Brushes/BrushRegistry.h"
#include "Brushes/Types/GroundBrush.h"
#include "Services/ClientDataService.h"
#include <spdlog/spdlog.h>

namespace MapEditor::Brushes {

EraserBrush::EraserBrush() noexcept = default;

void EraserBrush::draw(Domain::ChunkedMap &map, Domain::Tile *tile,
                       const DrawContext &ctx) {
  if (!tile)
    return;

  // Erase ground if enabled
  if (eraseGround_ && tile->hasGround()) {
    tile->removeGround();
    tile->setOptionalBorder(false);
    tile->setGroundBrushId(InvalidBrushId);
    tile->setOptionalBorderBrushId(InvalidBrushId);
  }

  // Erase stacked items if enabled (preserving ground borders)
  if (eraseItems_) {
    tile->removeItemsIf([&](const Domain::Item *item) {
      if (!item) {
        return false;
      }

      // 1. Check if the ItemType is explicitly marked as a border
      const auto *type = item->getType();
      if (!type && ctx.clientData) {
        type = ctx.clientData->getItemTypeByServerId(item->getServerId());
      }
      if (type && type->isBorder()) {
        return false; // Preserve
      }

      // 2. Check if the item is a border according to the brush registry/metadata
      if (ctx.brushRegistry) {
        if (ctx.brushRegistry->getBorderItemMetadata(item->getServerId()) != nullptr) {
          return false; // Preserve
        }

        const GroundBrush *groundBrush = nullptr;
        for (auto *b : ctx.brushRegistry->getBrushesForItem(item->getServerId())) {
          if (auto *gb = dynamic_cast<const GroundBrush *>(b)) {
            groundBrush = gb;
            break;
          }
        }
        if (groundBrush && groundBrush->isBorderItem(item->getServerId())) {
          return false; // Preserve
        }
      }

      return true; // Erase other items
    });
  }

  // Erase creature if enabled
  if (eraseCreatures_ && tile->hasCreature()) {
    tile->removeCreature();
    tile->setCreatureBrushId(InvalidBrushId);
  }

  // Erase spawn if enabled
  if (eraseSpawns_ && tile->hasSpawn()) {
    tile->removeSpawn();
    tile->setSpawnBrushId(InvalidBrushId);
  }

  tile->setFlags(Domain::TileFlag::None);
  tile->setZoneBrushId(Domain::TileFlag::ProtectionZone, InvalidBrushId);
  tile->setZoneBrushId(Domain::TileFlag::NoPvp, InvalidBrushId);
  tile->setZoneBrushId(Domain::TileFlag::NoLogout, InvalidBrushId);
  tile->setZoneBrushId(Domain::TileFlag::PvpZone, InvalidBrushId);
  tile->setZoneBrushId(Domain::TileFlag::Refresh, InvalidBrushId);

  map.removeWaypointAt(tile->getPosition());
  tile->setWaypointBrushId(InvalidBrushId);
  tile->setHouseId(0);
  tile->setHouseBrushId(InvalidBrushId);
  tile->setHouseExitBrushId(InvalidBrushId);
  tile->setHouseExitHouseId(0);
  map.markChanged();

  spdlog::trace("[EraserBrush] Erased at ({},{},{})", tile->getPosition().x,
                tile->getPosition().y, tile->getPosition().z);
}

void EraserBrush::undraw(Domain::ChunkedMap& /*map*/, Domain::Tile* tile) {
  // Eraser doesn't have undraw - history system handles undo
  // This is intentionally empty
}

} // namespace MapEditor::Brushes
