#pragma once
#include "SelectionMode.h"
namespace MapEditor::Services {
class ConfigService;
}

namespace MapEditor::Domain {

/**
 * Stores current selection preferences.
 * Can be persisted via ConfigService.
 */
struct SelectionSettings {

  /// Floor scope for selection operations
  SelectionFloorScope floor_scope = SelectionFloorScope::CurrentFloor;

  /// Whether to use pixel-perfect selection (sprite hit testing)
  /// When false, uses Smart selection (logical priority)
  bool use_pixel_perfect = false;

  // Persistence
  void loadFromConfig(const Services::ConfigService &config);
  void saveToConfig(Services::ConfigService &config) const;
};

} // namespace MapEditor::Domain
