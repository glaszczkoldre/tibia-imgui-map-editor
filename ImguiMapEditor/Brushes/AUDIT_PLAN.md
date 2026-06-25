# Remaining Issues — Architecture-Aligned Action Plan

> Previous phases (dead code, noexcept, includes, magic numbers, GLFW layering, dedup,
> `map.markChanged()` consistency, preview/styling fixes) are **complete and LGTM**.
>
> This plan covers the initial 7 remaining items, plus new findings (Items 8–15) from the June 2026 comprehensive audit of Brushes, Preview Services, Autoborder, and settings.


---

## Architecture Rules Summary (from AGENTS.md)

| Rule | Implication |
|------|-------------|
| **No global state** | `thread_local g_altGroundReplaceState` must be eliminated |
| **Each class owns one concept** | `WallDecorationBrush` needs its own `.cpp` |
| **File > 500 lines → split** | `BrushController.cpp` (2083), `WallBrush.cpp` (1077), `GroundBrush.cpp` (870) |
| **Function > 150 lines → refactor** | `WallBrush::buildPreviewTiles` (195), `GroundBrush::updateBorderItems` (~400) |
| **No duplicate code** | `DrawContext` construction repeated 5+ times across brushes |
| **Pass dependencies via constructor injection** | Alt-replace state → inject through `DrawContext` |
| **Dependencies flow downwards** | Services → Domain, not Domain → Services |
| **No `const_cast`** | 8 preview providers use `const_cast` to mutate from `const` methods |

---

## Item 1 — `BrushEnums.cpp` parse efficiency

**Priority**: Low | **Effort**: 30 min | **Risk**: Zero

### Problem
4 `static unordered_map<string_view, T>` created on every function call, doing heap allocation for 7-18 element lookups.

### Solution
Replace each `static unordered_map` with a `static constexpr array<pair<string_view, T>, N>` sorted at compile time, then `std::lower_bound` at runtime. Also unify fallback behavior.

### Files
- `Brushes/Enums/BrushEnums.cpp` — rewrite `parseEdgeName`, `parseTableAlign`, `parseWallType`, `parseDoorType`
- `Brushes/Enums/BrushEnums.h` — no changes needed

### Steps
1. Add `#include <algorithm>` and `#include <array>` to `.cpp`, remove `#include <unordered_map>`
2. Replace each map with `static constexpr std::array<std::pair<std::string_view, T>, N> kMap = {{...}}` sorted alphabetically
3. Replace `map.find(name)` with `std::lower_bound(kMap.begin(), kMap.end(), name, ...)`
4. Add `spdlog::warn` for unknown values before returning sentinel
5. Change `doorTypeToString` default from `""` → `"undefined"`

### Verification
- `build_ninja.bat` compiles + passes
- All parse/toString round-trips are identity:
  - `parseEdgeName(edgeTypeToString(x)) == x` for all `EdgeType` values
  - Same for `TableAlign`, `WallAlign`, `DoorType`

---

## Item 2 — `WallDecorationBrush` own `.cpp`

**Priority**: Medium | **Effort**: 1 hr | **Risk**: Medium

### Problem
`WallDecorationBrush::draw()` and `WallDecorationBrush::undraw()` are implemented in `WallBrush.cpp` (lines 1023-1075) but the class has its own header. Violates "one class per file."

These methods call `resolveWallBrushForItem` — a free function in `WallBrush.cpp`'s anonymous namespace, invisible to other TUs.

### Solution
1. Promote `resolveWallBrushForItem` to a `static` public method on `WallBrush`
2. Create `Types/WallDecorationBrush.cpp`
3. Move `draw()` and `undraw()` implementations to the new file
4. Replace `brushRegistry()` calls (previously removed) — verify `getBrushRegistry()` is used

### Files
| Action | File |
|--------|------|
| Modify | `Types/WallBrush.h` — add `static const WallBrush* resolveWallBrushForItem(const Item&, BrushRegistry&)` |
| Modify | `Types/WallBrush.cpp` — promote function from anon namespace, remove `draw`/`undraw` |
| Create | `Types/WallDecorationBrush.cpp` — move `draw()` and `undraw()` |
| Verify | CMake glob picks up new `.cpp` |

### Dependencies
- `WallBrush.h` already includes needed headers (`BrushRegistry.h` via forward decls, `WallNode.h`, etc.)
- `WallDecorationBrush.cpp` needs: `WallDecorationBrush.h`, `WallBrush.h`, `BrushUtils.h`, `Domain/Tile.h`, `Services/ClientDataService.h`

### Verification
- `build_ninja.bat` compiles
- Paint with WallDecorationBrush on a wall tile — decoration appears correctly
- Undraw removes only decoration, not the wall

---

## Item 3 — `const_cast` lazy-rebuild in 8 preview providers

**Priority**: Medium | **Effort**: 2 hrs | **Risk**: Low

### Problem
8 preview providers in `Services/Preview/` use this anti-pattern:
```cpp
const std::vector<PreviewTileData>& getTiles() const {
    const_cast<Provider*>(this)->build();  // violates const-correctness
    return tiles_;
}
```

Each provider has `mutable` members anyway. The `const_cast` is both unnecessary and masks the design intent.

### Solution
Replace with explicit `mutable` + `ensureBuilt()` pattern:
```cpp
// In header:
mutable bool dirty_ = true;  // already exists or add

// In .cpp:
const std::vector<PreviewTileData>& getTiles() const {
    if (dirty_) {
        tiles_.clear();
        bounds_ = {};
        buildPreview(*tiles_, *bounds_);  // populate mutable members
        dirty_ = false;
    }
    return tiles_;
}
```

Remove all `const_cast<Provider*>(this)` calls. Keep existing `mutable` members.

### Files (8 providers)
- `Services/Preview/RawBrushPreviewProvider.cpp`
- `Services/Preview/MultiItemBrushPreviewProvider.cpp`
- `Services/Preview/DoodadBrushPreviewProvider.cpp`
- `Services/Preview/CreaturePreviewProvider.cpp`
- `Services/Preview/ZoneBrushPreviewProvider.cpp`
- `Services/Preview/WallBrushPreviewProvider.cpp`
- `Services/Preview/PastePreviewProvider.cpp`
- `Services/Preview/SpawnPreviewProvider.cpp`

### Verification
- `build_ninja.bat` compiles
- Each brush type shows preview overlay when selected
- Preview updates correctly when brush size/variation changes

---

## Item 4 — Eliminate global `thread_local AltGroundReplaceState`

**Priority**: High | **Effort**: 2 hrs | **Risk**: Medium

### Problem
```cpp
// GroundBrush.cpp:28
thread_local AltGroundReplaceState g_altGroundReplaceState {};
```
This violates the **"No global state"** rule. It's per-thread-per-stroke state that tracks alt-replace mode across multiple `placeGroundTile` calls within a single `paintRecordedPositions` loop.

The challenge: `BrushController::createDrawContext()` creates a fresh `DrawContext` for each tile position, but the alt-replace state must persist across all positions in the same stroke.

### Solution
Move state ownership to `BrushController` and pass via `DrawContext*`:

1. Add `AltGroundReplaceState altReplaceState_` member to `BrushController` (private)
2. Add `AltGroundReplaceState* altReplace` field to `DrawContext`
3. In `BrushController::createDrawContext()`, set `ctx.altReplace = &altReplaceState_`
4. In `BrushController::beginStroke()`, reset `altReplaceState_ = {}`
5. In `GroundBrush::shouldSkipAltGroundPlacement()`, use `ctx.altReplace` instead of global
6. Remove `thread_local g_altGroundReplaceState`, remove `resetAltGroundReplaceState()` free function
7. Rename `GroundBrush::resetAltReplaceState()` → it becomes a no-op or is removed (caller resets via `beginStroke`)

### Files
| Action | File |
|--------|------|
| Modify | `Core/IBrush.h` — add `AltGroundReplaceState* altReplace = nullptr` to `DrawContext` |
| Modify | `BrushController.h` — add `AltGroundReplaceState altReplaceState_` member |
| Modify | `BrushController.cpp` — wire in `createDrawContext` and `beginStroke` |
| Modify | `Types/GroundBrush.cpp` — use `ctx.altReplace` instead of `g_altGroundReplaceState` |
| Modify | `Types/GroundBrush.h` — remove `resetAltReplaceState()` static method |

### Data flow (after fix)
```
BrushController::beginStroke()
  → altReplaceState_ = {}

BrushController::paintRecordedPositions()  [loop over tile positions]
  → ctx = createDrawContext(modifiers)
      ctx.altReplace = &altReplaceState_
  → brush->draw(map, tile, ctx)
      → shouldSkipAltGroundPlacement(registry, tile, ctx)
          reads/writes ctx.altReplace->active, ->emptyOnly, ->replaceBrush
```

### Verification
- `build_ninja.bat` compiles
- Alt+click first tile with GroundBrush → places ground on empty tile (emptyOnly mode)
- Alt+click on tile with different ground → replaces it (replaceBrush mode)
- Normal click (no Alt) → standard place, no alt-replace active
- Drag-paint with Alt held → alt-replace persists across stroke

---

## Item 5 — Deduplicate `DrawContext` construction

**Priority**: Low | **Effort**: 30 min | **Risk**: Zero

### Problem
`CarpetBrush::updateBorderItems` and `TableBrush::rebuildTile` construct identical `DrawContext` values 3-5 times each:
```cpp
DrawContext borderCtx;
borderCtx.clientData = registry_.getClientDataService();
borderCtx.brushRegistry = &registry_;
borderCtx.ownerBrushId = registry_.getBrushId(ownerBrush);
```

### Solution
Add a private inline helper to each brush (the pattern varies slightly — `ownerBrush` differs):
```cpp
// In CarpetBrush/TableBrush .cpp (anonymous namespace or private method)
DrawContext makeBrushContext(const BrushRegistry& registry, const IBrush* owner) {
    DrawContext ctx;
    ctx.clientData = registry.getClientDataService();
    ctx.brushRegistry = &registry;
    ctx.ownerBrushId = registry.getBrushId(owner);
    return ctx;
}
```
Replace 5 manual constructions with calls to this helper.

### Files
- `Types/CarpetBrush.cpp`
- `Types/TableBrush.cpp`

### Verification
- `build_ninja.bat` compiles
- CarpetBrush and TableBrush paint correctly with border alignment

---

## Item 6 — GroundBrush border struct cleanup

**Priority**: Low | **Effort**: 20 min | **Risk**: Zero

### Problem
Three local structs (`NeighborState`, `BorderCluster`, `ResolvedBorderRule`) are defined inside `updateBorderItems()`. The function is ~400 lines. Lifting them out reduces noise.

Also, `addEdgeItems` returns `bool` but its return value is only checked in `addEdgeWithFallback`, where the `false` path inside `updateBorderItems`'s direct call is dead.

### Solution
1. Move `NeighborState`, `BorderCluster`, `ResolvedBorderRule` from inside `updateBorderItems` → anonymous namespace at top of file
2. Change `addEdgeItems` return type from `bool` → `void`; remove the dead `if (!addEdgeItems(...)) return;` path

### Files
- `Types/GroundBrush.cpp`

### Verification
- `build_ninja.bat` compiles
- GroundBrush borders render identically

---

## Item 7 — File size refactors

**Priority**: High (per AGENTS.md limits) | **Effort**: 4 hrs | **Risk**: Medium

### Files exceeding 500-line limit

| File | Lines | Over by |
|------|-------|---------|
| `BrushController.cpp` | 2083 | 4× |
| `WallBrush.cpp` | 1077 | 2× |
| `GroundBrush.cpp` | 870 | 1.7× |

### Functions exceeding 150-line limit

| Function | Lines | File |
|----------|-------|------|
| `buildPreviewTiles` | ~195 | `WallBrush.cpp` |
| `updateBorderItems` | ~400 | `GroundBrush.cpp` |

### Split plan

#### 7a. Split `BrushController.cpp` → 4 files

| New File | Contains | Approx lines |
|----------|----------|--------------|
| `BrushController.cpp` | Core: `initialize`, `setBrush`, `clearBrush`, `createDrawContext`, `resolveBrushFromTile`, `resolveBrushSelection`, `captureCurrentSelection`, plumbing | ~400 |
| `BrushStroke.cpp` | `beginStroke`, `continueStroke`, `endStroke`, `getLinePositions`, `getPaintedStrokePositions`, `paintRecordedPosition*`, `eraseRecordedPosition`, `paintExpandedCenter`, `eraseExpandedCenter`, 5 `continue*LikeStroke` methods, `finalizeAutoborderStroke` | ~600 |
| `BrushDoorOps.cpp` | `activate*DoorBrush` (8 methods), `getDoorBrushForType`, `canSwitchDoorAt`, `switchDoorAt`, `getDoorOpenStateAt`, `canRotateItemAt`, `rotateItemAt` | ~350 |
| `BrushVariationOps.cpp` | `applyBrush`, `eraseBrush`, `refreshCurrentBrush`, `cycleBrushVariation`, `setBrushVariation`, `setBrushThickness`, `getBrushThickness`, `adjustBrushSize`, `storeBrushSlot`, `recallBrushSlot`, `usesPreciseMutationNotifications` | ~450 |

Internal helpers stay as methods on `BrushController`; new files `#include "BrushController.h"` and implement methods via `BrushController::methodName`.

#### 7b. Split `WallBrush.cpp` → 3 files

| New File | Contains | Approx lines |
|----------|----------|--------------|
| `WallBrush.cpp` | Core: constructor, `draw`, `undraw`, `ownsItem`, `addWallItem`, `addDoorItem`, `addRedirectName`, etc. | ~350 |
| `WallBrushPreview.cpp` | `buildPreviewTiles` + helpers (`resolveWallBrushForItem`, `resolveDoorItem`, `updateConsecutiveDecorations`) | ~250 |
| `WallDecorationBrush.cpp` | `WallDecorationBrush::draw`, `WallDecorationBrush::undraw` (from Item 2 above) | ~60 |

`resolveWallBrushForItem` becomes a `static` public method on `WallBrush` so both `WallBrush.cpp` and `WallBrushPreview.cpp` can use it.

#### 7c. Split `GroundBrush.cpp` → 2 files

| New File | Contains | Approx lines |
|----------|----------|--------------|
| `GroundBrush.cpp` | Core: constructor, `draw`, `undraw`, `ownsItem`, `addGroundItem`, friend/enemy, border rules, `placeGroundTile`, `eraseFromTile`, `rebuildAround`, `rebuildTile`, `selectWeightedItem`, `isBorderItem` | ~380 |
| `GroundBorderUpdater.cpp` | `updateBorderItems` + border lambdas + border helper functions | ~490 |

`updateBorderItems` stays as `GroundBrush::updateBorderItems` (declared in header), just implemented in the new file. The helper lambdas (`addEdgeItems`, `addEdgeWithFallback`, etc.) move too.

### Shared dependencies

The new `.cpp` files include the same headers as the originals. CMake glob picks up all `.cpp` files, so no build system changes needed.

### Verification
- `build_ninja.bat` compiles
- All brush types paint correctly
- Undo/redo works for complex strokes
- No new files exceed 500 lines

---

## Item 8 — `PositionHash` sign-extension bug

**Priority**: High | **Effort**: 15 min | **Risk**: Low

### Problem
In `BrushController.h:411`, coordinate elements `x` (`int32_t`), `y` (`int32_t`), and `z` (`int16_t`) are cast to `int64_t` and ORed. Casting negative coordinates to `int64_t` sign-extends them, filling upper bits with `1`s. This corrupts the other coordinate bits in the bitwise OR, causing severe hash collisions and key corruption when working at negative positions.

### Solution
Cast each coordinate to `uint64_t` and mask each element before ORing:
```cpp
struct PositionHash {
  size_t operator()(const std::tuple<int32_t, int32_t, int16_t> &p) const {
    const uint64_t x = static_cast<uint64_t>(std::get<0>(p)) & 0xFFFFF; // 20 bits
    const uint64_t y = static_cast<uint64_t>(std::get<1>(p)) & 0xFFFFF; // 20 bits
    const uint64_t z = static_cast<uint64_t>(std::get<2>(p)) & 0xFFFF;  // 16 bits
    return std::hash<uint64_t>()(x | (y << 20) | (z << 40));
  }
};
```

### Files
- `Brushes/BrushController.h`

---

## Item 9 — Thread-local / static mutable state violations

**Priority**: High | **Effort**: 1 hr | **Risk**: Low

### Problem
`WeightedSelection` (`WeightedSelection.h:50`) and `DoodadBrushPreviewProvider.cpp` contain `static thread_local` mutable RNG generators. This directly violates the project's rule against global/thread-local mutable state.

### Solution
- For `WeightedSelection`, pass `std::mt19937&` as a parameter to selection/seed methods, or inject it via constructor/service.
- For `DoodadBrushPreviewProvider.cpp`, replace the thread-local engine with a simple non-thread-local call to `std::random_device` or pass the RNG.

### Files
- `Brushes/Behaviors/WeightedSelection.h` / `WeightedSelection.cpp`
- `Services/Preview/DoodadBrushPreviewProvider.cpp`

---

## Item 10 — Layering violations

**Priority**: Medium | **Effort**: 1.5 hr | **Risk**: Low

### Problem
- `BrushSystem.h` includes ImGui UI widgets (`UI::TilesetWidget`, `UI::Panels::BrushSizePanel`) directly, coupling core brushes to UI panels.
- Lookup service headers (`BorderLookupService.h`, etc.) use relative includes like `../../Brushes/Enums/BrushEnums.h`.

### Solution
- Forward-declare UI widgets in `BrushSystem.h` or decouple widgets using callbacks/interfaces.
- Convert relative includes in lookup services to root-relative paths (e.g. `#include "Brushes/Enums/BrushEnums.h"`).

### Files
- `Brushes/BrushSystem.h`
- `Services/Brushes/BorderLookupService.h`
- `Services/Brushes/CarpetLookupService.h`
- `Services/Brushes/TableLookupService.h`
- `Services/Brushes/WallLookupService.h`

---

## Item 11 — Overly complex functions & file size violations

**Priority**: High | **Effort**: 3 hr | **Risk**: Medium

### Problem
- `BrushController::resolveBrushFromTile` is 570 lines (limit 150 lines).
- `GroundBrush::updateBorderItems` is 398 lines (limit 150 lines).
- `Services/BrushSettingsService.cpp` is 535 lines (limit 500 lines).

### Solution
- Extract nested lambdas in `resolveBrushFromTile` and `updateBorderItems` into private helper functions.
- Split `BrushSettingsService.cpp` by moving `CustomBrushShape` definition and file serialization/persistence helper classes into dedicated files (`Services/CustomBrushShape.h`/`.cpp` and `Services/BrushSettingsSerializer.h`/`.cpp`).

### Files
- `Brushes/BrushController.cpp`
- `Brushes/Types/GroundBorderUpdater.cpp`
- `Services/BrushSettingsService.h` / `.cpp`
- [NEW] `Services/CustomBrushShape.h` / `.cpp`
- [NEW] `Services/BrushSettingsSerializer.h` / `.cpp`

---

## Item 12 — Const correctness & observer leaks

**Priority**: Medium | **Effort**: 2 hr | **Risk**: Low

### Problem
- `resolveDoorTarget` takes `const Tile*` but returns mutable `Item*`, leaking mutable access to tile objects.
- Preview providers and factory pass `BrushSettingsService*` as mutable pointer when they only read config.
- `DragPreviewProvider` accepts mutable `ChunkedMap*` but only queries read-only tiles.
- `resolveBrushFromTile` and `canSelectBrushFromTile` do not mutate `BrushController` state and should be `const`.
- `IPreviewProvider` declares an unused virtual method `needsRegeneration()`.

### Solution
- Overload `resolveDoorTarget` for const/non-const variants.
- Update preview providers/factory to accept `const BrushSettingsService*`.
- Update `DragPreviewProvider` to accept `const ChunkedMap*`.
- Mark `resolveBrushFromTile` and `canSelectBrushFromTile` `const`.
- Remove `needsRegeneration()` from `IPreviewProvider` and its implementation classes.

### Files
- `Brushes/Helpers/DoorResolveUtils.h`
- `Brushes/BrushController.h` / `.cpp`
- `Services/Preview/IPreviewProvider.h`
- All 8+ Preview Providers and `BrushPreviewFactory`

---

## Item 13 — Duplicate code / DRY violations

**Priority**: Medium | **Effort**: 1.5 hr | **Risk**: Low

### Problem
- Exact duplication of wall alignment and door resolving logic between `WallBrush::rebuildTile` and `WallBrushPreview::buildPreviewTiles`.
- Duplication between `resolveMutableTileItem` and `resolveTileItem` in `DoorResolveUtils.h`.

### Solution
- Extract wall resolution logic to a shared private helper method on `WallBrush` (`resolveWallItem`).
- Deduplicate `resolveMutableTileItem` by calling `resolveTileItem` with const_cast.

### Files
- `Brushes/Types/WallBrush.h` / `.cpp`
- `Brushes/Types/WallBrushPreview.cpp`
- `Brushes/Helpers/DoorResolveUtils.h`

---

## Item 14 — Missing `map.markChanged()` calls in brushes

**Priority**: High | **Effort**: 1.5 hr | **Risk**: Low

### Problem
- `RawBrush`, `TableBrush`, `WallBrush`, and `WallDecorationBrush` modify tiles but omit calling `map.markChanged()`, causing rendering cache invalidation to fail.
- `OptionalBorderBrush::undraw` does not rebuild surrounding borders.
- `WaypointBrush` missing `isDraggable() const override { return false; }`, causing drag operations to corrupt waypoint names.

### Solution
- Call `map.markChanged()` in all mutation methods of these brushes.
- Update `OptionalBorderBrush::undraw` to trigger border rebuild on neighboring tiles.
- Override `isDraggable()` to return `false` in `WaypointBrush`.

### Files
- `Brushes/Types/RawBrush.cpp`
- `Brushes/Types/TableBrush.cpp`
- `Brushes/Types/WallBrush.cpp`
- `Brushes/Types/WallDecorationBrush.cpp`
- `Brushes/Types/OptionalBorderBrush.cpp`
- `Brushes/Types/WaypointBrush.h`

---

## Item 15 — Performance bottlenecks in Autoborder & Diffs

**Priority**: High | **Effort**: 2.5 hr | **Risk**: Medium

### Problem
- `applyTileDiffs` takes `TileDiffList` by const reference and performs deep clones of tiles (`after->clone()`), even though the original tiles are immediately discarded.
- `sameTileState` does expensive binary serialization (`TileSnapshot::capture`) to compare tile equality on every frame during brush drag.
- Lookup tables in lookup services (`BorderLookupService`, etc.) are initialized dynamically at runtime.

### Solution
- Change `applyTileDiffs` to accept by value or rvalue-reference and `std::move` the tiles, avoiding deep-cloning.
- Implement a lightweight equality operator/comparison method on `Domain::Tile` that compares fields directly.
- Modify Python code generator to output tables as static `constexpr std::array`.

### Files
- `Services/Autoborder/TileDiff.h` / `.cpp`
- `Services/Autoborder/AutoborderEngine.cpp`
- `Domain/Tile.h` / `.cpp`
- `Services/Brushes/*LookupTable.inc` / `BorderLookupService.cpp` / Carpet / Table / Wall

---

## Execution Order

```
Items 1, 5, 6, 8, 10      — Independent, low risk
    ↓
Items 12, 13, 14          — Const-correctness, DRY, and markChanged fixes
    ↓
Item 9 (global state)     — Random number generator updates
Item 4 (alt-replace)      — Alt-replace global state refactor
    ↓
Item 2 (WallDeco)         — Wall decoration separate file
Item 3 (const_cast)       — Preview provider mutable updates
    ↓
Item 15 (Performance)     — Autoborder diff & serialization optimizations
    ↓
Items 7, 11 (Splits)      — Large file splits & lambda extractions
```

---

## Summary

| Item | Description | Effort | Risk |
|------|-------------|--------|------|
| 1 | Parse efficiency (sorted array) | 30 min | Zero |
| 2 | WallDecorationBrush own .cpp | 1 hr | Medium |
| 3 | const_cast → mutable pattern | 2 hr | Low |
| 4 | Eliminate global alt-replace state | 2 hr | Medium |
| 5 | Deduplicate DrawContext construction | 30 min | Zero |
| 6 | Border struct cleanup | 20 min | Zero |
| 7 | File size refactors (7 new files) | 4 hr | Medium |
| 8 | PositionHash sign-extension bug | 15 min | Low |
| 9 | Thread-local / static RNG violations | 1 hr | Low |
| 10 | Layering & relative include violations | 1.5 hr | Low |
| 11 | Overly complex functions & settings splits | 3 hr | Medium |
| 12 | Const correctness & observer leaks | 2 hr | Low |
| 13 | Duplicate wall & door resolution logic | 1.5 hr | Low |
| 14 | Missing map.markChanged() & waypoint drag | 1.5 hr | Low |
| 15 | Performance bottlenecks (diffs & equality) | 2.5 hr | Medium |
| | **Total** | **~25.25 hr** | |

