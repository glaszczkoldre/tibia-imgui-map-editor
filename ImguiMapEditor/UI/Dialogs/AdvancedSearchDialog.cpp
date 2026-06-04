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
#include <format>

namespace MapEditor::UI {

static inline ImTextureID TexId(uint32_t id) { return (ImTextureID)(intptr_t)id; }

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

    // Execute deferred map search from previous frame
    if (pending_map_search_) {
        pending_map_search_ = false;
        executeMapSearch();
    }

    ImGui::SetNextWindowSize(ImVec2(850, 600), ImGuiCond_Once);

    if (ImGui::Begin(ICON_FA_MAGNIFYING_GLASS_PLUS " Advanced Search###AdvancedSearch", &is_open_,
                     ImGuiWindowFlags_NoCollapse)) {

        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float footer_pad = ImGui::GetStyle().ItemSpacing.y * 5.0f;
        const float footer_h = ImGui::GetFrameHeightWithSpacing() + footer_pad;
        const float content_h = ImGui::GetContentRegionAvail().y - footer_h;
        const float filters_w = ImGui::GetContentRegionAvail().x * 0.35f;
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

        // === FOOTER === always visible, bordered
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
    ImGui::SeparatorText(ICON_FA_MAGNIFYING_GLASS " Search");

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
    ImGui::SeparatorText(ICON_FA_CUBES " OR  (any match)");

    if (ImGui::BeginTable("OrColumns", 2, ImGuiTableFlags_SizingStretchSame)) {
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
    ImGui::SeparatorText(ICON_FA_SLIDERS " AND  (all must match)");

    if (ImGui::BeginTable("AndColumns", 2, ImGuiTableFlags_SizingStretchSame)) {
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
    ImGui::SeparatorText(ICON_FA_CIRCLE_INFO " Hints");
    ImGui::TextDisabled("Double-click result to search map.");
    ImGui::TextDisabled("Leave search empty to filter by types/properties only.");
}

void AdvancedSearchDialog::renderResultsColumn() {
    if (filters_changed_) {
        updatePreviewResults();
        filters_changed_ = false;
    }

    auto result_label = std::format("Result ({})", preview_results_.size());
    ImGui::SeparatorText(result_label.c_str());

    // State-based empty messages
    if (preview_results_.empty()) {
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        bool has_query = strlen(search_buffer_) > 0;
        bool has_filters = type_filter_.hasAnySelected() || property_filter_.hasAnySelected();

        const char* icon;
        const char* msg;
        if (!has_query && !has_filters) {
            icon = ICON_FA_KEYBOARD;
            msg = "Type to search or select filters";
        } else {
            icon = ICON_FA_CIRCLE_EXCLAMATION;
            msg = "No matching items";
        }

        std::string text = std::format("{} {}", icon, msg);
        ImVec2 tsize = ImGui::CalcTextSize(text.c_str());
        ImGui::SetCursorPos(ImVec2(
            (ImGui::GetContentRegionAvail().x - tsize.x) * 0.5f,
            wsize.y * 0.4f));
        ImGui::TextDisabled("%s", text.c_str());
        return;
    }

    ImGui::TextDisabled("Hover for details. Double-click to search map.");
    ImGui::Spacing();

    constexpr float CELL_SIZE = 32.0f;
    constexpr float PREVIEW_SIZE = 64.0f;
    constexpr float CARD_ROUNDING = 4.0f;
    constexpr float IMG_ROUNDING = 3.0f;
    constexpr float IMG_PADDING = 2.0f;
    const float pad = ImGui::GetStyle().WindowPadding.x;
    const float avail_w = ImGui::GetContentRegionAvail().x;
    const int columns = std::max(1, static_cast<int>((avail_w - pad) / CELL_SIZE));
    const int total_rows = (static_cast<int>(preview_results_.size()) + columns - 1) / columns;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 col_bg = ImGui::GetColorU32(ImGuiCol_FrameBg);
    const ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
    const ImU32 col_selected = ImGui::GetColorU32(ImGuiCol_Header);
    const ImU32 col_hovered = ImGui::GetColorU32(ImGuiCol_HeaderHovered);

    ImGuiListClipper clipper;
    clipper.Begin(total_rows, CELL_SIZE);
    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
            for (int col = 0; col < columns; ++col) {
                int idx = row * columns + col;
                if (idx >= static_cast<int>(preview_results_.size())) break;

                const auto& result = preview_results_[idx];
                bool is_selected = (idx == selected_preview_index_);

                ImGui::PushID(idx);

                // Load sprite texture
                uint32_t tex_id = 0;
                bool sprite_rendered = false;
                if (sprite_manager_ && client_data_) {
                    if (result.is_creature && result.creature) {
                        auto preview = Utils::GetCreaturePreview(*client_data_, *sprite_manager_, result.creature->outfit);
                        if (preview && preview.texture) {
                            tex_id = preview.texture->id();
                            sprite_rendered = true;
                        }
                    } else if (result.item) {
                        if (auto* texture = Utils::GetItemPreview(*sprite_manager_, result.item)) {
                            tex_id = texture->id();
                            sprite_rendered = true;
                        }
                    }
                }

                // Exact 32x32 hitbox
                ImGui::InvisibleButton("##cell", ImVec2(CELL_SIZE, CELL_SIZE));
                bool hovered = ImGui::IsItemHovered();
                bool clicked = ImGui::IsItemClicked();
                bool double_clicked = hovered && ImGui::IsMouseDoubleClicked(0);

                if (clicked) selected_preview_index_ = idx;
                if (double_clicked) onSearchMap();

                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();

                // Rounded card background
                ImU32 bg_col = is_selected ? col_selected : (hovered ? col_hovered : col_bg);
                dl->AddRectFilled(min, max, bg_col, CARD_ROUNDING);
                dl->AddRect(min, max, col_border, CARD_ROUNDING);

                // Rounded image inside card
                if (sprite_rendered) {
                    ImVec2 img_min(min.x + IMG_PADDING, min.y + IMG_PADDING);
                    ImVec2 img_max(max.x - IMG_PADDING, max.y - IMG_PADDING);
                    dl->AddImageRounded(TexId(tex_id), img_min, img_max,
                        ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, IMG_ROUNDING);
                }

                // Tooltip on hover
                if (hovered) {
                    ImGui::BeginTooltip();
                    if (sprite_rendered) {
                        ImGui::Image(TexId(tex_id), ImVec2(PREVIEW_SIZE, PREVIEW_SIZE));
                        ImGui::SameLine();
                    }
                    if (result.is_creature && result.creature) {
                        ImGui::TextUnformatted(result.creature->name.c_str());
                        ImGui::TextDisabled("Creature");
                    } else if (result.item) {
                        ImGui::TextUnformatted(result.item->name.empty() ? "(unnamed)" : result.item->name.c_str());
                        ImGui::TextDisabled("SID: %u", result.item->server_id);
                        ImGui::TextDisabled("CID: %u", result.item->client_id);
                    }
                    ImGui::EndTooltip();
                }

                // Grid flow
                if (col < columns - 1 && idx + 1 < static_cast<int>(preview_results_.size())) {
                    ImGui::SameLine(0.0f, 0.0f);
                }

                ImGui::PopID();
            }
        }
    }
}

void AdvancedSearchDialog::renderBottomBar() {
    ImGui::BeginChild("##footer", ImVec2(0, 0), ImGuiChildFlags_Borders);

    // Center content vertically
    float child_h = ImGui::GetContentRegionAvail().y;
    float frame_h = ImGui::GetFrameHeight();
    float v_pad = (child_h - frame_h) * 0.5f;
    if (v_pad > 0)
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + v_pad);

    // Result count on the left
    auto count_label = std::format("Results: {}", preview_results_.size());
    ImGui::TextUnformatted(count_label.c_str());
    ImGui::SameLine();

    // Buttons right-aligned with padding so they don't touch the edge
    const char* search_label = ICON_FA_MAP " Search Map";
    const char* select_label = ICON_FA_HAND_POINTER " Select Item";
    const char* cancel_label = ICON_FA_XMARK " Cancel";
    float gap = ImGui::GetStyle().ItemSpacing.x;
    float pad = ImGui::GetStyle().WindowPadding.x;
    float total_w = ImGui::CalcTextSize(search_label).x + ImGui::CalcTextSize(select_label).x
                  + ImGui::CalcTextSize(cancel_label).x + gap * 6;  // 3 buttons × 2 padding each
    float avail = ImGui::GetContentRegionAvail().x;
    float offset_x = avail - total_w - pad;
    if (offset_x > 0)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset_x);

    bool can_search = search_service_ && selected_preview_index_ >= 0;
    ImGui::BeginDisabled(!can_search);
    if (ImGui::Button(search_label)) {
        onSearchMap();
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !can_search) {
        ImGui::SetTooltip("Select an item from results first");
    }

    ImGui::SameLine();

    ImGui::BeginDisabled(true);
    if (ImGui::Button(select_label)) {
        onSelectItem();
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Select item as brush (Coming Soon)");
    }

    ImGui::SameLine();

    if (ImGui::Button(cancel_label)) {
        close();
    }

    ImGui::EndChild();
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

    // Search items (skip if only creature/combat types selected — searchItemDatabase handles type+property filtering)
    bool creature_only = type_filter_.creature && !type_filter_.depot && !type_filter_.mailbox &&
        !type_filter_.trash_holder && !type_filter_.container && !type_filter_.door &&
        !type_filter_.magic_field && !type_filter_.teleport && !type_filter_.bed &&
        !type_filter_.key && !type_filter_.podium && !type_filter_.weapon &&
        !type_filter_.armor && !type_filter_.ammo && !type_filter_.rune;

    if (search_service_ && !creature_only) {
        auto item_results = search_service_->searchItemDatabase(
            search_buffer_, type_filter_, property_filter_, 10000);

        for (const auto* item : item_results) {
            preview_results_.push_back({false, item, nullptr});
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

    // Sort by server_id ascending, creatures (id=0) last
    std::sort(preview_results_.begin(), preview_results_.end(),
        [](const PreviewResult& a, const PreviewResult& b) {
            uint16_t a_id = a.is_creature ? UINT16_MAX : (a.item ? a.item->server_id : 0);
            uint16_t b_id = b.is_creature ? UINT16_MAX : (b.item ? b.item->server_id : 0);
            return a_id < b_id;
        });
}

void AdvancedSearchDialog::onSearchMap() {
    if (!search_service_ || selected_preview_index_ < 0 ||
        selected_preview_index_ >= static_cast<int>(preview_results_.size())) {
        return;
    }

    // Defer search to next frame
    pending_map_search_ = true;

    // Show SearchResultsWidget immediately in searching state
    if (results_widget_) results_widget_->setSearching(true);
    if (view_settings_) *view_settings_ = true;
}

void AdvancedSearchDialog::executeMapSearch() {
    if (selected_preview_index_ < 0 ||
        selected_preview_index_ >= static_cast<int>(preview_results_.size())) {
        return;
    }

    const auto& selected = preview_results_[selected_preview_index_];
    std::vector<Domain::Search::MapSearchResult> results;

    if (selected.is_creature && selected.creature) {
        // Set widget search buffer so empty state shows "No results found"
        if (results_widget_) results_widget_->setSearchBuffer(selected.creature->name.c_str());
        results = search_service_->search(selected.creature->name, Services::MapSearchMode::ByName, false, true, 1000);
    } else if (selected.item) {
        auto id_str = std::to_string(selected.item->server_id);
        if (results_widget_) results_widget_->setSearchBuffer(id_str.c_str());
        results = search_service_->search(id_str, Services::MapSearchMode::ByServerId, true, false, 1000);
    }

    if (results_widget_) results_widget_->setResults(results);
}

void AdvancedSearchDialog::onSelectItem() {
    // Placeholder
}

} // namespace MapEditor::UI
