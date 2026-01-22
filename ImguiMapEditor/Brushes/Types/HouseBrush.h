#pragma once
#include "Brushes/Core/IBrush.h"

namespace MapEditor::Brushes {

/**
 * Brush for assigning house IDs to tiles.
 *
 * Setting house ID makes tiles part of a player house.
 */
class HouseBrush : public IBrush {
public:
  HouseBrush();
  ~HouseBrush() override = default;

  // IBrush interface
  void draw(Domain::ChunkedMap &map, Domain::Tile *tile,
            const DrawContext &ctx) override;
  void undraw(Domain::ChunkedMap &map, Domain::Tile *tile) override;

  const std::string &getName() const override { return name_; }
  BrushType getType() const override { return BrushType::House; }
  uint32_t getLookId() const override { return 0; }

  // House ID configuration
  void setHouseId(uint32_t id) { houseId_ = id; }
  uint32_t getHouseId() const { return houseId_; }

private:
  std::string name_ = "House";
  uint32_t houseId_ = 0; // 0 means no house assigned
};

} // namespace MapEditor::Brushes
