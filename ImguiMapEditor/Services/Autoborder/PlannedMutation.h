#pragma once

#include "Domain/Position.h"
#include <vector>

namespace MapEditor::Domain {
class ChunkedMap;
} // namespace MapEditor::Domain

namespace MapEditor::Domain::History {
class HistoryManager;
} // namespace MapEditor::Domain::History

namespace MapEditor::Services::Autoborder {

struct PlacementIntent;
class AutoborderEngine;

enum class PlannedMutationResult {
  Unsupported,
  NoChange,
  Applied,
};

[[nodiscard]] PlannedMutationResult applyPlannedIntentWithHistory(
    const AutoborderEngine &engine, Domain::ChunkedMap &map,
    Domain::History::HistoryManager &history, const PlacementIntent &intent,
    std::vector<Domain::Position> *changedPositions = nullptr);

} // namespace MapEditor::Services::Autoborder
