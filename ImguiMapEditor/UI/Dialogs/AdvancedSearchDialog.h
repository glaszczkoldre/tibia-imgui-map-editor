#pragma once

#include <imgui.h>
#include <string>
#include <functional>
#include <vector>
#include <variant>
#include "Domain/Search/MapSearchResult.h"
#include "Domain/Search/SearchFilterTypes.h"
namespace MapEditor {
namespace Domain { 
    class ItemType; 
    class CreatureType;
}
namespace AppLogic { 
    class ItemPickerService;
}
namespace Services { 
    class ClientDataService; 
    class SpriteManager;
    class MapSearchService;
}

namespace UI {

class SearchResultsWidget;

/**
 * Preview result - can be either an Item or a Creature.
 */
struct PreviewResult {
    bool is_creature = false;
    const Domain::ItemType* item = nullptr;
    const Domain::CreatureType* creature = nullptr;
    
    // Display helpers
    std::string getDisplayName() const;
    uint16_t getServerId() const;
};

/**
 * Advanced Search dialog (Ctrl+Shift+F) - RME-style item/map search.
 * 
 * 4-column layout:
 * 1. Find By - unified fuzzy search (auto-detects name vs ID)
 * 2. Types - multi-select toggles (OR logic)
 * 3. Properties - multi-select toggles (AND logic)  
 * 4. Results - preview of matching items/creatures from database
 * 
 * Bottom buttons: Search Map, Select Item (placeholder), Cancel
 */
class AdvancedSearchDialog {
public:
    AdvancedSearchDialog();
    ~AdvancedSearchDialog() = default;
    
    // Non-copyable
    AdvancedSearchDialog(const AdvancedSearchDialog&) = delete;
    AdvancedSearchDialog& operator=(const AdvancedSearchDialog&) = delete;
    
    // Dependencies
    void setItemPickerService(AppLogic::ItemPickerService* picker) { (void)picker; }
    void setMapSearchService(Services::MapSearchService* service) { search_service_ = service; }
    void setClientDataService(Services::ClientDataService* service) { client_data_ = service; }
    void setSpriteManager(Services::SpriteManager* sprites) { sprite_manager_ = sprites; }
    void setSearchResultsWidget(SearchResultsWidget* widget) { results_widget_ = widget; }
    void setShowSearchResultsToggle(bool* toggle) { view_settings_ = toggle; }
    
    void open() { is_open_ = true; focus_input_ = true; updatePreviewResults(); }
    void close() { is_open_ = false; }
    bool isOpen() const { return is_open_; }
    
    void render();
    
private:
    // Render helpers for 4-column layout
    void renderFindByColumn();
    void renderTypesColumn();
    void renderPropertiesColumn();
    void renderResultsColumn();
    void renderBottomBar();
    
    // Actions
    void updatePreviewResults();
    void onSearchMap();
    void onSelectItem();
    
    // Dependencies
    Services::MapSearchService* search_service_ = nullptr;
    Services::ClientDataService* client_data_ = nullptr;
    Services::SpriteManager* sprite_manager_ = nullptr;
    SearchResultsWidget* results_widget_ = nullptr;
    bool* view_settings_ = nullptr;
    
    // Dialog state
    bool is_open_ = false;
    bool focus_input_ = false;
    bool filters_changed_ = true;  // Trigger preview update
    
    // === COLUMN 1: Find By ===
    char search_buffer_[256] = {};
    
    // === COLUMN 2: Types (multi-select, OR logic) ===
    Domain::Search::TypeFilter type_filter_;
    
    // === COLUMN 3: Properties (multi-select, AND logic) ===
    Domain::Search::PropertyFilter property_filter_;
    
    // === COLUMN 4: Results Preview ===
    std::vector<PreviewResult> preview_results_;
    int selected_preview_index_ = -1;
};

} // namespace UI
} // namespace MapEditor
