#pragma once
#include "Domain/Tile.h"
#include <imgui.h>

namespace MapEditor {

namespace Services {
class SpriteManager;
class ClientDataService;
} // namespace Services

namespace UI {
namespace BrowseTile {

class SpawnCreatureRenderer {
public:
  SpawnCreatureRenderer(Services::SpriteManager *spriteManager,
                        Services::ClientDataService *clientData);

  void setContext(Services::SpriteManager *spriteManager,
                  Services::ClientDataService *clientData);

  void render(const Domain::Tile *current_tile, bool &spawn_selected,
              bool &creature_selected, int &selected_index);

private:
  Services::SpriteManager *sprite_manager_ = nullptr;
  Services::ClientDataService *client_data_ = nullptr;
};

} // namespace BrowseTile
} // namespace UI
} // namespace MapEditor
