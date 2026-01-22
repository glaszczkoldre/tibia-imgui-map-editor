#include "PropertyWidgets.h"
#include <imgui.h>

namespace MapEditor::UI::Properties {

bool PropertyWidgets::inputActionId(int& value) {
    bool changed = ImGui::InputInt("Action ID", &value);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Script identifier (100-65535, 0=none)");
    }
    if (value < 0) value = 0;
    if (value > 65535) value = 65535;
    return changed;
}

bool PropertyWidgets::inputUniqueId(int& value) {
    bool changed = ImGui::InputInt("Unique ID", &value);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Global map identifier (1000-65535, 0=none)");
    }
    if (value < 0) value = 0;
    if (value > 65535) value = 65535;
    return changed;
}

bool PropertyWidgets::inputCount(int& value, int max_count) {
    bool changed = ImGui::InputInt("Count", &value);
    if (value < 1) value = 1;
    if (value > max_count) value = max_count;
    return changed;
}

bool PropertyWidgets::inputTier(int& value) {
    bool changed = ImGui::InputInt("Tier", &value);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Item tier (0-255, OTBM v4+)");
    }
    if (value < 0) value = 0;
    if (value > 255) value = 255;
    return changed;
}

bool PropertyWidgets::inputDoorId(int& value) {
    bool changed = ImGui::InputInt("Door ID", &value);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("House door identifier (0-255)");
    }
    if (value < 0) value = 0;
    if (value > 255) value = 255;
    return changed;
}

bool PropertyWidgets::inputCharges(int& value) {
    bool changed = ImGui::InputInt("Charges", &value);
    if (value < 0) value = 0;
    if (value > 255) value = 255;
    return changed;
}

bool PropertyWidgets::inputPosition(int& x, int& y, int& z, int max_x, int max_y) {
    bool changed = false;
    ImGui::Text("Destination:");
    
    // Use InputScalar for cleaner look without +/- buttons on every field
    ImGui::PushItemWidth(80);
    ImGui::Text("X:"); ImGui::SameLine();
    changed |= ImGui::InputInt("##posX", &x, 0, 0); // step=0 hides +/- buttons
    ImGui::SameLine();
    ImGui::Text("Y:"); ImGui::SameLine();
    changed |= ImGui::InputInt("##posY", &y, 0, 0);
    ImGui::SameLine();
    ImGui::Text("Z:"); ImGui::SameLine();
    ImGui::PushItemWidth(50);
    changed |= ImGui::InputInt("##posZ", &z, 0, 0);
    ImGui::PopItemWidth();
    ImGui::PopItemWidth();
    
    if (x < 0) x = 0; if (x > max_x) x = max_x;
    if (y < 0) y = 0; if (y > max_y) y = max_y;
    if (z < 0) z = 0; if (z > 15) z = 15;
    return changed;
}

bool PropertyWidgets::inputDirection(int& direction) {
    const char* dirs[] = {"North", "East", "South", "West"};
    bool changed = ImGui::Combo("Direction", &direction, dirs, 4);
    return changed;
}

bool PropertyWidgets::inputFluidType(int& fluid_type) {
    // Fluid types matching Tibia's fluid system
    const char* fluids[] = {
        "Empty",        // 0
        "Water",        // 1
        "Blood",        // 2
        "Beer",         // 3
        "Slime",        // 4
        "Lemonade",     // 5
        "Milk",         // 6
        "Mana",         // 7
        "Life",         // 8 (health)
        "Oil",          // 9
        "Urine",        // 10
        "Coconut Milk", // 11
        "Wine",         // 12
        "Mud",          // 13
        "Fruit Juice",  // 14
        "Lava",         // 15
        "Rum",          // 16
        "Swamp"         // 17
    };
    bool changed = ImGui::Combo("Fluid Type", &fluid_type, fluids, 18);
    return changed;
}

bool PropertyWidgets::inputOutfit(OutfitEdit& o) {
    bool changed = false;
    
    changed |= ImGui::InputInt("Look Type", &o.look_type);
    if (o.look_type < 0) o.look_type = 0;
    
    ImGui::Text("Colors:");
    ImGui::PushItemWidth(55);
    changed |= ImGui::InputInt("Head##out", &o.look_head); ImGui::SameLine();
    changed |= ImGui::InputInt("Body##out", &o.look_body); ImGui::SameLine();
    changed |= ImGui::InputInt("Legs##out", &o.look_legs); ImGui::SameLine();
    changed |= ImGui::InputInt("Feet##out", &o.look_feet);
    ImGui::PopItemWidth();
    
    // Clamp color values 0-132
    auto clampColor = [](int& c) { if (c < 0) c = 0; if (c > 132) c = 132; };
    clampColor(o.look_head);
    clampColor(o.look_body);
    clampColor(o.look_legs);
    clampColor(o.look_feet);
    
    changed |= ImGui::InputInt("Addon", &o.look_addon);
    if (o.look_addon < 0) o.look_addon = 0;
    if (o.look_addon > 3) o.look_addon = 3;
    
    ImGui::Separator();
    ImGui::Text("Mount:");
    changed |= ImGui::InputInt("Mount Type", &o.look_mount);
    if (o.look_mount < 0) o.look_mount = 0;
    
    if (o.look_mount > 0) {
        ImGui::PushItemWidth(55);
        changed |= ImGui::InputInt("Head##mnt", &o.mount_head); ImGui::SameLine();
        changed |= ImGui::InputInt("Body##mnt", &o.mount_body); ImGui::SameLine();
        changed |= ImGui::InputInt("Legs##mnt", &o.mount_legs); ImGui::SameLine();
        changed |= ImGui::InputInt("Feet##mnt", &o.mount_feet);
        ImGui::PopItemWidth();
        
        clampColor(o.mount_head);
        clampColor(o.mount_body);
        clampColor(o.mount_legs);
        clampColor(o.mount_feet);
    }
    
    return changed;
}

bool PropertyWidgets::inputText(char* buffer, size_t buffer_size) {
    // Use horizontal scrollbar disabled for text wrapping behavior
    return ImGui::InputTextMultiline("##text", buffer, buffer_size,
                                     ImVec2(-1, 100),
                                     ImGuiInputTextFlags_AllowTabInput);
}

bool PropertyWidgets::inputSpawnRadius(int& radius, int max_radius) {
    bool changed = ImGui::SliderInt("Radius", &radius, 1, max_radius);
    return changed;
}

bool PropertyWidgets::inputSpawnTime(int& seconds) {
    bool changed = ImGui::InputInt("Spawn Time (s)", &seconds);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Respawn time in seconds (10-86400)");
    }
    if (seconds < 10) seconds = 10;
    if (seconds > 86400) seconds = 86400;
    return changed;
}

bool PropertyWidgets::inputDepotId(int& depot_id) {
    bool changed = ImGui::InputInt("Depot ID", &depot_id);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Town depot identifier");
    }
    if (depot_id < 0) depot_id = 0;
    return changed;
}

} // namespace MapEditor::UI::Properties
