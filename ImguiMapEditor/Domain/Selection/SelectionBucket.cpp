#include "SelectionBucket.h"
#include <algorithm>
#include <limits>

namespace MapEditor::Domain::Selection {

void SelectionBucket::add(const SelectionEntry &entry) {
  uint64_t entity_key = entityKey(entry.id);

  // Check if already exists
  if (entries_.find(entity_key) != entries_.end()) {
    return; // Already selected, no-op
  }

  // Add to primary storage
  entries_.emplace(entity_key, entry);

  // Add to position index
  uint64_t pos_key = positionKey(entry.id.position);
  position_index_[pos_key].insert(entity_key);
}

void SelectionBucket::remove(const EntityId &id) {
  uint64_t entity_key = entityKey(id);

  auto it = entries_.find(entity_key);
  if (it == entries_.end()) {
    return; // Not selected, no-op
  }

  // Remove from position index
  uint64_t pos_key = positionKey(id.position);
  auto pos_it = position_index_.find(pos_key);
  if (pos_it != position_index_.end()) {
    pos_it->second.erase(entity_key);
    // Clean up empty position entries
    if (pos_it->second.empty()) {
      position_index_.erase(pos_it);
    }
  }

  // Remove from primary storage
  entries_.erase(it);
}

void SelectionBucket::removeAllAt(const Position &pos) {
  uint64_t pos_key = positionKey(pos);

  auto pos_it = position_index_.find(pos_key);
  if (pos_it == position_index_.end()) {
    return; // No entries at this position
  }

  // Copy the set of entity keys (we'll modify the map while iterating)
  std::vector<uint64_t> entity_keys(pos_it->second.begin(),
                                    pos_it->second.end());

  // Remove each entry from primary storage
  for (uint64_t entity_key : entity_keys) {
    entries_.erase(entity_key);
  }

  // Remove position from index
  position_index_.erase(pos_it);
}

void SelectionBucket::clear() {
  entries_.clear();
  position_index_.clear();
}

bool SelectionBucket::contains(const EntityId &id) const {
  return entries_.find(entityKey(id)) != entries_.end();
}

bool SelectionBucket::hasEntriesAt(const Position &pos) const {
  auto it = position_index_.find(positionKey(pos));
  return it != position_index_.end() && !it->second.empty();
}

std::vector<SelectionEntry>
SelectionBucket::getEntriesAt(const Position &pos) const {
  std::vector<SelectionEntry> result;

  auto pos_it = position_index_.find(positionKey(pos));
  if (pos_it == position_index_.end()) {
    return result;
  }

  result.reserve(pos_it->second.size());
  for (uint64_t entity_key : pos_it->second) {
    auto entry_it = entries_.find(entity_key);
    if (entry_it != entries_.end()) {
      result.push_back(entry_it->second);
    }
  }

  return result;
}

std::vector<SelectionEntry> SelectionBucket::getAllEntries() const {
  std::vector<SelectionEntry> result;
  result.reserve(entries_.size());

  for (const auto &[key, entry] : entries_) {
    result.push_back(entry);
  }

  return result;
}

std::vector<Position> SelectionBucket::getPositions() const {
  std::vector<Position> result;
  result.reserve(position_index_.size());

  for (const auto &[pos_key, entity_keys] : position_index_) {
    if (!entity_keys.empty()) {
      // Get position from any entry at this position
      auto entity_it = entries_.find(*entity_keys.begin());
      if (entity_it != entries_.end()) {
        result.push_back(entity_it->second.id.position);
      }
    }
  }

  return result;
}

Position SelectionBucket::getMinBound() const {
  if (entries_.empty()) {
    return Position{0, 0, 0};
  }

  int32_t min_x = std::numeric_limits<int32_t>::max();
  int32_t min_y = std::numeric_limits<int32_t>::max();
  int16_t min_z = std::numeric_limits<int16_t>::max();

  for (const auto &[key, entry] : entries_) {
    const Position &pos = entry.id.position;
    min_x = std::min(min_x, pos.x);
    min_y = std::min(min_y, pos.y);
    min_z = std::min(min_z, pos.z);
  }

  return Position{min_x, min_y, min_z};
}

Position SelectionBucket::getMaxBound() const {
  if (entries_.empty()) {
    return Position{0, 0, 0};
  }

  int32_t max_x = std::numeric_limits<int32_t>::min();
  int32_t max_y = std::numeric_limits<int32_t>::min();
  int16_t max_z = std::numeric_limits<int16_t>::min();

  for (const auto &[key, entry] : entries_) {
    const Position &pos = entry.id.position;
    max_x = std::max(max_x, pos.x);
    max_y = std::max(max_y, pos.y);
    max_z = std::max(max_z, pos.z);
  }

  return Position{max_x, max_y, max_z};
}

std::vector<SelectionEntry>
SelectionBucket::getEntriesOnFloor(int16_t floor) const {
  std::vector<SelectionEntry> result;

  for (const auto &[key, entry] : entries_) {
    if (entry.id.position.z == floor) {
      result.push_back(entry);
    }
  }

  return result;
}

std::vector<Position>
SelectionBucket::getPositionsOnFloor(int16_t floor) const {
  std::set<uint64_t> seen_positions; // Use set to deduplicate
  std::vector<Position> result;

  for (const auto &[key, entry] : entries_) {
    if (entry.id.position.z == floor) {
      uint64_t pos_key = positionKey(entry.id.position);
      if (seen_positions.insert(pos_key).second) {
        result.push_back(entry.id.position);
      }
    }
  }

  return result;
}

} // namespace MapEditor::Domain::Selection
