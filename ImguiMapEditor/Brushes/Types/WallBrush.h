#pragma once

#include "Brushes/Core/BrushBase.h"
#include "Brushes/Data/WallNode.h"
#include "Brushes/Enums/BrushEnums.h"
#include "Services/Preview/PreviewTypes.h"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace MapEditor::Brushes {

class BrushRegistry;

class WallBrush : public BrushBase {
public:
  struct DirectPlacementResult {
    bool changed = false;
    bool requiresRebuild = false;
  };

  WallBrush(std::string name, uint32_t lookId, BrushRegistry &registry);

  BrushType getType() const override { return BrushType::Wall; }
  bool needsBorderUpdate() const override { return true; }

  void draw(Domain::ChunkedMap &map, Domain::Tile *tile,
            const DrawContext &ctx) override;
  void undraw(Domain::ChunkedMap &map, Domain::Tile *tile) override;
  bool ownsItem(const Domain::Item *item) const override;

  void addWallItem(WallAlign align, uint16_t itemId, uint32_t chance);
  void addDoorItem(WallAlign align, DoorNode door);
  void addRedirectName(const std::string &name);
  void addFriendName(const std::string &name);
  void addWallHateMeItem(uint16_t itemId);

  uint16_t getPreviewItemId() const;
  void rebuildAround(Domain::ChunkedMap &map, const Domain::Position &center) const;
  void rebuildTiles(Domain::ChunkedMap &map,
                    std::span<const Domain::Position> positions) const;
  void rebuildTile(Domain::ChunkedMap &map, const Domain::Position &pos) const;
  DirectPlacementResult placeWallTile(Domain::Tile &tile,
                                      const DrawContext &ctx) const;
  void eraseFromTile(Domain::Tile &tile) const;
  std::vector<Services::Preview::PreviewTileData>
  buildPreviewTiles(const Domain::ChunkedMap &map,
                    const Domain::Position &anchor,
                    std::span<const std::pair<int, int>> offsets) const;

  bool canApplyDoor(const Domain::Tile &tile, DoorType type, bool open,
                    bool preferLocked) const;
  bool applyDoor(Domain::ChunkedMap &map, Domain::Tile &tile, DoorType type,
                 bool open, bool preferLocked,
                 BrushId ownerBrushId = InvalidBrushId) const;
  bool removeDoor(Domain::ChunkedMap &map, Domain::Tile &tile,
                  const Domain::Item *preferredItem = nullptr) const;
  bool switchDoor(Domain::ChunkedMap &map, Domain::Tile &tile,
                  const Domain::Item *preferredItem,
                  bool preferLocked) const;
  std::optional<WallAlign> getAlignmentForItem(uint16_t itemId) const;
  uint16_t getWallItemForAlign(WallAlign align) const;
  std::optional<uint16_t> findNextWallVariant(uint16_t currentItemId) const;
  std::optional<DoorNode> getDoorItemForAlign(WallAlign align, DoorType type,
                                              bool open,
                                              bool preferLocked) const;
  std::optional<DoorNode> findDoorForItem(uint16_t itemId) const;
  const std::vector<const WallBrush *> &getRedirectBrushes() const;
  BrushRegistry &getBrushRegistry() const { return registry_; }
  static const WallBrush *resolveWallBrushForItem(const Domain::Item &item,
                                                   BrushRegistry &registry);
  bool connectsTo(const IBrush *brush) const;
  const std::unordered_set<uint16_t> &getWallHateMeItems() const {
    return wallHateMeItems_;
  }

private:
  template <typename Fn>
  bool visitWallRedirectChain(Fn &&fn) const {
    std::vector<const WallBrush *> pending{this};
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

  void updateConsecutiveDecorations(
      Domain::Tile &tile, Domain::Item *baseItem,
      const std::function<uint16_t(const WallBrush &, const Domain::Item &)> &resolveItemId) const;

  static constexpr size_t kWallAlignCount = 17;
  bool isWallGroupItem(uint16_t itemId) const;
  std::optional<WallAlign> findAlignmentForItem(uint16_t itemId) const;
  std::optional<WallAlign> findTileAlignment(const Domain::Tile &tile) const;
  uint16_t selectWallItem(WallAlign align) const;
  std::optional<DoorNode> selectDoorItem(WallAlign align, DoorType type,
                                         bool open, bool preferLocked) const;
  void rebuildNeighbors(Domain::ChunkedMap &map,
                        const Domain::Position &center) const;
  bool tileHasWallGroup(const Domain::Tile *tile) const;

  BrushRegistry &registry_;
  std::array<WallNode, kWallAlignCount> wallNodes_{};
  std::array<std::vector<DoorNode>, kWallAlignCount> doorNodes_{};
  std::unordered_map<uint16_t, WallAlign> itemAlignments_;
  std::unordered_map<uint16_t, DoorNode> doorNodesByItemId_;
  std::unordered_set<uint16_t> ownedItemIds_;
  std::unordered_set<std::string> redirectNames_;
  std::unordered_set<std::string> friendNames_;   // non-redirect friends
  std::unordered_set<uint16_t> wallHateMeItems_;  // items that break wall connections
  std::string normalizedName_;
  mutable std::vector<const WallBrush *> redirectBrushesCache_;
  mutable bool redirectBrushesCacheDirty_ = true;
};

} // namespace MapEditor::Brushes
