#include "PreviewUtils.hpp"
#include "Services/SpriteManager.h"
#include "Services/ClientDataService.h"
#include "Rendering/Tile/CreatureSpriteHelper.h"
#include "Rendering/Core/Texture.h"

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
    // Basic validity check for outfit could go here, but helper handles it.
    return GetCreaturePreviewImpl(clientData, spriteManager, outfit);
}

} // namespace MapEditor::UI::Utils
