#pragma once

#include "Domain/Position.h"
#include <span>
#include <vector>

namespace MapEditor::Brushes {

struct DoodadRedoBorderTouch {
  Domain::Position position;
  bool placedGround = false;
  bool placedWall = false;
};

[[nodiscard]] std::vector<Domain::Position>
buildDoodadRedoBorderPositions(std::span<const DoodadRedoBorderTouch> touches);

} // namespace MapEditor::Brushes
