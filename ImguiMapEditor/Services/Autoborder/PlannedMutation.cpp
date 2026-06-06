#include "PlannedMutation.h"

#include "Domain/Tile.h"
#include "Services/Autoborder/AutoborderEngine.h"
#include "Services/Autoborder/TileDiff.h"

namespace MapEditor::Services::Autoborder {

PlannedMutationResult applyPlannedIntentWithHistory(
    const AutoborderEngine &engine, Domain::ChunkedMap &map,
    Domain::History::HistoryManager &history, const PlacementIntent &intent) {
  if (!engine.canPlan(intent)) {
    return PlannedMutationResult::Unsupported;
  }

  auto diffs = engine.plan(map, intent);
  if (diffs.empty()) {
    return PlannedMutationResult::NoChange;
  }

  applyTileDiffsWithHistory(map, history, diffs);
  return PlannedMutationResult::Applied;
}

} // namespace MapEditor::Services::Autoborder
