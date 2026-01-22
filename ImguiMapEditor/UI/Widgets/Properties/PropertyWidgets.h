#pragma once

#include <cstdint>
#include <string>

namespace MapEditor::UI::Properties {

/**
 * Outfit display data for podium panel.
 */
struct OutfitEdit {
    int look_type = 0;
    int look_head = 0;
    int look_body = 0;
    int look_legs = 0;
    int look_feet = 0;
    int look_addon = 0;
    int look_mount = 0;
    int mount_head = 0;
    int mount_body = 0;
    int mount_legs = 0;
    int mount_feet = 0;
};

/**
 * Reusable ImGui widget wrappers with input validation.
 * All methods return true if the value was modified.
 */
class PropertyWidgets {
public:
    // Common item properties
    static bool inputActionId(int& value);
    static bool inputUniqueId(int& value);
    static bool inputCount(int& value, int max_count);
    static bool inputTier(int& value);
    static bool inputDoorId(int& value);
    static bool inputCharges(int& value);
    
    // Position input (teleport destination)
    static bool inputPosition(int& x, int& y, int& z, int max_x = 65535, int max_y = 65535);
    
    // Direction dropdown (N/E/S/W)
    static bool inputDirection(int& direction);
    
    // Fluid type dropdown
    static bool inputFluidType(int& fluid_type);
    
    // Outfit editor for podium
    static bool inputOutfit(OutfitEdit& outfit);
    
    // Multiline text input
    static bool inputText(char* buffer, size_t buffer_size);
    
    // Spawn properties
    static bool inputSpawnRadius(int& radius, int max_radius = 30);
    static bool inputSpawnTime(int& seconds);
    
    // Depot ID (town link)
    static bool inputDepotId(int& depot_id);
};

} // namespace MapEditor::UI::Properties
