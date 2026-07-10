#pragma once

#include "Services/Autoborder/AutoborderResolver.h"
#include "Services/Autoborder/TileDiff.h"
#include <memory>
#include <vector>

namespace MapEditor::Domain {
class ChunkedMap;
} // namespace MapEditor::Domain

namespace MapEditor::Services::Autoborder {

class AutoborderEngine {
public:
  AutoborderEngine();
  ~AutoborderEngine();

  AutoborderEngine(const AutoborderEngine &) = delete;
  AutoborderEngine &operator=(const AutoborderEngine &) = delete;

  [[nodiscard]] bool canPlan(const PlacementIntent &intent) const;

  [[nodiscard]] TileDiffList plan(const Domain::ChunkedMap &map,
                                  const PlacementIntent &intent) const;

private:
  [[nodiscard]] const AutoborderResolver *
  findResolver(const PlacementIntent &intent) const;

  std::vector<std::unique_ptr<AutoborderResolver>> resolvers_;
};

} // namespace MapEditor::Services::Autoborder
