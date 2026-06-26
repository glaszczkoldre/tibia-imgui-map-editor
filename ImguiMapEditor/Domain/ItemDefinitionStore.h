#pragma once

#include "Domain/ItemType.h"
#include <vector>
#include <unordered_map>

namespace MapEditor::Domain {

class ItemDefinitionStore {
public:
    ItemDefinitionStore() = default;
    ~ItemDefinitionStore() = default;

    // Non-copyable
    ItemDefinitionStore(const ItemDefinitionStore&) = delete;
    ItemDefinitionStore& operator=(const ItemDefinitionStore&) = delete;

    // Moveable
    ItemDefinitionStore(ItemDefinitionStore&&) noexcept = default;
    ItemDefinitionStore& operator=(ItemDefinitionStore&&) noexcept = default;

    void setItems(std::vector<ItemType> items);

    const std::vector<ItemType>& getItemTypes() const { return items_; }
    std::vector<ItemType>& getItemTypes() { return items_; }

    const ItemType* getItemTypeByServerId(uint16_t server_id) const;
    ItemType* getItemTypeByServerId(uint16_t server_id);
    const ItemType* getItemTypeByClientId(uint16_t client_id) const;
    ItemType* getItemTypeByClientId(uint16_t client_id);

    void clear();

    uint16_t getMaxServerId() const { return max_server_id_; }
    uint16_t getMaxClientId() const { return max_client_id_; }

    void rebuildIndexes();

private:
    std::vector<ItemType> items_;
    std::unordered_map<uint16_t, size_t> server_id_index_;
    std::unordered_map<uint16_t, size_t> client_id_index_;
    uint16_t max_server_id_ = 0;
    uint16_t max_client_id_ = 0;
};

} // namespace MapEditor::Domain
