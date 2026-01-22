#pragma once
#include "Domain/Search/ISearchProvider.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace MapEditor {
namespace AppLogic { class ItemPickerService; }
namespace Services {
    class SpriteManager;
    class ClientDataService;
}

namespace UI {

/**
 * QuickSearch popup (Ctrl+F) - VS Code style item picker.
 * Modal popup for quickly selecting items/creatures to place.
 */
class QuickSearchPopup {
public:
    using SelectCallback = std::function<void(uint16_t server_id, bool is_creature)>;
    
    QuickSearchPopup();
    ~QuickSearchPopup() = default;
    
    // Non-copyable
    QuickSearchPopup(const QuickSearchPopup&) = delete;
    QuickSearchPopup& operator=(const QuickSearchPopup&) = delete;
    
    /**
     * Set dependencies (call before first render)
     */
    void setItemPickerService(AppLogic::ItemPickerService* picker) { picker_ = picker; }
    void setSpriteManager(Services::SpriteManager* sprites) { sprite_manager_ = sprites; }
    void setClientDataService(Services::ClientDataService* client_data) { client_data_ = client_data; }
    void setSelectCallback(SelectCallback callback) { on_select_ = std::move(callback); }
    
    /**
     * Open the popup
     */
    void open();
    
    /**
     * Close the popup
     */
    void close();
    
    /**
     * Check if popup is open
     */
    bool isOpen() const { return is_open_; }
    
    /**
     * Render the popup (call each frame)
     */
    void render();
    
private:
    void doSearch();
    void handleKeyboardNavigation();
    void selectCurrent();
    
    AppLogic::ItemPickerService* picker_ = nullptr;
    Services::SpriteManager* sprite_manager_ = nullptr;
    Services::ClientDataService* client_data_ = nullptr;
    SelectCallback on_select_;
    
    bool is_open_ = false;
    bool focus_input_ = false;
    char search_buffer_[256] = {};
    std::string last_query_;
    
    std::vector<Domain::Search::PickResult> results_;
    int selected_index_ = 0;
    
    static constexpr size_t MAX_VISIBLE_RESULTS = 10;
    static constexpr size_t SEARCH_LIMIT = 50;
};

} // namespace UI
} // namespace MapEditor
