#pragma once

#include "Domain/Position.h"
#include <algorithm>
#include <unordered_set>
#include <vector>

namespace MapEditor::Utils {

inline std::vector<Domain::Position>
dedupeAndSortPositions(std::vector<Domain::Position> positions) {
  std::unordered_set<Domain::Position> seen;
  seen.reserve(positions.size());
  std::vector<Domain::Position> result;
  result.reserve(positions.size());

  for (const auto &pos : positions) {
    if (seen.insert(pos).second) {
      result.push_back(pos);
    }
  }

  std::sort(result.begin(), result.end());
  return result;
}

} // namespace MapEditor::Utils
