#include "TileDiff.h"

#include "Domain/ChunkedMap.h"
#include "Domain/History/HistoryManager.h"
#include "Domain/Tile.h"

namespace MapEditor::Services::Autoborder {

void applyTileDiffs(Domain::ChunkedMap &map, TileDiffList diffs) {
  for (auto &diff : diffs) {
    map.setTile(diff.position, std::move(diff.after));
  }
  if (!diffs.empty()) {
    map.markChanged();
  }
}

void applyTileDiffsWithHistory(Domain::ChunkedMap &map,
                               Domain::History::HistoryManager &history,
                               TileDiffList diffs) {
  for (const auto &diff : diffs) {
    history.recordTileBefore(diff.position, map.getTile(diff.position));
  }
  applyTileDiffs(map, std::move(diffs));
}

} // namespace MapEditor::Services::Autoborder
