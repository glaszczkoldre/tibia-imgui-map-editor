#pragma once
#include "Domain/Spawn.h"
#include "Domain/Position.h"
#include "BasePropertiesDialog.h"

namespace MapEditor::UI {

/**
 * Modal dialog for editing spawn properties.
 * Edits: Spawn Radius.
 */
class SpawnPropertiesDialog : public BasePropertiesDialog {
public:
    SpawnPropertiesDialog() = default;
    
    /**
     * Open dialog for the given spawn.
     */
    void open(Domain::Spawn* spawn, const Domain::Position& pos, SaveCallback on_save = nullptr);
    
    // BasePropertiesDialog implementation
    void renderContent() override;
    void onSave() override;
    void onClose() override;
    
    // Expose render() from base class with specific title
    void render() { BasePropertiesDialog::render("Spawn Properties", ImVec2(280, 0)); }
    
private:
    Domain::Spawn* current_spawn_ = nullptr;
    Domain::Position spawn_position_;
    
    // Editable values
    int radius_ = 1;
};

} // namespace MapEditor::UI
