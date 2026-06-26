# Parity Comparison Report: Wall Hangings & Hook Detection

This report compares how **Remere's Map Editor (RME)** and the **ImguiMapEditor** handle wall hangings (such as tapestries, paintings, blackboards) and wall hooks (South-facing and East-facing).

---

## 1. How RME Handles Hook Detection & Hangables

In RME, the alignment of hangable items to walls is determined using tile state flags and the OTB item definitions:

### A. Tile Hook Flags (`TILESTATE_HOOK_SOUTH` & `TILESTATE_HOOK_EAST`)
- When any item on a tile is modified or loaded, RME runs a tile update loop (`TileOperations::update` in `tile_operations.cpp`).
- It iterates through all items on the tile and reads their OTB definitions (`i->getDefinition()`).
- If an item's OTB definition has the `HookSouth` flag, the tile's `statflags` is updated to include `TILESTATE_HOOK_SOUTH`.
- If it has `HookEast`, the tile is marked with `TILESTATE_HOOK_EAST`.
- Thus, hook presence is cached directly on the tile state.

### B. Hangable Pattern Selection
- During rendering, the pattern coordinates are computed in `PatternCalculator::Calculate` (`rendering/utilities/pattern_calculator.h`).
- If an item's OTB definition has `ItemFlag::IsHangable`:
  - If `tile->hasHookSouth()` is true (reads `TILESTATE_HOOK_SOUTH`), it sets `patterns.x = 1` (South-aligned sprite).
  - Else if `tile->hasHookEast()` is true (reads `TILESTATE_HOOK_EAST`), it sets `patterns.x = 2` (East-aligned sprite).
  - Else, it defaults to `patterns.x = 0` (freestanding sprite).
- This applies to both **placed items** and **ghost preview overlays**.

---

## 2. How ImguiMapEditor Currently Handles Hooks (and Where it Fails)

In `ImguiMapEditor`, hook flags are computed dynamically, but a combination of domain-level limitations and lookup bugs causes hangables to fail:

### A. The Domain Null-Type Bug (Preview/History/Brushes)
- `Domain::Tile::hasHookSouth()` and `hasHookEast()` query `ground_->getType()` and `item->getType()`.
- However, loaded map tiles do not store populated `ItemType*` pointers inside the domain objects, and new items created during brush strokes/history restoration are initialized with just a server ID.
- Since the `Domain` layer cannot reference `ClientDataService` (violating layer dependencies), these `getType()` calls evaluate to `nullptr`.
- In `PreviewOverlay.cpp`, hook detection only checked `tileItem->getType()`, so hook detection **always returned false during preview**.

### B. Ground Hook Lookup Bug
- In `TileRenderer::drawTile`, the hook status is calculated dynamically.
- However, for the ground item, it checked:
  ```cpp
  if (const auto *type = ground->getType()) { // Always null
      tile_has_hook_south |= type->hook_south;
  }
  ```
- It failed to fall back to `client_data_->getItemTypeByServerId` for the ground item (unlike normal items).

### C. OTB Property Overwriting Bug
- In `ClientDataService::mergeOtbWithDat`, the OTB flags are merged with DAT properties. However, it overwrote OTB's hook flags entirely with DAT properties:
  ```cpp
  merged.is_hangable = dat->is_hangable;
  merged.hook_east = dat->is_horizontal;
  merged.hook_south = dat->is_vertical;
  ```
- If a wall item had correct hook flags in `items.otb` but was missing `is_horizontal`/`is_vertical` flags in `tibia.dat`, the hook properties were destroyed.

### D. Secondary Client Swapped Flags
- In `SecondaryClientData.cpp`, the hook flag assignments were swapped:
  ```cpp
  otb_item.hook_south = dat_item->is_horizontal; // Swapped
  otb_item.hook_east = dat_item->is_vertical;     // Swapped
  ```

---

## 3. Proposed Parity Solution

To achieve complete parity and fix the tapestry/hangable rendering, we will make the following corrections:

1. **Logical OR in `ClientDataService`**: Merge OTB and DAT flags instead of overwriting, ensuring correct properties from both definitions are used.
2. **Correct Secondary Client Hooks**: Swap `hook_south` / `hook_east` assignments in `SecondaryClientData.cpp` to match primary client definitions.
3. **Resolve Types in `PreviewOverlay`**: Fall back to `clientData->getItemTypeByServerId` when querying tile item types in the preview overlay.
4. **Resolve Ground Type in `TileRenderer`**: Fall back to `client_data_` when checking ground hooks in the main tile renderer.
