# Right-Click Context Menu RME Parity Report

This document compares Remere's Map Editor (RME) context menu implementation against the ImGui Map Editor and explains the root causes of the broken context menus.

---

## 1. Core Differences and Root Causes

| Feature | Remere's Map Editor (RME) | ImGui Map Editor (Before Fix) | Impact / Root Cause |
| :--- | :--- | :--- | :--- |
| **Selection Paradigm** | Tile-based selection. Right-clicking a tile automatically selects it (`editor.selection.select(tile)`). | Pixel-perfect, entity-based selection. Right-clicking on transparent area on a tile leaves the selection empty. | **Broken Context Menu on Empty Selection**: ImGui context menu was strictly gated behind `has_single_selection_context_` (`selected_positions.size() == 1`). If the selection was empty, the menu hid all tile, item, and brush options. |
| **Item Type Resolution** | Items have their type information resolved directly. | Map-loaded items do not have `type_` set on construction; `item->getType()` returns `nullptr`. | **Missing Properties & Actions**: The menu queried `item->getType()` for door and rotation flags. Since it returned `nullptr`, door controls, rotation, and copy name options were completely disabled or missing. |
| **Brush Picking Target** | Evaluates all items on the tile to offer brush picking (e.g., checks if *any* item is a wall to offer "Select Wallbrush"). | Evaluates only the single selected item (or topmost item) via `preferredItem` passed to `resolveBrushFromTile`. | **Brush Picking Invisibility**: Because selection was empty or item types were missing, brush options could not be resolved or displayed. |

---

## 2. RME Context Menu Logic (`map_popup_menu.cpp`)

RME's context menu operates as follows:
1. **Clipboard Actions**: Cut, Copy, Copy Position, Paste, and Delete are enabled if `anything_selected` is true.
2. **Single Tile Selection**: If `editor.selection.size() == 1`, RME retrieves the selected tile and examines its items.
3. **Item Identification**:
   - `topSelectedItem` is the selected item (if only one item is selected).
   - `topItem` is the selected item, falling back to `tile->ground.get()`.
4. **Action Presentation**:
   - Offers door switches and rotations based on `topSelectedItem`'s properties.
   - Offers brush selections (Wall, Carpet, Table, Doodad, Door, Ground, Creature, Spawn, House) based on either the items on the tile or the ground brush.
   - Offers "Properties" if any item/spawn/creature exists on the tile.
   - Offers "Browse Field" and "Tile Properties" for the selected tile.

---

## 3. ImGui Map Editor Context Menu Logic (`MapContextMenu.cpp`)

In the ImGui editor:
1. `MapContextMenu::render` queries selection:
   ```cpp
   has_single_selection_context_ =
       selected_positions.size() == 1 && selected_positions.front() == position_;
   ```
2. If `has_single_selection_context_` is false, it returns early from `renderTileActions`, `renderItemActions`, and `renderBrushSelectionActions`.
3. `renderItemActions` checks door state/rotation using:
   ```cpp
   const auto *selected_type = selected_item ? selected_item->getType() : nullptr;
   ```
   Since `selected_item->getType()` is null for map-loaded items, all conditions like `selected_type->isDoor()` or `selected_type->rotateTo != 0` evaluate to false.

---

## 4. Proposed Fixes for ImGui Map Editor

To achieve full parity with RME:
1. **Relax Context Gate**: Update `has_single_selection_context_` to be true if selection is empty *or* exactly one position matches the clicked coordinates.
2. **Fall Back to Active Item**: When selection is empty, let `selected_item` fall back to the active item (topmost item or ground) on the clicked tile.
3. **Dynamic Type Resolution**: Resolve item types dynamically using `ClientDataService` (retrieved from `session->getDocument()->getClientData()`) if `item->getType()` is null.
4. **Correct Action Triggers**: Ensure rotation, door toggling, name copying, and properties opening are triggered for this resolved active item.
