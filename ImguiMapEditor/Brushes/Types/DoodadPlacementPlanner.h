#pragma once

#include "Brushes/Types/DoodadBrush.h"
#include <optional>
#include <random>

namespace MapEditor::Brushes {

class DoodadPlacementPlanner {
public:
  struct Request {
    const DoodadBrush &brush;
    Domain::Position center;
    const Services::BrushSettingsService *brushSettings = nullptr;
    size_t preferredVariation = 0;
    const Domain::ChunkedMap *map = nullptr;
    bool forcePlace = false;
    std::optional<uint32_t> seed;
  };

  static DoodadBrush::DoodadLayout generateRawStamp(
      const DoodadBrush &brush,
      const Services::BrushSettingsService *brushSettings,
      size_t preferredVariation,
      std::optional<uint32_t> seed);

  static DoodadBrush::DoodadLayout build(const Request &request);
  static DoodadBrush::PlacementPlan buildPlan(const Request &request);
  static uint32_t
  buildSeed(const DoodadBrush &brush, const Domain::Position &center,
            const Services::BrushSettingsService *brushSettings,
            size_t preferredVariation, bool forcePlace);

private:
  struct LayoutBuildResult {
    DoodadBrush::DoodadLayout layout;
    std::vector<DoodadBrush::PlacementSkip> skipped;
  };

private:
  static std::vector<DoodadRedoBorderTouch>
  buildRedoTouches(const Request &request,
                   const DoodadBrush::DoodadLayout &layout);
  static std::vector<Domain::Position>
  buildAffectedPositions(const Request &request,
                         const DoodadBrush::DoodadLayout &layout,
                         const std::vector<DoodadRedoBorderTouch> &redoTouches);
};

} // namespace MapEditor::Brushes
