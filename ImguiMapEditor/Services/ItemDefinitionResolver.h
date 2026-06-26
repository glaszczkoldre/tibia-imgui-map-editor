#pragma once

#include "Domain/ClientVersionTypes.h"
#include "Domain/ItemType.h"
#include "IO/Readers/DatReaderBase.h"

#include <vector>

namespace MapEditor::Services {

/**
 * Resolves server metadata and DAT client visuals into one final ItemType table.
 *
 * This is the single place where source precedence is allowed. Callers receive
 * final ItemType rows and must not reason about whether a property came from
 * OTB, SRV, DAT, XML, or a future source adapter.
 */
class ItemDefinitionResolver {
public:
  static std::vector<Domain::ItemType>
  resolve(Domain::ItemDataSource source,
          const std::vector<Domain::ItemType> &server_items,
          const IO::DatResult &dat_result);

  static void normalizeResolvedItemTypes(std::vector<Domain::ItemType> &items);

private:
  static Domain::ItemType createDatOnlyItem(const IO::ClientItem &dat);
  static void applyDatFragment(Domain::ItemType &item, const IO::ClientItem &dat,
                               bool datOwnsClassification);
  static void normalizeResolvedItemType(Domain::ItemType &item);
};

} // namespace MapEditor::Services
