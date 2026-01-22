#pragma once
#include "Brushes/Core/IBrush.h"
#include "Domain/Tile.h"

namespace MapEditor::Brushes {

/**
 * Brush for setting zone flags (PZ, NoPvp, NoLogout, etc) on tiles.
 *
 * Each FlagBrush instance handles one specific flag type.
 * Create multiple instances for different flag types.
 */
class FlagBrush : public IBrush {
public:
  /**
   * Create a flag brush for the specified flag type.
   * @param flag The TileFlag to set/clear
   * @param name Display name for the brush
   */
  explicit FlagBrush(Domain::TileFlag flag, const std::string &name);
  ~FlagBrush() override = default;

  // IBrush interface
  void draw(Domain::ChunkedMap &map, Domain::Tile *tile,
            const DrawContext &ctx) override;
  void undraw(Domain::ChunkedMap &map, Domain::Tile *tile) override;

  const std::string &getName() const override { return name_; }
  BrushType getType() const override { return BrushType::Flag; }
  uint32_t getLookId() const override { return 0; }

  // Get the flag this brush sets
  Domain::TileFlag getFlag() const { return flag_; }

private:
  Domain::TileFlag flag_;
  std::string name_;
};

} // namespace MapEditor::Brushes
