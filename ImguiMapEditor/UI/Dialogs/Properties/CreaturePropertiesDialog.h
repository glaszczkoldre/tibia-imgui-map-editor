#pragma once
#include "Domain/Creature.h"
#include "Domain/Position.h"
#include "BasePropertiesDialog.h"

namespace MapEditor::UI {

/**
 * Modal dialog for editing creature properties.
 * Edits: Spawn Time, Direction.
 */
class CreaturePropertiesDialog : public BasePropertiesDialog {
public:
    CreaturePropertiesDialog() = default;
    
    /**
     * Open dialog for the given creature.
     */
    void open(Domain::Creature* creature, const std::string& creature_name,
              const Domain::Position& creature_pos, SaveCallback on_save = nullptr);
    
    // BasePropertiesDialog implementation
    void renderContent() override;
    void onSave() override;
    void onClose() override;

    // Expose render() from base class with specific title
    void render() { BasePropertiesDialog::render("Creature Properties", ImVec2(300, 0)); }
    
private:
    Domain::Creature* current_creature_ = nullptr;
    std::string creature_name_;
    Domain::Position creature_pos_;
    
    // Editable values
    int spawn_time_ = 60;  // seconds
    int direction_ = 2;    // 0=North, 1=East, 2=South, 3=West
};

} // namespace MapEditor::UI

