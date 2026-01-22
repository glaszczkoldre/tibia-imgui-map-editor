#include "SpawnPropertiesDialog.h"
#include "ext/fontawesome6/IconsFontAwesome6.h"
#include <imgui.h>
#include "Core/Config.h"

namespace MapEditor::UI {

void SpawnPropertiesDialog::open(Domain::Spawn* spawn, const Domain::Position& pos, SaveCallback on_save) {
    if (!spawn) return;
    
    current_spawn_ = spawn;
    spawn_position_ = pos;
    save_callback_ = on_save;
    is_open_ = true;
    
    // Load current values
    radius_ = spawn->radius;
}

void SpawnPropertiesDialog::renderContent() {
    if (!current_spawn_) return;
    
    ImGui::Text("Spawn at: %d, %d, %d", spawn_position_.x, spawn_position_.y, spawn_position_.z);
    ImGui::Separator();
    
    // Spawn Radius
    ImGui::SliderInt("Radius", &radius_, 1, 10);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Spawn radius (1-10 tiles)");
    if (radius_ < 1) radius_ = 1;
    if (radius_ > 10) radius_ = 10;

    // Show area size
    int area_size = radius_ * 2 + 1;
    ImGui::Text("Area: %dx%d tiles", area_size, area_size);
    
    // Note: Creatures are stored per-tile now, not in spawn
    ImGui::Text("(Creatures are displayed on tiles)");
}

void SpawnPropertiesDialog::onSave() {
    if (current_spawn_) {
        current_spawn_->radius = radius_;
    }
}

void SpawnPropertiesDialog::onClose() {
    BasePropertiesDialog::onClose();
    current_spawn_ = nullptr;
}

} // namespace MapEditor::UI
