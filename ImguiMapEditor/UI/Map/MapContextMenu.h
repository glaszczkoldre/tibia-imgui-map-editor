#pragma once
#include "Domain/Position.h"
#include <functional>

namespace MapEditor {
namespace AppLogic {
    class EditorSession;
    class ClipboardService;
}
namespace Domain {
    class Tile;
    class Item;
}
}

namespace MapEditor::UI {

/**
 * Right-click context menu for map tiles/items.
 * Rendered as ImGui popup.
 */
class MapContextMenu {
public:
    using PropertiesCallback = std::function<void(Domain::Item*)>;
    using GotoCallback = std::function<void(const Domain::Position&)>;
    // Extended to pass top item server ID (0 if no item on tile)
    using BrowseTileCallback = std::function<void(const Domain::Position&, uint16_t item_server_id)>;
    
    MapContextMenu();
    
    /**
     * Show the context menu at the given position.
     */
    void show(const Domain::Position& pos);
    
    /**
     * Render the context menu. Call each frame.
     */
    void render(
        AppLogic::EditorSession* session,
        AppLogic::ClipboardService* clipboard,
        PropertiesCallback on_properties = nullptr,
        GotoCallback on_goto = nullptr
    );
    
    void setBrowseTileCallback(BrowseTileCallback cb) { browse_tile_callback_ = std::move(cb); }
    
    bool isOpen() const { return is_open_; }
    
private:
    void renderTileActions(AppLogic::EditorSession* session);
    void renderItemActions(AppLogic::EditorSession* session);
    void renderClipboardActions(
        AppLogic::EditorSession* session,
        AppLogic::ClipboardService* clipboard
    );
    void renderNavigationActions(AppLogic::EditorSession* session);
    
    bool is_open_ = false;
    Domain::Position position_;
    const Domain::Tile* current_tile_ = nullptr;
    Domain::Item* selected_item_ = nullptr;
    PropertiesCallback properties_callback_;
    GotoCallback goto_callback_;
    BrowseTileCallback browse_tile_callback_;
};

} // namespace MapEditor::UI
