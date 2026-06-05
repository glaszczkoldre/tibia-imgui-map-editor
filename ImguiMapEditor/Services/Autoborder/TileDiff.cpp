#include "TileDiff.h"

#include "Domain/ChunkedMap.h"
#include "Domain/History/HistoryManager.h"
#include "Domain/Tile.h"

namespace MapEditor::Services::Autoborder {

void applyTileDiffs(Domain::ChunkedMap &map, const TileDiffList &diffs) {
  for (const auto &diff : diffs) {
    map.setTile(diff.position, diff.after ? diff.after->clone() : nullptr);
  }
  if (!diffs.empty()) {
    map.markChanged();
  }
}

void applyTileDiffsWithHistory(Domain::ChunkedMap &map,
                               Domain::History::HistoryManager &history,
                               const TileDiffList &diffs) {
  for (const auto &diff : diffs) {
    history.recordTileBefore(diff.position, map.getTile(diff.position));
  }
  applyTileDiffs(map, diffs);
}

} // namespace MapEditor::Services::Autoborder
