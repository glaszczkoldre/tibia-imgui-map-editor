# Brushes Audit — Action Plan

> Generated from review of 54 `.cpp`/`.h` files. 53 issues found across 5 phases.

---

## Phase 1 — Quick Wins (remove dead code, fix signatures)

### 1.1 Remove dead enums, namespaces, and factory

- [ ] **`BrushEnums.h:24-45`** — Delete entire `EdgeName` namespace (13 `constexpr string_view`, zero usages)
- [ ] **`BrushEnums.h:188-196`** — Delete `ZoneFlag` enum and `ENABLE_BITMASK_OPERATORS(ZoneFlag)`
- [ ] **`Core/IBrush.h:72-73`** — Delete `BrushPreviewDescriptor::clientSprite()` static factory (zero callers)

### 1.2 Remove dead member functions

- [ ] **`Behaviors/WeightedSelection.h:46-47`** + `.cpp:54-63` — Delete `passesThicknessCheck()`
- [ ] **`Data/DoodadAlternative.h:70`** + `.cpp:25-34` — Delete `getTotalChance()`
- [ ] **`Types/GroundBrush.h:65-66`** + `.cpp:401-422` — Delete `findRuleFor()`
- [ ] **`Types/GroundBrush.h:68`** + `.cpp:444-446` — Delete `isFriendName()`

### 1.3 Remove unused includes and forward declarations

- [ ] **`BrushController.h:21`** — Remove `#include <unordered_map>`
- [ ] **`BrushController.h:28`** — Remove `class DoodadBrush;` forward decl
- [ ] **`BrushRegistry.h:5`** — Remove `#include "Domain/Position.h"`
- [ ] **`BrushRegistry.cpp:2`** — Remove `#include "Domain/ChunkedMap.h"`
- [ ] **`BrushSystem.h:9`** — Remove `#include <memory>`
- [ ] **`BrushSystem.h:18-24`** — Remove `BorderLookupService`, `WallLookupService`, `TableLookupService`, `CarpetLookupService` forward decls
- [ ] **`BrushSystem.h:28`** — Remove `class EditorSession;` forward decl
- [ ] **`Behaviors/ItemPlacement.h:5`** — Remove `#include <memory>`
- [ ] **`Behaviors/ItemPlacement.h:11`** — Remove `struct ItemType;` forward decl
- [ ] **`Types/RawBrush.cpp:2`** — Remove `#include "Brushes/Behaviors/ItemPlacement.h"`
- [ ] **`Types/RawBrush.cpp:8`** — Remove `#include "Domain/Position.h"`
- [ ] **`Types/DoodadPlacementPlanner.h:5`** — Remove `#include <unordered_set>`

### 1.4 Fix unused parameters (unnamed or [[maybe_unused]])

- [ ] **`RawBrush.cpp:19,39`** — Rename `map` → `/*map*/` in `draw()` and `undraw()`
- [ ] **`EraserBrush.cpp:59`** — Rename `map` → `/*map*/` in `undraw()`
- [ ] **`CreatureBrush.cpp:73`** — Rename `map` → `/*map*/` in `undraw()`
- [ ] **`FlagBrush.cpp:26`** — Rename `map` → `/*map*/` in `undraw()`
- [ ] **`HouseBrush.cpp:33`** — Rename `map` → `/*map*/` in `undraw()`
- [ ] **`PlaceholderBrush.h:18`** — Rename all params → unnamed in `draw()` and `undraw()`
- [ ] **`Core/IBrush.h:216`** — Change `size_t /*index*/` → `[[maybe_unused]] size_t` or remove comment

### 1.5 Fix missing noexcept

- [ ] **`BrushController.h:96`** — `BrushController() = default;` → `= default noexcept`
- [ ] **`BrushController.h:228`** — `~BrushController() = default;` → `= default noexcept`
- [ ] **`BrushRegistry.h:39,40`** — Destructor and constructor → `= default noexcept`
- [ ] **`BrushSystem.h:42`** — Destructor → `= default noexcept`
- [ ] **`BrushController.h:78`** — `isValid() const` → `isValid() const noexcept`
- [ ] **`Core/BrushBase.h:50-52`** — Add `noexcept` to `setPreviewDescriptor()`

### 1.6 Fix include paths and order

- [ ] **`Enums/BrushEnums.h:10`** — Change `../../Utils/EnumFlags.h` → `"Utils/EnumFlags.h"`
- [ ] **`Data/BorderBlock.h:7`** — Change `"../Enums/BrushEnums.h"` → `"Brushes/Enums/BrushEnums.h"`
- [ ] **`Data/WallNode.h:7`** — Change `"../Enums/BrushEnums.h"` → `"Brushes/Enums/BrushEnums.h"`
- [ ] **`Data/DoodadAlternative.cpp:9`** — Move `<utility>` above project header per include order rules

### 1.7 Fix types and magic numbers

- [ ] **`Data/DoodadAlternative.h:31-33`** — `int dx, dy, dz` → `int32_t dx, dy, dz`
- [ ] **`Types/CarpetBrush.h:38`** — `std::array<..., 14>` → use `static_cast<size_t>(EdgeType::Count)`
- [ ] **`Types/TableBrush.h:38`** — `std::array<..., 7>` → use `static_cast<size_t>(TableAlign::Count)`
- [ ] **`Types/WallBrush.h:95-96`** — `std::array<WallNode, 17>` → use `kWallAlignCount`
- [ ] **`Types/GroundBrush.cpp:393-394`** — Remove unnecessary `const_cast`

### 1.8 Fix BrushBase encapsulation

- [ ] **`Core/BrushBase.h:55-57`** — Move `name_`, `lookId_`, `draggable_` from `protected` to `private`; mark them `const`

### Smoke test — Phase 1

- [ ] `build_ninja.bat` compiles with zero warnings and zero errors
- [ ] Launch app, open a map, verify brushes palette loads
- [ ] Paint with RawBrush, EraserBrush, CreatureBrush, FlagBrush, HouseBrush, WaypointBrush, SpawnBrush
- [ ] Verify no crash on `undraw()` for each brush type

---

## Phase 2 — Medium (consolidate duplicates, fix patterns)

### 2.1 Deduplicate coordinate encoding

- [ ] Create `encodePosition(Position)` in `Domain/Position.h` (combine `DoodadBrush:25-29` + `DoodadPlacementPlanner:57-61`)
- [ ] Replace all call sites in `DoodadBrush.cpp` and `DoodadPlacementPlanner.cpp`

### 2.2 Deduplicate dedup+sort utilities

- [ ] Create single `dedupeAndSort(std::vector<T>&)` utility in Brushes or Utils
- [ ] Replace `DoodadBrush.cpp:88-94` (`dedupeSorted`) with shared utility
- [ ] Replace `DoodadPlacementPlanner.cpp:176-190` (`dedupeAndSort`) with shared utility

### 2.3 WallNode — use shared WeightedSelection

- [ ] Replace manual weighted random in `WallNode.cpp:14-42` with call to `WeightedSelection::select()`
- [ ] Remove `mutable std::mt19937 rng_` from `WallNode.h:44`

### 2.4 Make lookup services static const in hot paths

- [ ] **`GroundBrush.cpp:780`** — `BorderLookupService borderLookupService;` → `static const`
- [ ] **`CarpetBrush.cpp:137`** — `CarpetLookupService{}` → `static const`
- [ ] **`TableBrush.cpp:137`** — `TableLookupService{}` → `static const`

### 2.5 Fix BrushEnums.cpp parse inefficiency

- [ ] Replace 4x `static std::unordered_map<std::string_view, ...>` with `static constexpr std::array<std::pair<std::string_view, T>, N>` + `std::lower_bound`
- [ ] Add consistent fallback: log warning + return sentinel, or assert for unknown values
- [ ] Make `doorTypeToString` return consistent default (not empty string)

### 2.6 WallDecorationBrush — give it its own .cpp

- [ ] Create `Types/WallDecorationBrush.cpp`
- [ ] Move `draw()` and `undraw()` from `WallBrush.cpp:1023-1075` into new file
- [ ] Move `rebuildTile` helper if applicable

### 2.7 Fix redundant public/protected accessor in WallBrush

- [ ] **`WallBrush.h:74,81`** — Remove `brushRegistry()` protected method; keep only public `getBrushRegistry()`

### Smoke test — Phase 2

- [ ] `build_ninja.bat` compiles with zero errors
- [ ] Paint with GroundBrush, WallBrush, DoodadBrush, CarpetBrush, TableBrush, WallDecorationBrush
- [ ] Use border-aware brushes (GroundBrush across terrain boundaries)
- [ ] Verify weighted selection results haven't changed (brush alternatives randomize correctly)
- [ ] Verify DoodadBrush placement produces same results as before

---

## Phase 3 — Hard (architectural fixes)

### 3.1 Eliminate global state — GroundBrush

- [ ] Move `thread_local AltGroundReplaceState` (`GroundBrush.cpp:29`) into `DrawContext` or per-session edit state
- [ ] Pass via parameter to `placeGroundTile()` and `resetAltGroundReplaceState()`
- [ ] Unify naming: `resetAltGroundReplaceState()` vs `GroundBrush::resetAltReplaceState()` (uppercase/lowercase `alt`/`Alt`)

### 3.2 Fix GLFW layering violations

- [ ] **`DoorBrush.cpp:9`** — Remove `#include <GLFW/glfw3.h>`; define `Modifier_Alt` constant locally or use a key-modifier enum from `DrawContext`
- [ ] **`GroundBrush.cpp:12`** — Same fix; replace `GLFW_MOD_ALT` with abstraction

### 3.3 Deduplicate DrawContext construction in brushes

- [ ] **`CarpetBrush.cpp:158-160, 169-171`** — Extract repeated `DrawContext` init into inline helper or factory
- [ ] **`TableBrush.cpp:157-160`** — Use same helper
- [ ] **`BrushController.cpp:1698-1743`** — Replace manual `DrawContext` construction with `createDrawContext()`

### 3.4 GroundBrush — clean up border struct definitions

- [ ] Move `NeighborState`, `BorderCluster`, `ResolvedBorderRule` from inside `updateBorderItems()` to anonymous namespace or private nested types
- [ ] Fix `addEdgeItems` return value — the `false` return at line 653 is dead since call path only goes through `addEdgeWithFallback`

### Smoke test — Phase 3

- [ ] `build_ninja.bat` compiles with zero errors
- [ ] Alt+click ground replacement works identically to before
- [ ] All modifier-key-dependent brush behaviors work (DoorBrush alt-placement, GroundBrush alt-replace)
- [ ] All complex brush types still draw correctly

---

## Phase 4 — Refactors (file/function size limits)

### 4.1 Split BrushController.cpp (2084 lines → target <500)

- [ ] Extract `BrushStroke.cpp` — `beginStroke()`, `continueStroke()`, `endStroke()`, stroke tracking
- [ ] Extract `BrushDoorOps.cpp` — all 8 door brush management functions + door brush activators
- [ ] Extract `BrushDoodadOps.cpp` — `paintDoodadRecordedPosition()`, `eraseDoodadRecordedPosition()`, doodad placement helpers
- [ ] Extract `BrushEraseOps.cpp` — erase-specific methods
- [ ] Keep `BrushController.cpp` with core orchestration only (`setBrush()`, `applyBrush()`, `createDrawContext()`, `resolveBrush()`)

### 4.2 Split WallBrush.cpp (1077 lines)

- [ ] Move `WallDecorationBrush::draw()` and `WallDecorationBrush::undraw()` to `WallDecorationBrush.cpp` (already done in Phase 2.6)
- [ ] Extract `WallBrushPreview.cpp` — `buildPreviewTiles()` (195 lines) and its helpers
- [ ] Keep `WallBrush.cpp` under ~500 lines

### 4.3 Split GroundBrush.cpp (898 lines)

- [ ] Extract `GroundBorderUpdater.cpp` — `updateBorderItems()`, `addEdgeItems()`, `addEdgeWithFallback()`, border resolution helpers
- [ ] Keep `GroundBrush.cpp` under ~500 lines

### 4.4 Split BrushController.h (475 lines, approaching limit)

- [ ] Extract `BrushDoorManager.h` — 8x `unique_ptr<DoorBrush>` + getter/activator + `PositionHash`
- [ ] Include in `BrushController.h`

### 4.5 Split WallBrush::buildPreviewTiles (195 lines)

- [ ] Extract door selection logic (~50 lines) into `resolveDoorPlacement()`
- [ ] Extract alignment resolution (~40 lines) into `resolveWallAlignment()`
- [ ] Keep main flow in `buildPreviewTiles()` under 100 lines

### Smoke test — Phase 4

- [ ] `build_ninja.bat` compiles with zero errors
- [ ] Full brush regression: every brush type paints correctly
- [ ] Stroke operations (click-drag) work for all brushes
- [ ] Door brush auto-detects wall brushes
- [ ] Undo/redo for complex brush strokes works
- [ ] No new warnings introduced

---

## Verification Plan (run after each phase)

| Check | Command / Action |
|-------|-----------------|
| Build | `build_ninja.bat` — must succeed with 0 errors, 0 new warnings |
| Lint | Run project linter if available (check `AGENTS.md` for command) |
| Smoke — simple brushes | Open any map, paint with RawBrush, EraserBrush, CreatureBrush, FlagBrush, HouseBrush, WaypointBrush, SpawnBrush, PlaceholderBrush |
| Smoke — complex brushes | Paint with GroundBrush, WallBrush, DoodadBrush, CarpetBrush, TableBrush, OptionalBorderBrush, WallDecorationBrush, DoorBrush, HouseExitBrush |
| Smoke — undo/redo | Make 5 brush strokes of different types, Ctrl+Z through all, Ctrl+Y through all |
| Smoke — stroke tracking | Click-drag paint with each brush type that supports dragging |
| Smoke — modifier keys | Alt+click with GroundBrush (alt-replace), Alt+click with DoorBrush |
| Smoke — palette integration | Select brushes from palette windows, verify correct brush activates |
| Smoke — border logic | Paint GroundBrush adjacent to different terrain types, verify border tiles appear |

---

## Totals

| Phase | Items | Effort |
|-------|-------|--------|
| Phase 1 — Quick Wins | 36 | ~2-3 hours |
| Phase 2 — Medium | 12 | ~3-4 hours |
| Phase 3 — Hard | 6 | ~3-5 hours |
| Phase 4 — Refactors | 8 | ~6-8 hours |
| **Total** | **62** | **~14-20 hours** |
