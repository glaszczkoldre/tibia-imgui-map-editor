#pragma once
#include "Domain/Position.h"
#include <imgui.h>
#include <string>
#include <vector>

namespace MapEditor {

namespace Domain {
class Tile;
class ChunkedMap;
class Item;
class ItemType;
} // namespace Domain

namespace Services {
class SpriteManager;
}

namespace AppLogic {
class EditorSession;
}

namespace UI {
namespace BrowseTile {

/**
 * Actions that can be requested from the toolbar.
 * ItemsListRenderer only handles item-related actions internally.
 * Spawn/Creature deletion is delegated to the parent window.
 */
enum class ToolbarAction { None, DeleteSpawn, DeleteCreature };

class ItemsListRenderer {
public:
  ItemsListRenderer(Domain::ChunkedMap *map,
                    Services::SpriteManager *spriteManager,
                    AppLogic::EditorSession *session);

  void setContext(Domain::ChunkedMap *map,
                  Services::SpriteManager *spriteManager,
                  AppLogic::EditorSession *session);

  void render(const Domain::Tile *current_tile,
              const Domain::Position &current_pos, int &selected_index,
              bool &spawn_selected, bool &creature_selected);

  /**
   * Renders the toolbar and returns any action that needs parent handling.
   * Item deletion is handled internally; spawn/creature deletion is delegated.
   */
  ToolbarAction renderToolbar(const Domain::Tile *current_tile,
                              const Domain::Position &current_pos,
                              int &selected_index, bool spawn_selected,
                              bool creature_selected);

  // Call this from the parent window context (after EndChild of list) to check
  // if item was dragged out of the window
  void checkDragOutDeletion(const Domain::Tile *current_tile,
                            const Domain::Position &current_pos,
                            int &selected_index);

private:
  Domain::ChunkedMap *map_ = nullptr;
  Services::SpriteManager *sprite_manager_ = nullptr;
  AppLogic::EditorSession *session_ = nullptr;

  // Helper to deduplicate rendering logic for ground and stacked items
  void renderItemRow(const Domain::Item *item, const Domain::ItemType *type,
                     int display_index, bool is_ground,
                     const Domain::Position &current_pos, int &selected_index,
                     bool &spawn_selected, bool &creature_selected);

  void handleItemDragDrop(int source_index, int display_index,
                          const Domain::Position &current_pos,
                          int &selected_index);
  void handleDelete(int source_index, const Domain::Tile *current_tile,
                    const Domain::Position &current_pos, int &selected_index);
  void swapItems(const Domain::Position &pos, int src_idx, int dst_idx);
  void moveItem(const Domain::Position &pos, int src_idx, int dst_idx,
                int &selected_index);
};

} // namespace BrowseTile
} // namespace UI
} // namespace MapEditor
