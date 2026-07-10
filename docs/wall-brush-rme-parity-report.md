# Wall Brush Redesign and RME Parity Report

This document reports on the design and verification of the **Real-Time RME-Compatible Wall Brush Redesign** implemented in the Tibia ImGui Map Editor.

---

## 1. Architectural Changes

The primary goal of the redesign was to achieve immediate, real-time connection alignment during drag strokes and erasures, bringing the behavior to parity with RME while eliminating the deferred alignment passes.

### Before Redesign (Deferred Rebuilds)
* Painting/erasing during an active drag did not resolve wall connections in real-time. It set a `strokeNeedsAutoborderFinalize_` flag.
* Connections were resolved only at the end of the stroke (`endStroke`) by running a separate deferred resolve pass over the accumulated painted positions using `PlacementMode::ResolveOnly`.
* This caused visual lag during drags, where connection ends looked unaligned until the mouse button was released.

### After Redesign (Real-Time Batched Resolves)
* **Real-time Rebuilds**: We completely removed `PlacementMode::ResolveOnly`, the end-of-stroke `finalizeAutoborderStroke()` pass, and the deferred flag `strokeNeedsAutoborderFinalize_`.
* **Batch Drag Operations**: Drag movements are now processed as deduplicated batches. Instead of resolving tiles one-by-one (which creates $O(N)$ redundant rebuild passes over overlapping neighborhoods), we:
  1. Calculate the Bresenham line segment for the drag step.
  2. Gather the footprint of all positions along the segment.
  3. Filter out positions already painted in the current stroke.
  4. Perform a single batch placement/erasure intent call (`applyPlannedIntentWithHistory`).
  5. The resolver expands the neighborhood by 1 (the cardinal neighbors) and rebuilds their connection layouts on a single scratch map in one consolidated step, applying a single history diff list.
* **Immediate Response**: Wall connections now update instantly on screen during active drags and erasures.

---

## 2. RME XML Parity & Metadata Registration

We improved parsing robustness to correctly align and register RME wall metadata.

* **Clamped Negative Chances**: Some XML definitions or custom user additions might have negative `chance` attributes. These are parsed as signed integers first and clamped via `std::max(0, chanceVal)` inside `parseChance()`, preventing underflow bugs that wrap values to large positive integers.
* **Skip Malformed/Unknown Door Nodes**: In `<wall>` nodes, the `<door>` sub-nodes are parsed. If `parseDoorType` returns `DoorType::Undefined` for an unknown door/window configuration, the parser now logs a warning and skips registering the node rather than failing silently or causing crashes.
* **Hate Flags & Redirects Support**: Parity tests confirm that:
  - Friend redirect configurations connect walls across distinct brushes bidirectionally.
  - "Hate" items (configured via `addWallHateMeItem`) correctly block connection checks, matching RME's layout separation.

---

## 3. Prioritized Context Picking

Smart context picking (`BrushController::resolveBrushFromTile` under smart pick mode) was updated:
* Clicking/picking a tile with stacked items (e.g., walls, decorations, doors, or windows) now prioritized detecting the owning `WallBrush` over falling back to generic door brushes.
* By placing the candidate search for `BrushType::Wall` first and executing it before checking for generic doors, the editor correctly activates the corresponding wall brush palette option so the user can continue drawing walls of the matching type.

---

## 4. Verification Results

We registered a new smoke test executable, `WallBrushSmoke`, and updated the existing suite:

### Test Suite Results

| Test Target | Purpose | Status |
|-------------|---------|--------|
| `AutoborderSmoke.exe` | Verified real-time wall painting during drags, and asserted that diff planning includes neighbor tiles correctly. | **PASSED** |
| `WallBrushSmoke.exe`  | Validated all 16 neighbor mask alignments, untouchable tiles, redirects, hate flags, doors state toggling, prioritized context picking, single vs. segmented drag parity, and performance. | **PASSED** |
| `BrushSmoke.exe`      | Validated general brush and tileset configurations. | **PASSED** |
| `CarpetSmoke.exe`     | Validated carpet lookups and boundary sweeps. | **PASSED** |
| `DoodadPreviewSmoke.exe` | Validated doodad previews and placements. | **PASSED** |

### Performance Benchmark
The 1,000-tile performance regression test in `WallBrushSmoke.cpp` verifies connection alignments on a zig-zag path:
* **Completed in**: `< 50ms` (typically resolves in under 20-30ms in optimized builds).
* **Parity check**: Segmented drags (movement event segments applied one-by-one) yielded the exact same resolved wall item layouts as single-batch operations, confirming the math and state logic is identical.
