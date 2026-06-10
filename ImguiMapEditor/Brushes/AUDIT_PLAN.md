# Remaining Issues — Architecture-Aligned Action Plan

> Previous phases (dead code, noexcept, includes, magic numbers, GLFW layering, dedup,
> `map.markChanged()` consistency, preview/styling fixes) are **complete and LGTM**.
>
> This plan covers the 7 remaining items, re-evaluated against `AGENTS.md` rules.

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

## Execution Order

```
Item 1 (parse perf)      — 30 min, independent, zero risk
    ↓
Item 5 (DrawContext dedup) — 30 min, independent, zero risk
Item 6 (border structs)    — 20 min, independent, zero risk
    ↓
Item 4 (global state)      — 2 hr, touches DrawContext, blocks nothing
    ↓
Item 2 (WallDecorationBrush) — 1 hr, clean separation
    ↓
Item 3 (const_cast)       — 2 hr, mechanical, 8 files
    ↓
Item 7 (file splits)      — 4 hr, depends on Item 2 for WallDecorationBrush
```

Items 1+5+6 can be done in parallel. Item 4 is the most important architecturally (violates "no global state") but also the highest risk — do it after low-risk items and before the larger refactors.

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
| | **Total** | **~10.5 hr** | |
