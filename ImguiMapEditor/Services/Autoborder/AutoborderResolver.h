#pragma once

#include "Domain/Position.h"
#include "Services/Autoborder/PlacementIntent.h"
#include <vector>

namespace MapEditor::Domain {
class ChunkedMap;
} // namespace MapEditor::Domain

namespace MapEditor::Services::Autoborder {

class AutoborderResolver {
public:
  virtual ~AutoborderResolver() = default;

  [[nodiscard]] virtual bool canResolve(const PlacementIntent &intent) const = 0;

  [[nodiscard]] virtual std::vector<Domain::Position>
  expandAffectedPositions(const PlacementIntent &intent) const = 0;

  virtual void applyIntent(Domain::ChunkedMap &scratchMap,
                           const Domain::ChunkedMap &sourceMap,
                           const PlacementIntent &intent) const = 0;

  virtual void resolve(Domain::ChunkedMap &scratchMap,
                       const PlacementIntent &intent,
                       const std::vector<Domain::Position> &affected) const = 0;
};

} // namespace MapEditor::Services::Autoborder
