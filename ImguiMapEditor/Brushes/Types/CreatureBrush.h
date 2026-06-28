#pragma once
#include "Brushes/Core/IBrush.h"
#include "Domain/Outfit.h"
#include <string>

namespace MapEditor::Brushes {

  class CreatureBrush : public IBrush {
  public:
    explicit CreatureBrush(const std::string& name, const Domain::Outfit& outfit);
    ~CreatureBrush() override = default;

    // IBrush interface
    const std::string& getName() const override { return name_; }
    BrushType getType() const override { return BrushType::Creature; }
    uint32_t getLookId() const override { return static_cast<uint32_t>(outfit_.lookType); } // Used for preview
    BrushPreviewDescriptor getPreviewDescriptor() const override {
      return BrushPreviewDescriptor::creature(outfit_);
    }

    void draw(Domain::ChunkedMap& map, Domain::Tile* tile, const DrawContext& ctx) override;
    void undraw(Domain::ChunkedMap& map, Domain::Tile* tile) override;

    size_t getMaxVariation() const override { return 4; }
    void setVariation(size_t index) override { direction_ = static_cast<uint8_t>(index % 4); }
    size_t getVariation() const { return direction_; }

  private:
    std::string name_;
    Domain::Outfit outfit_;
    uint8_t direction_ = 2; // 0=North, 1=East, 2=South, 3=West (default South)
  };

} // namespace MapEditor::Brushes
