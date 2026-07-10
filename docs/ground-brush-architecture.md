# Ground Brush & Auto-Border Pipeline — Architecture & Parity Report

> **Status**: The core pipeline is functionally complete. This report documents the design, RME parity, known gaps, and future direction.

---

## 1. RME Pipeline (Reference)

### 1.1 Stroke Flow
```
User drags ground brush
  → BrushUtility::GetTilesToDraw()
      → Returns { tilesToDraw, tilesToBorder } (brush footprint + one ring of neighbors)
  → drawGroundOrEraserImpl():
      1. For each tile in tilesToDraw:
         a. Deep-copy tile (undo snapshot)
         b. Clean existing borders
         c. brush->draw(map, tile) → places ground item + sets ground brush identity
      2. For each tile in tilesToBorder:
         a. Deep-copy tile
         b. TileOperations::borderize(tile, map)
           → GroundBorderCalculator::calculate()
      3. Commit batch action
```

### 1.2 Border Calculation (`GroundBorderCalculator::calculate()`)
```
1. Get center tile's ground brush
2. Scan 8 neighbors (NW,N,NE,W,E,SW,S,SE) → build {brush or nullptr} per direction
3. Build border clusters:
   For each unvisited neighbor:
     - Group same-brush adjacent tiles into a bitmask (TileAlignment)
     - Resolve which BorderBlock applies:
       * center.z < neighbor.z AND neighbor has outer border?
         → Try center's inner border, then neighbor's outer border
       * Otherwise:
         → Try center's inner border
     - Handle zilch (empty/groundless neighbors) → z = -1000
     - Handle optional borders (mountain/gravel) → z = MAX_INT
     - Check friend/enemy → friends skip normal borders
4. Sort clusters by z-order (ascending → back-to-front)
5. Render borders: for each cluster, lookup `border_types[alignment]` → 1-4 EdgeTypes
   - Diagonal fallback: if DNW defined but no diagonal item → place W + N
6. Process specific cases:
   - match_border / match_item / match_group → collect matching items
   - replace_border / replace_item → swap one item
   - delete_borders → remove all matched
   - keep_border → preserve first match, delete rest
```

### 1.3 Key Data Structures
- `BorderType` enum: 13 values (4 horizontals, 4 corners, 4 diagonals) + NONE
- `TileAlignement` bitmask: 8 bits for NW,N,NE,W,E,SW,S,SE
- `border_types[256]` lookup table: maps 8-neighbor bitmask → packed uint32 of up to 4 EdgeTypes
- `AutoBorder` class: 13-item array indexed by BorderType, plus id/group/ground fields
- `SpecificCaseBlock`: match conditions + replace/delete/keep actions

---

## 2. Our Pipeline

### 2.1 Stroke Flow
```
User drags ground brush
  → BrushController::continueStroke() → continueGroundLikeStroke()
    → paintExpandedCenter(pos) → for each brush-footprint position:
      → paintRecordedPosition(pos)
        → AutoborderEngine::plan() [scratch-map clone]
          1. GroundResolver::expandAffectedPositions() → 3×3 around each intent pos, deduplicated
          2. Clone source tiles into scratch map
          3. GroundResolver::applyIntent() → placeGroundTile() on scratch map
          4. GroundResolver::resolve() → rebuildTile() for each affected pos
          5. Compute TileDiffs → emit only changed tiles
        → applyPlannedIntentWithHistory() → record undo, apply diffs
```

### 2.2 Border Calculation (`GroundBrush::updateBorderItems()`)
```
1. Clear existing border items from tile
2. Resolve ground brush for tile and all 8 neighbors
3. Build border clusters (group same-brush neighbors into TileNeighbor bitmask):
   - Same brush → skip
   - Friend/connected brush with optional border → only optional, skip normal
   - Resolve rule via resolveRuleTo():
     * center.z < neighbor.z AND neighbor has outer border?
       → Try center's inner rule, then neighbor's outer rule
     * Otherwise → try center's inner rule
     * Zilch (empty neighbor): use center's inner zilch rule, z=-1000
     * Zilch (empty center, neighbor brush): use neighbor's outer zilch rule
4. Optional border handling:
   - tile.hasOptionalBorder() AND other.has optionalBorderBlock → cluster with z=MAX_INT
   - soloOptional → skip normal borders entirely
5. Sort clusters by z-order
6. Render borders back-to-front (reverse iterator):
   - BorderLookupService::getBorderTypes(alignment) → up to 4 EdgeTypes
   - For each edge → BorderBlock::getRandomItem(edge) with diagonal fallback
7. Process specific cases:
   - match_border/match_item/match_group → scan tile's border items
   - replace_border/replace_item → swap one item in-place
   - delete_borders → remove all matched
   - keep_border → keep first match, delete rest
```

### 2.3 Preview Pipeline
```
AutoborderBrushPreviewProvider::buildPreview()
  → AutoborderEngine::plan(sourceMap, intent)
    → Same GroundResolver as commit (same scratch-map + apply + resolve + diff)
  → Extract items from diff.after tiles → PreviewTileData
```
**Preview and commit use the identical code path.** This guarantees pixel-identical results.

---

## 3. RME Parity Matrix

| Feature | RME | Ours | Parity |
|---------|-----|------|--------|
| Ground tile placement | `terrain_placement.cpp` | `placeGroundTile()` | ✅ |
| Ground equivalent | `setGround(Item::Create(base))` | `metadata->groundEquivalent` | ✅ |
| Weighted random ground items | `ItemChanceBlock` | `WeightedSelection::select()` | ✅ |
| 8-neighbor scan | `kNeighborOffsets` | Same array | ✅ |
| 256-entry border lookup | `border_types[256]` | `BorderLookupTable.inc` (auto-gen from RME) | ✅ |
| Z-order priority (inner vs outer) | `getBrushTo()` | `resolveRuleTo()` | ✅ |
| Friend/enemy semantics | `friendOf()` with `hate_friends` | `connectsTo()` with `hateFriends_` | ✅ |
| Optional borders | `TILESTATE_OP_BORDER` | `tile.hasOptionalBorder()` / `optionalBorder_` | ✅ |
| Solo optional (skip normal) | `use_only_optional` | `soloOptionalBorder_` | ✅ |
| Outer zilch (border vs nothing) | `zilch outer/inner` | `hasOuterZilchBorderRule()` / `targetNone` | ✅ |
| Inner zilch (empty center vs brush) | `zilch` | `borderBrush==nullptr && neighbor has outer zilch` | ✅ |
| match_border | By border id + edge | By item ID (meta-driven) | ✅ |
| match_item | By direct item ID | By direct item ID | ✅ |
| match_group | By group + edge | By `BorderItemMetadata` group + alignment | ✅ |
| replace_border | Replace border item | Replace by item ID | ✅ |
| replace_item | Replace by item ID | Replace by item ID | ✅ |
| delete_borders | Delete all matched | Delete all matched | ✅ |
| keep_border | Keep first, delete rest | Keep first, delete rest | ✅ |
| Diagonal fallback | If no diagonal item, use two horizontals | Same: `addEdgeWithFallback()` | ✅ |
| ALT-replace (swap terrains) | `alt && current == replaceBrush` | `shouldSkipAltGroundPlacement()` | ✅ |
| clear_borders / clear_friends | XML directive | `clearBorderRules()` / `clearFriends()` | ✅ |
| Deterministic RNG | ~ | `ScopedSeed` + `deterministicSeed()` | ✅ |
| Live autoborder preview | `AutoborderPreviewManager` | `AutoborderBrushPreviewProvider` | ✅ |
| Scratch-map planning | N/A (RME applies directly to map) | `AutoborderEngine::plan()` | ✅ (ours is cleaner) |
| Multi-tile batch planning | `BrushUtility::GetTilesToDraw()` | Engine plans per-position during drag | ⚠️ (see §4.2) |
| `super` attribute on borders | Parsed, never checked | Parsed, never checked | ✅ (dead field in both) |
| Wall/Carpet/Table brushes | Separate calculators | Separate resolvers | ✅ |

---

## 4. Known Gaps & Design Limitations

### 4.1 Per-Position Planning During Drag (Performance)
**Current**: `paintExpandedCenter()` calls `paintRecordedPosition()` for each brush-footprint position. Each call creates a new `AutoborderEngine`, clones tiles, runs resolve, and commits.

**Impact**: For a 5×5 brush during drag, 25 separate engine plans run per frame. Each plan clones a 5×5 → 7×7 area (49 tiles). Total: 25 × 49 = 1225 tile clones per frame. This is O(brush_area × expanded_area) instead of O(expanded_all_positions).

**RME Approach**: Collects all tiles-to-draw first, then runs one borderization pass on all tiles-to-border. O(expanded_all_positions).

**Recommended Fix**: Collect all brush-footprint positions for the frame, pass them as a single `PlacementIntent.positions` vector to one engine plan. The `GroundResolver` already handles multi-position intents correctly — each position gets `placeGroundTile()`, then deduplicated affected area gets `rebuildTile()`.

### 4.2 AutoborderEngine Per-Call Construction
**Current**: `paintRecordedPosition()` and `eraseRecordedPosition()` construct a new `AutoborderEngine` on each call (line 1554/1592 of BrushController.cpp).

**Impact**: Negligible (engine construction is cheap — 4 resolver allocations, ~100 bytes). But the repeated template instantiation in applyPlannedIntentWithHistory is unnecessary.

### 4.3 No Drag-Skip for Ground Brushes (Unlike Walls)
**Current**: `WallResolver::resolve()` skips neighbor border resolution while dragging (`skipsLiveWallResolve`), deferring to `endStroke()`. GroundResolver does NOT have this optimization.

**Design Decision**: This is intentional. Ground brush borders are relatively cheap to compute (just neighbor lookups, no complex owner-checking like walls). Ground brush borders also need to be visible during drag for the user to see the terrain transitions. The per-tile approach is acceptable for ground because:
- Each tile's border calculation is local (8 neighbors × simple lookup)
- No deferred finalize pass needed
- Visual feedback is important during terrain painting

### 4.4 `rebuildTile` vs `rebuildAround` in GroundResolver
**Current**: `GroundResolver::resolve()` calls `rebuildTile()` for each affected position. `rebuildTile()` correctly resolves the tile's OWN ground brush (not the paint brush) and uses that brush's border rules.

**Potential Issue**: If multiple adjacent tiles change ground type during the same stroke, `resolve()` processes them sequentially. Since each `rebuildTile()` only modifies its own tile's border items, and since the source map is the scratch map (which already has all ground changes applied), this is correct — tile A's borders are computed with full knowledge of tile B's new ground type.

### 4.5 Preview-Only Ghost Mode
**Current**: `AutoborderBrushPreviewProvider` has two styles: `Ghost` (semi-transparent overlay) and `Outline` (border-only). Both use the full autoborder planner.

**Design**: Ghost mode shows the complete result including ground + borders. Outline mode shows just borders. Neither is a "static placeholder."

---

## 5. Proposed Design (Already Implemented)

The current architecture is the proposed design:

### 5.1 Planner Pattern
```
                    ┌─────────────────┐
                    │ AutoborderEngine │  ← stateless, reusable
                    │   plan(map,      │
                    │        intent)   │
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              ▼              ▼
     GroundResolver    WallResolver    CarpetResolver
    (placeGround +    (placeWall +     (placeCenter +
     rebuildTile)      rebuildTiles)    rebuildAround)
```

Both preview and commit call `engine.plan()`:
- **Preview**: `AutoborderBrushPreviewProvider::buildPreview()` → `engine.plan(sourceMap, intent)` → display diffs
- **Commit**: `applyPlannedIntentWithHistory(engine, map, history, intent)` → apply diffs with undo

### 5.2 Single Source of Truth for Border Calculation
`GroundBrush::updateBorderItems()` is THE canonical border calculation. It is called by:
- `rebuildTile()` (used by AutoborderEngine and direct draw)
- `rebuildAround()` (used by direct draw fallback)

There is no second implementation. The `GroundResolver` delegates to `rebuildTile()`.

### 5.3 Deterministic Placement
```cpp
// AutoborderEngine::plan() — deterministic seed
MapEditor::Brushes::WeightedSelection::ScopedSeed scopedSeed(
    deterministicSeed(intent));
// same seed → same random items → pixel-identical preview and commit
```

---

## 6. File Ownership Map

| File | Owns |
|------|------|
| `Brushes/Types/GroundBrush.h/.cpp` | Border calculation, friend/enemy, optional borders, specific cases, z-order resolution |
| `Brushes/Data/BorderBlock.h/.cpp` | Border item storage per edge type, specific case data model |
| `Brushes/Enums/BrushEnums.h` | EdgeType (14 values), TileNeighbor (8-bit mask), edge name strings |
| `Brushes/BrushRegistry.h/.cpp` | Border template metadata, item-to-brush bindings |
| `Services/Autoborder/AutoborderEngine.h/.cpp` | Scratch-map planning, GroundResolver/WallResolver/CarpetResolver/TableResolver |
| `Services/Autoborder/AutoborderResolver.h` | Resolver interface |
| `Services/Autoborder/PlannedMutation.h/.cpp` | Engine → history bridge |
| `Services/Autoborder/TileDiff.h/.cpp` | Change tracking between plan and map |
| `Services/Brushes/BorderLookupService.h/.cpp` | 256-entry direction lookup table |
| `Services/Brushes/BorderLookupTable.inc` | Auto-generated from RME reference |
| `Services/Preview/AutoborderBrushPreviewProvider.h/.cpp` | Live autoborder preview |
| `IO/BrushXmlReader.h/.cpp` | Ground brush XML parsing including borders, specific cases, etc. |
| `IO/MaterialsXmlReader.cpp` | Border template registration from materials.xml |
| `Tests/BrushSmoke.cpp` | Smoke tests: z-order, friend/enemy, optional, outer zilch, specific cases, real data |
| `Tests/AutoborderSmoke.cpp` | Integration tests: preview/commit parity, undo/redo, drag behavior |

---

## 7. Remaining Known Gaps (Non-Critical)

1. **`super` border attribute** — Parsed in XML but never used by border calculation in either RME or our codebase. Dead field.

2. **Multi-position batch planning** — The engine supports it, but `paintExpandedCenter()` feeds positions one at a time. Fixing this would reduce clone overhead for large brushes (see §4.1).

3. **Engine caching** — `AutoborderEngine` is constructed on each `paintRecordedPosition()` call. Trivial to cache.

4. **`resolve()` granularity** — `GroundResolver::resolve()` calls `rebuildTile()` per position rather than using `rebuildAround()`. Functionally identical but slightly less clear.

### 4.6 Friend/Enemy Border Suppression (Fixed)
**Fixed**: The friend check in `updateBorderItems()` was only suppressing normal borders when the friend brush had an optional border rule. RME suppresses ALL normal borders between friends regardless of optional border status (optional borders are a separate pass). This was causing incorrect border placement between friend brushes like "earth (stone border)" and "earth".

**Fix location**: `Brushes/Types/GroundBrush.cpp:723-741` — changed from conditional `&& other->hasOptionalBorderRule()` to an early `continue` when friends lack optional borders.

### 4.7 Engine Caching & Batched Painting (Fixed)
**Fixed**: `BrushController` was constructing a new `AutoborderEngine` on every `paintRecordedPosition` / `eraseRecordedPosition` call. Now uses a cached member `autoborderEngine_`.

**Batching**: `paintExpandedCenter` now passes all brush-footprint positions to `paintRecordedPositions()` which creates a single engine plan instead of N separate plans. This reduces O(n × expanded_area) to O(expanded_all_area) for multi-tile brushes.

---

## 8. Build & Test

```bash
# Build
.\build_ninja.bat

# Run brush smoke tests (ground-specific: z-order, friend/enemy, optional, zilch, specific cases, real data)
.\build\bin\BrushSmoke.exe

# Run autoborder smoke tests (preview/commit parity, undo/redo, drag)
.\build\bin\AutoborderSmoke.exe
```

---

## 9. Conclusion

The ground brush auto-border pipeline is feature-complete and RME-parity. The key architectural insight is the **planner pattern**: a single `AutoborderEngine::plan()` call produces deterministic TileDiffs that are used identically for both preview and committed placement. Border calculation lives in one place (`GroundBrush::updateBorderItems()`), and the 256-entry lookup table is auto-generated from RME's reference logic to guarantee pixel-identical direction resolution.

The primary optimization opportunity is batching multi-position drag strokes into a single engine plan to reduce tile clone overhead. This does not affect correctness, only performance for large brushes during drag.
