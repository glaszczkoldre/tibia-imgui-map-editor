#pragma once

#include <string>

namespace MapEditor::Services {
class SpriteManager;
class ClientDataService;
} // namespace MapEditor::Services

namespace MapEditor::Domain {
class ItemType;
struct Outfit;
} // namespace MapEditor::Domain

namespace MapEditor::Rendering {
class Texture;
} // namespace MapEditor::Rendering

namespace MapEditor::UI::Utils {

// Retrieves a preview texture for an item. Returns nullptr if not available.
Rendering::Texture* GetItemPreview(Services::SpriteManager& spriteManager,
                                   const Domain::ItemType* itemType);

// Result struct for creature preview to include recommended size
struct CreaturePreviewResult {
    Rendering::Texture* texture = nullptr;
    float size = 32.0f; // Recommended size in pixels (default 32)

    operator bool() const { return texture != nullptr; }
};

// Retrieves a preview texture and recommended size for a creature by name.
CreaturePreviewResult GetCreaturePreview(Services::ClientDataService& clientData,
                                         Services::SpriteManager& spriteManager,
                                         const std::string& name);

// Retrieves a preview texture and recommended size for a creature by outfit.
CreaturePreviewResult GetCreaturePreview(Services::ClientDataService& clientData,
                                         Services::SpriteManager& spriteManager,
                                         const Domain::Outfit& outfit);

} // namespace MapEditor::UI::Utils
