#include "PlannedMutation.h"

#include "Services/Autoborder/AutoborderEngine.h"
#include "Services/Autoborder/PlacementIntent.h"
#include "Services/Autoborder/TileDiff.h"

namespace MapEditor::Services::Autoborder {

PlannedMutationResult applyPlannedIntentWithHistory(
    const AutoborderEngine &engine, Domain::ChunkedMap &map,
    Domain::History::HistoryManager &history, const PlacementIntent &intent,
    std::vector<Domain::Position> *changedPositions) {
  if (changedPositions) {
    changedPositions->clear();
  }

  if (!engine.canPlan(intent)) {
    return PlannedMutationResult::Unsupported;
  }

  auto diffs = engine.plan(map, intent);
  if (diffs.empty()) {
    return PlannedMutationResult::NoChange;
  }

  if (changedPositions) {
    changedPositions->reserve(diffs.size());
    for (const auto &diff : diffs) {
      changedPositions->push_back(diff.position);
    }
  }

  applyTileDiffsWithHistory(map, history, diffs);
  return PlannedMutationResult::Applied;
}

} // namespace MapEditor::Services::Autoborder
