#include "ItemDefinitionStore.h"
#include <algorithm>

namespace MapEditor::Domain {

void ItemDefinitionStore::setItems(std::vector<ItemType> items) {
    items_ = std::move(items);
    rebuildIndexes();
}

const ItemType* ItemDefinitionStore::getItemTypeByServerId(uint16_t server_id) const {
    auto it = server_id_index_.find(server_id);
    if (it != server_id_index_.end()) {
        return &items_[it->second];
    }
    return nullptr;
}

const ItemType* ItemDefinitionStore::getItemTypeByClientId(uint16_t client_id) const {
    auto it = client_id_index_.find(client_id);
    if (it != client_id_index_.end()) {
        return &items_[it->second];
    }
    return nullptr;
}

void ItemDefinitionStore::clear() {
    items_.clear();
    server_id_index_.clear();
    client_id_index_.clear();
    max_server_id_ = 0;
    max_client_id_ = 0;
}

void ItemDefinitionStore::rebuildIndexes() {
    server_id_index_.clear();
    client_id_index_.clear();
    max_server_id_ = 0;
    max_client_id_ = 0;
    for (size_t index = 0; index < items_.size(); ++index) {
        const auto &item = items_[index];
        if (item.server_id > 0) {
            server_id_index_[item.server_id] = index;
            max_server_id_ = std::max(max_server_id_, item.server_id);
        }
        if (item.client_id > 0) {
            client_id_index_[item.client_id] = index;
            max_client_id_ = std::max(max_client_id_, item.client_id);
        }
    }
}

} // namespace MapEditor::Domain
