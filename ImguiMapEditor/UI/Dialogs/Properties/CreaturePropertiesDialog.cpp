#include "CreaturePropertiesDialog.h"
#include "ext/fontawesome6/IconsFontAwesome6.h"
#include <imgui.h>
#include "Core/Config.h"

namespace MapEditor::UI {

static const char* DIRECTION_NAMES[] = {"North", "East", "South", "West"};

void CreaturePropertiesDialog::open(Domain::Creature* creature, 
                                     const std::string& creature_name,
                                     const Domain::Position& creature_pos,
                                     SaveCallback on_save) {
    if (!creature) return;
    
    current_creature_ = creature;
    creature_name_ = creature_name;
    creature_pos_ = creature_pos;
    save_callback_ = on_save;
    is_open_ = true;
    
    // Load current values
    spawn_time_ = creature->spawn_time;
    direction_ = creature->direction;
    
    // Validate direction
    if (direction_ < 0 || direction_ > 3) direction_ = 2; // Default south
}

void CreaturePropertiesDialog::renderContent() {
    if (!current_creature_) return;
    
    ImGui::Text("Creature: %s", creature_name_.c_str());
    ImGui::Text("Position: %d, %d, %d", creature_pos_.x, creature_pos_.y, creature_pos_.z);
    ImGui::Separator();
    
    // Spawn Time
    ImGui::InputInt("Spawn Time (s)", &spawn_time_);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Respawn time in seconds (10-86400)");
    if (spawn_time_ < 10) spawn_time_ = 10;
    if (spawn_time_ > 86400) spawn_time_ = 86400;

    // Show as minutes:seconds
    int minutes = spawn_time_ / 60;
    int seconds = spawn_time_ % 60;
    ImGui::Text("(%d min %d sec)", minutes, seconds);
    
    ImGui::Spacing();

    // Direction
    ImGui::Combo("Direction", &direction_, DIRECTION_NAMES, 4);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Creature's facing direction");
}

void CreaturePropertiesDialog::onSave() {
    if (current_creature_) {
        current_creature_->spawn_time = spawn_time_;
        current_creature_->direction = direction_;
    }
}

void CreaturePropertiesDialog::onClose() {
    BasePropertiesDialog::onClose();
    current_creature_ = nullptr;
}

} // namespace MapEditor::UI

