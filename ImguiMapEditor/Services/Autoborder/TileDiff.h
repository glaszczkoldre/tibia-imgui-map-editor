#pragma once

#include "Domain/Position.h"
#include "Domain/Tile.h"
#include <memory>
#include <vector>

namespace MapEditor::Domain {
class ChunkedMap;
class Tile;
} // namespace MapEditor::Domain

namespace MapEditor::Domain::History {
class HistoryManager;
} // namespace MapEditor::Domain::History

namespace MapEditor::Services::Autoborder {

struct TileDiff {
  Domain::Position position;
  std::unique_ptr<Domain::Tile> after;
};

using TileDiffList = std::vector<TileDiff>;

void applyTileDiffs(Domain::ChunkedMap &map, const TileDiffList &diffs);
void applyTileDiffsWithHistory(Domain::ChunkedMap &map,
                               Domain::History::HistoryManager &history,
                               const TileDiffList &diffs);

} // namespace MapEditor::Services::Autoborder
