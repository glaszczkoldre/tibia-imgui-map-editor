#include "PreviewUtils.hpp"
#include "Services/SpriteManager.h"
#include "Services/ClientDataService.h"
#include "Rendering/Tile/CreatureSpriteHelper.h"
#include "Rendering/Core/Texture.h"
#include <imgui.h>

namespace MapEditor::UI::Utils {

Rendering::Texture* GetItemPreview(Services::SpriteManager& spriteManager,
                                   const Domain::ItemType* itemType) {
    if (!itemType) {
        return nullptr;
    }
    return spriteManager.getItemCompositor().getCompositedItemTexture(itemType);
}

namespace {
template <typename T>
CreaturePreviewResult GetCreaturePreviewImpl(Services::ClientDataService& clientData,
                                             Services::SpriteManager& spriteManager,
                                             const T& identifier) {
    CreaturePreviewResult result;
    Rendering::CreatureSpriteHelper helper(&clientData, &spriteManager);
    result.texture = helper.getThumbnail(identifier);
    if (result.texture) {
        result.size = helper.getRecommendedSize(identifier);
    }
    return result;
}
} // namespace

CreaturePreviewResult GetCreaturePreview(Services::ClientDataService& clientData,
                                         Services::SpriteManager& spriteManager,
                                         const std::string& name) {
    if (name.empty()) {
        return {};
    }
    return GetCreaturePreviewImpl(clientData, spriteManager, name);
}

CreaturePreviewResult GetCreaturePreview(Services::ClientDataService& clientData,
                                         Services::SpriteManager& spriteManager,
                                         const Domain::Outfit& outfit) {
    return GetCreaturePreviewImpl(clientData, spriteManager, outfit);
}

bool RenderPreviewCard(Rendering::Texture* texture, float size,
                       bool is_selected, float rounding, float padding) {
    ImVec2 min = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(size, size));
    bool is_hovered = ImGui::IsItemHovered();
    bool is_clicked = ImGui::IsItemClicked();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 max(min.x + size, min.y + size);

    ImU32 bg_col = ImGui::GetColorU32(ImGuiCol_FrameBg);
    ImU32 border_col = ImGui::GetColorU32(ImGuiCol_Border);

    if (is_selected) {
        bg_col = ImGui::GetColorU32(ImGuiCol_Header);
        border_col = IM_COL32(255, 200, 0, 255);
    } else if (is_hovered) {
        bg_col = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
    }

    dl->AddRectFilled(min, max, bg_col, rounding);
    dl->AddRect(min, max, border_col, rounding);

    if (texture) {
        ImVec2 img_min(min.x + padding, min.y + padding);
        ImVec2 img_max(max.x - padding, max.y - padding);
        dl->AddImageRounded(
            (void*)(intptr_t)texture->id(), img_min, img_max,
            ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, rounding);
    }

    return is_clicked;
}

} // namespace MapEditor::UI::Utils
