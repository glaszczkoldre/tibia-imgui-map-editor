#include "DoodadRedoBorderPlanner.h"

#include <unordered_set>

namespace MapEditor::Brushes {

namespace {

void appendUnique(std::vector<Domain::Position> &positions,
                  std::unordered_set<Domain::Position> &seen,
                  const Domain::Position &position) {
  if (seen.insert(position).second) {
    positions.push_back(position);
  }
}

void appendGroundRedoPositions(std::vector<Domain::Position> &positions,
                               std::unordered_set<Domain::Position> &seen,
                               const Domain::Position &center) {
  for (int32_t dy = -1; dy <= 1; ++dy) {
    for (int32_t dx = -1; dx <= 1; ++dx) {
      appendUnique(positions, seen,
                   {center.x + dx, center.y + dy, center.z});
    }
  }
}

void appendWallRedoPositions(std::vector<Domain::Position> &positions,
                             std::unordered_set<Domain::Position> &seen,
                             const Domain::Position &center) {
  appendUnique(positions, seen, {center.x, center.y - 1, center.z});
  appendUnique(positions, seen, {center.x - 1, center.y, center.z});
  appendUnique(positions, seen, {center.x + 1, center.y, center.z});
  appendUnique(positions, seen, {center.x, center.y + 1, center.z});
}

} // namespace

std::vector<Domain::Position>
buildDoodadRedoBorderPositions(std::span<const DoodadRedoBorderTouch> touches) {
  std::vector<Domain::Position> positions;
  positions.reserve(touches.size() * 5);
  std::unordered_set<Domain::Position> seen;
  seen.reserve(touches.size() * 5);

  for (const auto &touch : touches) {
    appendUnique(positions, seen, touch.position);

    if (touch.placedGround) {
      appendGroundRedoPositions(positions, seen, touch.position);
    } else if (touch.placedWall) {
      appendWallRedoPositions(positions, seen, touch.position);
    }
  }

  return positions;
}

} // namespace MapEditor::Brushes
