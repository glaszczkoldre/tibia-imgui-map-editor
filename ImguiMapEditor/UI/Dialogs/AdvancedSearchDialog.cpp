#include "AdvancedSearchDialog.h"
#include "UI/Widgets/SearchResultsWidget.h"
#include "Services/Map/MapSearchService.h"
#include "Services/ClientDataService.h"
#include "Services/SpriteManager.h"
#include "Domain/ItemType.h"
#include "Domain/CreatureType.h"
#include "IO/Readers/DatReaderBase.h"
#include "ext/fontawesome6/IconsFontAwesome6.h"
#include "Rendering/Core/Texture.h"
#include "Presentation/NotificationHelper.h"
#include "UI/Utils/PreviewUtils.hpp"
#include <algorithm>
#include <cctype>

namespace MapEditor::UI {

std::string PreviewResult::getDisplayName() const {
    if (is_creature && creature) return creature->name;
    if (item) return item->name.empty() ? "(unnamed)" : item->name;
    return "(unknown)";
}

uint16_t PreviewResult::getServerId() const {
    if (item) return item->server_id;
    return 0;
}

AdvancedSearchDialog::AdvancedSearchDialog() {
    search_buffer_[0] = '\0';
}

void AdvancedSearchDialog::render() {
    if (!is_open_) return;

    ImGui::SetNextWindowSize(ImVec2(850, 600), ImGuiCond_FirstUseEver);

    if (ImGui::Begin(ICON_FA_MAGNIFYING_GLASS_PLUS " Advanced Search###AdvancedSearch", &is_open_,
                     ImGuiWindowFlags_NoCollapse)) {

        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float footer_h = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y * 2.0f;
        const float content_h = ImGui::GetContentRegionAvail().y - footer_h;
        const float filters_w = 320.0f;
        const float results_w = ImGui::GetContentRegionAvail().x - filters_w - spacing;

        // === LEFT PANEL: Filters ===
        ImGui::BeginChild("##filters_panel", ImVec2(filters_w, content_h), ImGuiChildFlags_Borders);
        renderFiltersPanel();
        ImGui::EndChild();

        ImGui::SameLine(0.0f, spacing);

        // === RIGHT PANEL: Results ===
        ImGui::BeginChild("##results_panel", ImVec2(results_w, content_h), ImGuiChildFlags_Borders);
        renderResultsColumn();
        ImGui::EndChild();

        // === FOOTER === always visible
        renderBottomBar();
    }
    ImGui::End();

    if (is_open_ && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        close();
    }
}

void AdvancedSearchDialog::renderFiltersPanel() {
    renderSearchSection();
    ImGui::Spacing();
    renderOrSection();
    ImGui::Spacing();
    renderAndSection();
    ImGui::Spacing();
    renderHintsSection();
}

void AdvancedSearchDialog::renderSearchSection() {
    ImGui::Text(ICON_FA_MAGNIFYING_GLASS " Search");
    ImGui::Separator();

    if (focus_input_) {
        ImGui::SetKeyboardFocusHere();
        focus_input_ = false;
    }

    ImGui::PushItemWidth(-1);
    if (ImGui::InputTextWithHint("##SearchInput", "Name or ID...",
                                  search_buffer_, sizeof(search_buffer_))) {
        filters_changed_ = true;
    }
    ImGui::PopItemWidth();
}

void AdvancedSearchDialog::renderOrSection() {
    ImGui::Text(ICON_FA_CUBES " OR");
    ImGui::SameLine();
    ImGui::TextDisabled("(any match)");
    ImGui::Separator();

    if (ImGui::BeginTable("OrColumns", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::PushID("OrLeft");
        if (ImGui::Checkbox("Depot", &type_filter_.depot)) filters_changed_ = true;
        if (ImGui::Checkbox("Trash Holder", &type_filter_.trash_holder)) filters_changed_ = true;
        if (ImGui::Checkbox("Door", &type_filter_.door)) filters_changed_ = true;
        if (ImGui::Checkbox("Teleport", &type_filter_.teleport)) filters_changed_ = true;
        if (ImGui::Checkbox("Key", &type_filter_.key)) filters_changed_ = true;
        if (ImGui::Checkbox("Weapon", &type_filter_.weapon)) filters_changed_ = true;
        if (ImGui::Checkbox("Armor", &type_filter_.armor)) filters_changed_ = true;
        if (ImGui::Checkbox("Creature", &type_filter_.creature)) filters_changed_ = true;
        ImGui::PopID();

        ImGui::TableSetColumnIndex(1);
        ImGui::PushID("OrRight");
        if (ImGui::Checkbox("Mailbox", &type_filter_.mailbox)) filters_changed_ = true;
        if (ImGui::Checkbox("Container", &type_filter_.container)) filters_changed_ = true;
        if (ImGui::Checkbox("Magic Field", &type_filter_.magic_field)) filters_changed_ = true;
        if (ImGui::Checkbox("Bed", &type_filter_.bed)) filters_changed_ = true;
        if (ImGui::Checkbox("Podium", &type_filter_.podium)) filters_changed_ = true;
        if (ImGui::Checkbox("Ammo", &type_filter_.ammo)) filters_changed_ = true;
        if (ImGui::Checkbox("Rune", &type_filter_.rune)) filters_changed_ = true;
        ImGui::PopID();

        ImGui::EndTable();
    }
}

void AdvancedSearchDialog::renderAndSection() {
    ImGui::Text(ICON_FA_SLIDERS " AND");
    ImGui::SameLine();
    ImGui::TextDisabled("(all must match)");
    ImGui::Separator();

    if (ImGui::BeginTable("AndColumns", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::PushID("AndLeft");
        if (ImGui::Checkbox("Unpassable", &property_filter_.unpassable)) filters_changed_ = true;
        if (ImGui::Checkbox("Block Missiles", &property_filter_.block_missiles)) filters_changed_ = true;
        if (ImGui::Checkbox("Has Elevation", &property_filter_.has_elevation)) filters_changed_ = true;
        if (ImGui::Checkbox("Full Tile", &property_filter_.full_tile)) filters_changed_ = true;
        if (ImGui::Checkbox("Writeable", &property_filter_.writeable)) filters_changed_ = true;
        if (ImGui::Checkbox("Force Use", &property_filter_.force_use)) filters_changed_ = true;
        if (ImGui::Checkbox("Rotatable", &property_filter_.rotatable)) filters_changed_ = true;
        if (ImGui::Checkbox("Has Light", &property_filter_.has_light)) filters_changed_ = true;
        if (ImGui::Checkbox("Always Top", &property_filter_.always_on_top)) filters_changed_ = true;
        if (ImGui::Checkbox("Has Charges", &property_filter_.has_charges)) filters_changed_ = true;
        if (ImGui::Checkbox("Decays", &property_filter_.decays)) filters_changed_ = true;
        if (ImGui::Checkbox("Dist Read", &property_filter_.allow_dist_read)) filters_changed_ = true;
        if (ImGui::Checkbox("Hangable", &property_filter_.hangable)) filters_changed_ = true;
        if (ImGui::Checkbox("Stackable", &property_filter_.stackable)) filters_changed_ = true;
        ImGui::PopID();

        ImGui::TableSetColumnIndex(1);
        ImGui::PushID("AndRight");
        if (ImGui::Checkbox("Unmovable", &property_filter_.unmovable)) filters_changed_ = true;
        if (ImGui::Checkbox("Block Pathfinder", &property_filter_.block_pathfinder)) filters_changed_ = true;
        if (ImGui::Checkbox("Floor Change", &property_filter_.floor_change)) filters_changed_ = true;
        if (ImGui::Checkbox("Readable", &property_filter_.readable)) filters_changed_ = true;
        if (ImGui::Checkbox("Pickupable", &property_filter_.pickupable)) filters_changed_ = true;
        if (ImGui::Checkbox("Animation", &property_filter_.animation)) filters_changed_ = true;
        if (ImGui::Checkbox("Ignore Look", &property_filter_.ignore_look)) filters_changed_ = true;
        if (ImGui::Checkbox("Client Charges", &property_filter_.client_charges)) filters_changed_ = true;
        if (ImGui::Checkbox("Has Speed", &property_filter_.has_speed)) filters_changed_ = true;
        if (ImGui::Checkbox("Hook East", &property_filter_.hook_east)) filters_changed_ = true;
        if (ImGui::Checkbox("Hook South", &property_filter_.hook_south)) filters_changed_ = true;
        ImGui::PopID();

        ImGui::EndTable();
    }
}

void AdvancedSearchDialog::renderHintsSection() {
    ImGui::Text(ICON_FA_CIRCLE_INFO " Hints");
    ImGui::Separator();
    ImGui::TextDisabled("Double-click result to search map.");
    ImGui::TextDisabled("Leave search empty to filter by types/properties only.");
}

void AdvancedSearchDialog::renderResultsColumn() {
    if (filters_changed_) {
        updatePreviewResults();
        filters_changed_ = false;
    }

    ImGui::Text(ICON_FA_LIST " Result (%zu)", preview_results_.size());
    ImGui::Separator();

    if (preview_results_.empty()) {
        ImGui::Spacing();
        ImGui::TextDisabled("No matching items");
        ImGui::TextDisabled("Enter search term or select filters");
        return;
    }

    ImGui::TextDisabled("Double-click to search map");
    ImGui::Spacing();

    constexpr float SPRITE_SIZE = 24.0f;
    constexpr float ROW_HEIGHT = 28.0f;

    for (size_t i = 0; i < preview_results_.size(); ++i) {
        const auto& result = preview_results_[i];
        bool is_selected = (static_cast<int>(i) == selected_preview_index_);

        ImGui::PushID(static_cast<int>(i));

        if (ImGui::Selectable("##Row", is_selected, ImGuiSelectableFlags_AllowDoubleClick,
                              ImVec2(0, ROW_HEIGHT))) {
            selected_preview_index_ = static_cast<int>(i);
            if (ImGui::IsMouseDoubleClicked(0)) {
                onSearchMap();
            }
        }

        ImGui::SameLine(4);

        bool sprite_rendered = false;
        if (sprite_manager_ && client_data_) {
            if (result.is_creature && result.creature) {
                auto preview = Utils::GetCreaturePreview(*client_data_, *sprite_manager_, result.creature->outfit);
                if (preview && preview.texture) {
                    ImGui::Image((void*)(intptr_t)preview.texture->id(), ImVec2(SPRITE_SIZE, SPRITE_SIZE));
                    sprite_rendered = true;
                }
            } else if (result.item) {
                if (auto* texture = Utils::GetItemPreview(*sprite_manager_, result.item)) {
                    ImGui::Image((void*)(intptr_t)texture->id(), ImVec2(SPRITE_SIZE, SPRITE_SIZE));
                    sprite_rendered = true;
                }
            }
        }

        if (!sprite_rendered) {
            ImGui::Dummy(ImVec2(SPRITE_SIZE, SPRITE_SIZE));
            ImGui::SameLine(4);
            ImGui::Text("%s", result.is_creature ? ICON_FA_DRAGON : ICON_FA_CUBE);
        }

        ImGui::SameLine();

        if (result.is_creature) {
            ImGui::Text("%s", result.getDisplayName().c_str());
        } else {
            ImGui::Text("[%u] %s", result.getServerId(), result.getDisplayName().c_str());
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            if (result.is_creature && result.creature) {
                ImGui::Text("Creature: %s", result.creature->name.c_str());
                ImGui::Text("LookType: %u", result.creature->outfit.lookType);
            } else if (result.item) {
                ImGui::Text("Server ID: %u", result.item->server_id);
                ImGui::Text("Client ID: %u", result.item->client_id);
                if (!result.item->name.empty()) {
                    ImGui::Text("Name: %s", result.item->name.c_str());
                }
            }
            ImGui::EndTooltip();
        }

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 30);

        if (ImGui::Button(ICON_FA_COPY)) {
            if (result.is_creature && result.creature) {
                ImGui::SetClipboardText(result.creature->name.c_str());
                Presentation::showSuccess("Creature name copied");
            } else if (result.item) {
                ImGui::SetClipboardText(std::to_string(result.item->server_id).c_str());
                Presentation::showSuccess("Item ID copied");
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Copy ID/Name to clipboard");
        }

        ImGui::PopID();
    }
}

void AdvancedSearchDialog::renderBottomBar() {
    float button_w = 120.0f;
    float total_buttons_w = button_w * 3.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f;

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);

    // Result count on the left
    ImGui::Text("Results: %zu", preview_results_.size());
    ImGui::SameLine();

    // Buttons right-aligned
    float offset_x = ImGui::GetContentRegionAvail().x - total_buttons_w;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset_x);

    bool can_search = search_service_ && selected_preview_index_ >= 0;
    ImGui::BeginDisabled(!can_search);
    if (ImGui::Button(ICON_FA_MAP " Search Map", ImVec2(button_w, 0))) {
        onSearchMap();
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !can_search) {
        ImGui::SetTooltip("Select an item from results first");
    }

    ImGui::SameLine();

    ImGui::BeginDisabled(true);
    if (ImGui::Button(ICON_FA_HAND_POINTER " Select Item", ImVec2(button_w, 0))) {
        onSelectItem();
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Select item as brush (Coming Soon)");
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_XMARK " Cancel", ImVec2(button_w, 0))) {
        close();
    }
}

void AdvancedSearchDialog::updatePreviewResults() {
    preview_results_.clear();
    selected_preview_index_ = -1;

    bool has_query = strlen(search_buffer_) > 0;
    bool has_type_filter = type_filter_.hasAnySelected();
    bool has_property_filter = property_filter_.hasAnySelected();

    if (!has_query && !has_type_filter && !has_property_filter) return;

    std::string query_lower = search_buffer_;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    bool search_items = !type_filter_.creature || type_filter_.hasAnySelected();
    if (search_items && search_service_ && !property_filter_.hasAnySelected() || !type_filter_.creature) {
        if (!(type_filter_.creature && !type_filter_.depot && !type_filter_.mailbox &&
              !type_filter_.trash_holder && !type_filter_.container && !type_filter_.door &&
              !type_filter_.magic_field && !type_filter_.teleport && !type_filter_.bed &&
              !type_filter_.key && !type_filter_.podium)) {

            auto item_results = search_service_->searchItemDatabase(
                search_buffer_, type_filter_, property_filter_, 10000);

            for (const auto* item : item_results) {
                preview_results_.push_back({false, item, nullptr});
            }
        }
    }

    bool search_creatures = type_filter_.creature || (!has_type_filter && has_query);
    if (search_creatures && client_data_ && !property_filter_.hasAnySelected()) {
        const auto& creatures = client_data_->getCreatures();
        for (const auto& creature_ptr : creatures) {
            if (!creature_ptr) continue;
            const auto* creature = creature_ptr.get();

            if (has_query) {
                std::string name_lower = creature->name;
                std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                if (name_lower.find(query_lower) == std::string::npos) continue;
            }

            preview_results_.push_back({true, nullptr, creature});
        }
    }
}

void AdvancedSearchDialog::onSearchMap() {
    if (!search_service_ || selected_preview_index_ < 0 ||
        selected_preview_index_ >= static_cast<int>(preview_results_.size())) {
        return;
    }

    const auto& selected = preview_results_[selected_preview_index_];
    std::vector<Domain::Search::MapSearchResult> results;

    if (selected.is_creature && selected.creature) {
        results = search_service_->search(selected.creature->name, Services::MapSearchMode::ByName, false, true, 1000);
    } else if (selected.item) {
        results = search_service_->search(std::to_string(selected.item->server_id), Services::MapSearchMode::ByServerId, true, false, 1000);
    }

    if (results_widget_) results_widget_->setResults(results);
    if (view_settings_) *view_settings_ = true;
}

void AdvancedSearchDialog::onSelectItem() {
    // Placeholder
}

} // namespace MapEditor::UI
