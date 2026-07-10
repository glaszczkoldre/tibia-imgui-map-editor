# RAW Brush RME Parity Report

This document compares RAW brush behavior between `RME_Readonly` (wxWidgets reference editor) and `ImguiMapEditor` (Dear ImGui C++ port).

---

## 1. Item Placement & Stack-Order Parity

| Behavior | RME Reference | ImGui Editor (Current) | Parity Status | Notes |
|:---|:---|:---|:---|:---|
| **Ground Placement** | Replaces ground slot (`tile->ground`) | Replaces ground slot (`tile->ground_`) | 🟢 Full Parity | Handled in `Tile::addItem` |
| **Normal Item Placement** | Appended to `tile->items` | Appended to `tile->items_` | 🟢 Full Parity | Appended to maintain insertion order |
| **Always-On-Bottom Items** | Sorted by `top_order` before normal items | Sorted by `top_order` before normal items | 🟢 Full Parity | Handled in `Tile::addItem` |
| **Hook South/East Items** | Triggers visual flags, no custom stacking | Triggers visual flags, no custom stacking | 🟢 Full Parity | Flags checked dynamically in ImGui |
| **SimOne Replace Mode** | Replaces border items (`top_order == 2`) when setting `RAW_LIKE_SIMONE` is active and Alt is NOT pressed | Stacks them regardless | 🔴 Gap | To be implemented in `RawBrush::draw` |

---

## 2. RAW Palette Behavior

In both editors, selecting an item from the RAW palette/tileset registers a temporary `RawBrush` for the corresponding server item ID. The ImGui editor's `BrushRegistry::getOrCreateRAWBrush(itemId)` performs this exactly as RME does.

---

## 3. Gaps & Risks

### Gap A: RAW_LIKE_SIMONE Border Replacement
* **Description**: RME raw brush checks `Config::RAW_LIKE_SIMONE`. When active, placing a border item (AlwaysOnBottom, top order 2) deletes all other top order 2 items on the tile first, preventing messy border stacking.
* **Risk**: High risk of visual clutter / multiple stacked gravel or grass borders when raw-placing without automagic.
* **Solution**: Add `rawLikeSimone` boolean setting to `BrushSettingsService`, toggle in Tool Options panel, and enforce replacement in `RawBrush::draw()` when Alt is not held.

### Gap B: Erasing Ground Items in RAW Mode
* **Description**: RME `RAWBrush::undraw` clears `tile->ground` if its ID matches the brush ID. Currently, the ImGui editor's `RawBrush::undraw` only clears stacked items via `tile.removeItemsIf`, leaving ground items intact.
* **Risk**: Unable to erase raw-painted ground tiles using raw brush eraser mode.
* **Solution**: Update `RawBrush::undraw` to call `tile->removeGround()` if ground ID matches.

### Gap C: Smart Pick Fallback to RAW Brush
* **Description**: RME `Ctrl+Alt+Click` smart picking falls back to selecting the RAW brush of the top item if no specialized domain brush exists. ImGui editor returns `std::nullopt` in Smart mode if no domain brush matches.
* **Risk**: Cannot pick raw items (e.g. chests, containers, quest items) using the Smart Pick mouse shortcut.
* **Solution**: Add a fallback to `selectRawBrush` at the end of `BrushController::resolveBrushFromTile` for Smart mode.

---

## 4. Tests Needed

To ensure complete regression protection, the following cases must be verified in `Tests/BrushSmoke.cpp`:
1. **Ground-like RAW placement**: Placing ground item overwrites ground.
2. **Normal item RAW placement**: Placing normal item appends it.
3. **Always-on-bottom RAW placement**: Placing multiple bottom items (e.g. borders, gravel) sorts them by `top_order` and keeps them at the bottom.
4. **Hook South/East item placement**: Placement correctly registers hook properties on the tile.
5. **SimOne Replace vs Stacking**:
   - With `rawLikeSimone = true`, placing order 2 item replaces order 2 items.
   - With `rawLikeSimone = true` + Alt modifier, placing order 2 item stacks it.
6. **RAW Erasing**: Erasing ground and stacked items using RawBrush eraser mode.
7. **Smart Pick Fallback**: Ctrl+Alt+Clicking a raw item successfully selects its raw brush.
