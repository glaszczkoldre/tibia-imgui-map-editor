#pragma once

#include "Brushes/Core/BrushBase.h"
#include "Brushes/Data/DoodadAlternative.h"
#include "Brushes/Types/DoodadRedoBorderPlanner.h"
#include "Services/Preview/PreviewTypes.h"
#include "Utils/PositionUtils.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace MapEditor::Brushes {

class BrushRegistry;
class DoodadPlacementPlanner;

class DoodadBrush : public BrushBase {
public:
  struct EraseOptions {
    bool matchingBrushOnly = false;
    bool preserveComplexItems = true;
  };

  using DoodadLayout = std::vector<Services::Preview::PreviewTileData>;
  enum class PlacementSkipReason {
    OccupiedInPlan,
    BlockingTile,
    DuplicateOwnItem,
  };
  struct PlacementSkip {
    Domain::Position position;
    PlacementSkipReason reason = PlacementSkipReason::OccupiedInPlan;
  };
  struct PlacementPlan {
    DoodadLayout layout;
    std::vector<DoodadRedoBorderTouch> redoTouches;
    std::vector<PlacementSkip> skipped;
    std::vector<Domain::Position> affectedPositions;
  };
  struct ErasePlan {
    std::vector<Domain::Position> positions;
    std::vector<DoodadRedoBorderTouch> redoTouches;
    std::vector<Domain::Position> affectedPositions;
  };

  struct Variant {
    size_t alternativeIndex = 0;
    bool isSingle = true;
    size_t itemIndex = 0;
  };

  DoodadBrush(std::string name, uint32_t lookId, BrushRegistry &registry,
              bool draggable);

  BrushType getType() const override { return BrushType::Doodad; }
  bool needsBorderUpdate() const override { return redoBorders_; }
  size_t getMaxVariation() const override {
    return variants_.empty() ? alternatives_.size() : variants_.size();
  }
  void setVariation(size_t index) override { activeVariation_ = index; }

  [[nodiscard]] std::optional<Variant> getSpecificVariant() const {
    if (activeVariation_ >= variants_.size()) return std::nullopt;
    return variants_[activeVariation_];
  }

  [[nodiscard]] const DoodadAlternative *getAlternative(size_t index) const {
    if (index >= alternatives_.size()) return nullptr;
    return &alternatives_[index];
  }

  void draw(Domain::ChunkedMap &map, Domain::Tile *tile,
            const DrawContext &ctx) override;
  void undraw(Domain::ChunkedMap &map, Domain::Tile *tile) override;
  void undraw(Domain::ChunkedMap &map, Domain::Tile *tile,
              EraseOptions options);
  bool ownsItem(const Domain::Item *item) const override;

  void addAlternative(DoodadAlternative alternative);
  void setOnBlocking(bool value) { onBlocking_ = value; }
  void setOnDuplicate(bool value) { onDuplicate_ = value; }
  void setRedoBorders(bool value) { redoBorders_ = value; }
  void setOneSize(bool value) { oneSize_ = value; }
  void setRemoveOptionalBorder(bool value) { removeOptionalBorder_ = value; }
  void setThickness(float value) { thickness_ = value; }
  [[nodiscard]] float getThickness() const { return thickness_; }
  [[nodiscard]] bool isOneSize() const { return oneSize_; }
  [[nodiscard]] bool removesOptionalBorder() const {
    return removeOptionalBorder_;
  }
  [[nodiscard]] size_t getVariation() const { return activeVariation_; }

  uint16_t getPreviewItemId() const;
  DoodadLayout buildPreviewTiles(const Domain::Position &anchor,
                                 const Services::BrushSettingsService *brushSettings,
                                 const Domain::ChunkedMap *map = nullptr,
                                 std::optional<uint32_t> seed = std::nullopt) const;
  DoodadLayout buildPlacementLayout(const Domain::Position &center,
                                    const Services::BrushSettingsService *brushSettings,
                                    size_t preferredVariation,
                                    const Domain::ChunkedMap *map = nullptr,
                                    bool forcePlace = false,
                                    std::optional<uint32_t> seed = std::nullopt) const;
  PlacementPlan buildPlacementPlan(const Domain::Position &center,
                                   const Services::BrushSettingsService *brushSettings,
                                   size_t preferredVariation,
                                   const Domain::ChunkedMap *map = nullptr,
                                   bool forcePlace = false,
                                   std::optional<uint32_t> seed = std::nullopt) const;
  ErasePlan buildErasePlan(const Domain::Position &center,
                           const Services::BrushSettingsService *brushSettings,
                           size_t preferredVariation,
                           const Domain::ChunkedMap *map,
                           bool forcePlace,
                           std::optional<uint32_t> seed,
                           EraseOptions options) const;
  std::vector<Domain::Position>
  getPlacementPositions(const Domain::Position &center,
                       const Services::BrushSettingsService *brushSettings,
                       size_t preferredVariation,
                       const Domain::ChunkedMap *map = nullptr,
                       bool forcePlace = false,
                       std::optional<uint32_t> seed = std::nullopt) const;
  void applyPlacementLayout(Domain::ChunkedMap &map,
                            const Domain::Position &center,
                            const DoodadLayout &layout,
                            const DrawContext &ctx) const;
  void applyPlacementPlan(Domain::ChunkedMap &map,
                          const Domain::Position &center,
                          const PlacementPlan &plan,
                          const DrawContext &ctx) const;

private:
  friend class DoodadPlacementPlanner;

  const DoodadAlternative *selectAlternative(size_t preferredIndex) const;
  bool tileHasOwnItem(const Domain::Tile *tile) const;
  bool shouldEraseDoodadItem(const Domain::Item *item,
                             EraseOptions options) const;

  BrushRegistry &registry_;
  std::vector<DoodadAlternative> alternatives_;
  std::vector<Variant> variants_;
  std::unordered_set<uint16_t> ownedItemIds_;
  size_t activeVariation_ = 0;
  float thickness_ = 1.0f;
  bool onBlocking_ = false;
  bool onDuplicate_ = false;
  bool redoBorders_ = false;
  bool oneSize_ = false;
  bool removeOptionalBorder_ = false;
};

inline int64_t encodeDoodadPosition(const Domain::Position &position) {
  return (static_cast<int64_t>(position.x) << 32) |
         (static_cast<int64_t>(position.y) << 16) |
         static_cast<uint16_t>(position.z);
}

} // namespace MapEditor::Brushes
