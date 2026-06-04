#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include "Domain/Search/MapSearchResult.h"
#include "Domain/Search/SearchFilterTypes.h"
namespace MapEditor {
namespace Domain { 
    class ItemType; 
    class CreatureType;
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
 * Advanced Search dialog - RME-style item/map search.
 * 
 * 2-column layout:
 * Left column: 4 bordered sections (Search, OR types, AND properties, Hints)
 * Right column: Results preview of matching items/creatures from database
 * 
 * Footer: result count + Search Map, Select Item (placeholder), Cancel buttons
 */
class AdvancedSearchDialog {
public:
    AdvancedSearchDialog();
    ~AdvancedSearchDialog() = default;
    
    // Non-copyable
    AdvancedSearchDialog(const AdvancedSearchDialog&) = delete;
    AdvancedSearchDialog& operator=(const AdvancedSearchDialog&) = delete;
    
    // Dependencies
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
    // Render helpers for 2-column layout
    void renderFiltersPanel();
    void renderSearchSection();
    void renderOrSection();
    void renderAndSection();
    void renderHintsSection();
    void renderResultsColumn();
    void renderBottomBar();
    
    // Actions
    void updatePreviewResults();
    void onSearchMap();
    void executeMapSearch();
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
    bool filters_changed_ = false;
    bool pending_map_search_ = false;
    
    char search_buffer_[256] = {};
    
    Domain::Search::TypeFilter type_filter_;
    Domain::Search::PropertyFilter property_filter_;
    
    std::vector<PreviewResult> preview_results_;
    int selected_preview_index_ = -1;
};

} // namespace UI
} // namespace MapEditor
