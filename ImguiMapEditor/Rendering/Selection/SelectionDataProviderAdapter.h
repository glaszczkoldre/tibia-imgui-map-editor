#pragma once
#include "ISelectionDataProvider.h"
#include "Services/Selection/SelectionService.h"

namespace MapEditor {
namespace Rendering {

/**
 * Adapter that wraps SelectionService to provide data to renderers.
 * Implements ISelectionDataProvider interface for clean separation.
 *
 * Follows Adapter Pattern (GoF): converts SelectionService interface
 * to the ISelectionDataProvider interface expected by rendering layer.
 *
 * Dependency Inversion: Rendering depends on ISelectionDataProvider abstraction,
 * not on SelectionService concrete implementation.
 */
class SelectionDataProviderAdapter : public ISelectionDataProvider {
public:
  explicit SelectionDataProviderAdapter(
      const Services::Selection::SelectionService *service = nullptr)
      : service_(service) {}

  /**
   * Update the underlying service reference.
   * Called when session changes or selection service is recreated.
   */
  void setService(const Services::Selection::SelectionService *service) {
    service_ = service;
  }

  // === Basic Queries ===

  bool isEmpty() const override { return !service_ || service_->isEmpty(); }

  size_t getSelectionCount() const override {
    return service_ ? service_->size() : 0;
  }

  bool hasSelectionAt(const Domain::Position &pos) const override {
    return service_ && service_->hasSelectionAt(pos);
  }

  bool isItemSelected(const Domain::Position &pos,
                      const Domain::Item *item) const override {
    if (!service_)
      return false;

    auto entries = service_->getEntriesAt(pos);
    for (const auto &entry : entries) {
      // Ground type with null ptr means whole tile selected
      if (entry.getType() == Domain::Selection::EntityType::Ground &&
          entry.entity_ptr == nullptr) {
        return true;
      }
      // Check specific item
      if (entry.entity_ptr == item) {
        return true;
      }
    }
    return false;
  }

  bool getSelectionBounds(int32_t &min_x, int32_t &min_y, int16_t &min_z,
                          int32_t &max_x, int32_t &max_y,
                          int16_t &max_z) const override {
    if (!service_ || service_->isEmpty())
      return false;

    // Use getMinBound() and getMaxBound() from SelectionService
    Domain::Position min_pos = service_->getMinBound();
    Domain::Position max_pos = service_->getMaxBound();

    min_x = min_pos.x;
    min_y = min_pos.y;
    min_z = min_pos.z;
    max_x = max_pos.x;
    max_y = max_pos.y;
    max_z = max_pos.z;
    return true;
  }

  // === Floor-Filtered Iteration ===

  std::vector<Domain::Position>
  getPositionsOnFloor(int16_t floor) const override {
    if (!service_)
      return {};

    std::vector<Domain::Position> result;
    for (const auto &entry : service_->getAllEntries()) {
      // Use getPosition() method, not .pos member
      const Domain::Position &pos = entry.getPosition();
      if (pos.z == floor) {
        result.push_back(pos);
      }
    }
    return result;
  }

  void forEachEntryOnFloor(int16_t floor,
                           const EntryCallback &callback) const override {
    if (!service_)
      return;

    for (const auto &entry : service_->getAllEntries()) {
      // Use getPosition() method, not .pos member
      const Domain::Position &pos = entry.getPosition();
      if (pos.z == floor) {
        callback(pos, entry.getType());
      }
    }
  }

  bool hasSpawnSelectionAt(const Domain::Position &pos) const override {
    if (!service_)
      return false;

    auto entries = service_->getEntriesAt(pos);
    for (const auto &entry : entries) {
      if (entry.getType() == Domain::Selection::EntityType::Spawn) {
        return true;
      }
    }
    return false;
  }

  bool hasCreatureSelectionAt(const Domain::Position &pos) const override {
    if (!service_)
      return false;

    auto entries = service_->getEntriesAt(pos);
    for (const auto &entry : entries) {
      if (entry.getType() == Domain::Selection::EntityType::Creature) {
        return true;
      }
    }
    return false;
  }

private:
  const Services::Selection::SelectionService *service_ = nullptr;
};

} // namespace Rendering
} // namespace MapEditor
