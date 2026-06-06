#pragma once

#include "Services/Autoborder/PlacementIntent.h"

namespace MapEditor::Domain {
class ChunkedMap;
} // namespace MapEditor::Domain

namespace MapEditor::Domain::History {
class HistoryManager;
} // namespace MapEditor::Domain::History

namespace MapEditor::Services::Autoborder {

class AutoborderEngine;

enum class PlannedMutationResult {
  Unsupported,
  NoChange,
  Applied,
};

[[nodiscard]] PlannedMutationResult applyPlannedIntentWithHistory(
    const AutoborderEngine &engine, Domain::ChunkedMap &map,
    Domain::History::HistoryManager &history, const PlacementIntent &intent);

} // namespace MapEditor::Services::Autoborder
