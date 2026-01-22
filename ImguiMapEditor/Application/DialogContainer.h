#pragma once
#include "UI/Dialogs/UnsavedChangesModal.h"
#include "UI/Dialogs/Import/ImportMapDialog.h"
#include "UI/Dialogs/Import/ImportMonstersDialog.h"
#include "UI/PreferencesDialog.h"
#include "UI/Dialogs/EditTownsDialog.h"
#include "UI/Dialogs/Properties/MapPropertiesDialog.h"
#include "UI/Dialogs/ConfirmationDialog.h"
#include "Presentation/Dialogs/ImportMapController.h"
#include "Presentation/Dialogs/CleanupController.h"
#include "Presentation/Dialogs/TownPickController.h"
namespace MapEditor {

/**
 * Container for all dialog components and controllers.
 * Groups file menu dialogs, map menu dialogs, confirmation dialogs, and controllers.
 */
struct DialogContainer {
    // File menu dialogs
    UI::UnsavedChangesModal unsaved_changes;
    UI::ImportMapDialog import_map;
    UI::ImportMonstersDialog import_monsters;
    UI::PreferencesDialog preferences;
    
    // Map menu dialogs
    UI::EditTownsDialog edit_towns;
    UI::MapPropertiesDialog map_properties;
    UI::ConfirmationDialog cleanup_confirm;
    
    // Dialog controllers (Phase 3)
    Presentation::ImportMapController import_controller;
    Presentation::CleanupController cleanup_controller;
    Presentation::TownPickController town_pick_controller;
};

} // namespace MapEditor
