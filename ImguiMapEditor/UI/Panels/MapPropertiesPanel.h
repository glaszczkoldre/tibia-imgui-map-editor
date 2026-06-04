#pragma once
#include "Services/ClientVersionRegistry.h"
#include <cstdint>
#include <string>

namespace MapEditor {
namespace Domain {
class ChunkedMap;
}

namespace UI {

class MapPropertiesPanel {
public:
  static constexpr float LEFT_RATIO = 0.60f;
  static constexpr float RIGHT_RATIO = 0.40f;

  struct State {
    std::string map_name;
    uint32_t otbm_version = 0;
    uint32_t items_major = 0;
    uint32_t items_minor = 0;
    std::string client_version_name;
    uint16_t map_width = 0;
    uint16_t map_height = 0;
    std::string description;
    std::string house_file;
    std::string spawn_file;
  };

  void initialize(Services::ClientVersionRegistry *registry);
  State loadFromMap(Domain::ChunkedMap *map);
  bool render(State &state);

private:
  Services::ClientVersionRegistry *registry_ = nullptr;
};

} // namespace UI
} // namespace MapEditor
