#pragma once

#include <cstdint>

namespace MapEditor::Domain::Search {

/**
 * Type filter for advanced search - multi-select (OR logic).
 * Any item matching at least one selected type passes.
 */
struct TypeFilter {
    bool depot = false;
    bool mailbox = false;
    bool trash_holder = false;
    bool container = false;
    bool door = false;
    bool magic_field = false;
    bool teleport = false;
    bool bed = false;
    bool key = false;
    bool podium = false;
    
    // Combat types
    bool weapon = false;
    bool ammo = false;
    bool armor = false;
    bool rune = false;
    
    bool creature = false;
    
    bool hasAnySelected() const {
        return depot || mailbox || trash_holder || container || door ||
               magic_field || teleport || bed || key || podium || creature ||
               weapon || ammo || armor || rune;
    }
    
    void clear() {
        depot = mailbox = trash_holder = container = door = false;
        magic_field = teleport = bed = key = podium = creature = false;
        weapon = ammo = armor = rune = false;
    }
};

/**
 * Property filter for advanced search - multi-select (AND logic).
 * Item must have ALL selected properties to pass.
 */
struct PropertyFilter {
    // Movement/Blocking
    bool unpassable = false;
    bool unmovable = false;
    bool block_missiles = false;
    bool block_pathfinder = false;
    bool floor_change = false;
    
    // Interaction
    bool readable = false;
    bool writeable = false;
    bool pickupable = false;
    bool force_use = false;      // Right-click usage
    bool allow_dist_read = false; // Readable from distance
    
    // Storage
    bool stackable = false;
    bool has_charges = false;
    bool client_charges = false; // Shows charges in client
    
    // Placement
    bool rotatable = false;
    bool hangable = false;
    bool hook_east = false;
    bool hook_south = false;
    bool has_elevation = false;
    bool full_tile = false;      // Full ground tile
    bool always_on_top = false;
    
    // Visuals/Misc
    bool ignore_look = false;
    bool has_light = false;
    bool animation = false;      // Is always animated
    bool has_speed = false;
    bool decays = false;         // Has decay properties
    
    bool hasAnySelected() const {
        return unpassable || unmovable || block_missiles || block_pathfinder ||
               readable || writeable || pickupable || stackable || rotatable ||
               hangable || hook_east || hook_south || has_elevation || 
               ignore_look || floor_change || has_light || has_charges || 
               decays || has_speed || force_use || allow_dist_read ||
               full_tile || client_charges || animation || always_on_top;
    }
    
    void clear() {
        unpassable = unmovable = block_missiles = block_pathfinder = false;
        readable = writeable = pickupable = stackable = rotatable = false;
        hangable = hook_east = hook_south = has_elevation = false;
        ignore_look = floor_change = false;
        has_light = has_charges = decays = has_speed = false;
        force_use = allow_dist_read = full_tile = client_charges = false;
        animation = always_on_top = false;
    }
};

} // namespace MapEditor::Domain::Search
