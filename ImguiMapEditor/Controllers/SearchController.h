#pragma once
#include "Services/ItemPickerService.h"
#include "Services/Map/MapSearchService.h"
#include "UI/Widgets/QuickSearchPopup.h"
#include "UI/Dialogs/AdvancedSearchDialog.h"
#include "UI/Widgets/SearchResultsWidget.h"
#include "Services/ViewSettings.h"
#include <memory>

namespace MapEditor {

namespace Services {
    class ClientDataService;
    class SpriteManager;
}

namespace Domain {
    class ChunkedMap;
}

namespace AppLogic {

/**
 * Orchestrates search functionality, managing search services and UI widgets.
 */
class SearchController {
public:
    SearchController();
    ~SearchController();

    /**
     * Update search components when a map is loaded.
     * Wires services with the new map and client data.
     */
    void onMapLoaded(
        Domain::ChunkedMap* map,
        Services::ClientDataService* client_data,
        Services::SpriteManager* sprite_manager,
        Services::ViewSettings* view_settings
    );

    // Accessors for UI components (needed for rendering and callbacks)
    UI::QuickSearchPopup* getQuickSearchPopup() { return &quick_search_popup_; }
    UI::AdvancedSearchDialog* getAdvancedSearchDialog() { return &advanced_search_dialog_; }
    UI::SearchResultsWidget* getSearchResultsWidget() { return &search_results_widget_; }

private:
    // UI Components
    UI::QuickSearchPopup quick_search_popup_;
    UI::AdvancedSearchDialog advanced_search_dialog_;
    UI::SearchResultsWidget search_results_widget_;

    // Services
    std::unique_ptr<AppLogic::ItemPickerService> item_picker_service_;
    std::unique_ptr<Services::MapSearchService> map_search_service_;

    // State tracking
    Services::ClientDataService* current_client_data_ = nullptr;
};

} // namespace AppLogic
} // namespace MapEditor
