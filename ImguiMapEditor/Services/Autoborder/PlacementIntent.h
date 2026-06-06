#pragma once

#include "Brushes/Core/IBrush.h"
#include "Domain/Position.h"
#include <vector>

namespace MapEditor::Services::Autoborder {

enum class PlacementMode {
  Draw,
  Erase,
  ResolveOnly,
};

struct PlacementIntent {
  const MapEditor::Brushes::IBrush *brush = nullptr;
  MapEditor::Brushes::DrawContext context;
  PlacementMode mode = PlacementMode::Draw;
  std::vector<Domain::Position> positions;

  [[nodiscard]] bool isValid() const {
    return brush != nullptr && !positions.empty();
  }
};

} // namespace MapEditor::Services::Autoborder
