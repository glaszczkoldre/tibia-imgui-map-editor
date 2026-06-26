# RME-Compatible Doodad Brush Redesign Parity Report

This document outlines the architecture, design choices, and parity alignment achieved in the Doodad Brush redesign to match the reference behavior of Remere's Map Editor (RME).

---

## 1. Overview & Goals

The primary goal of this redesign was to ensure that the Doodad Brush implementation aligns 1-to-1 with RME's handling of XML formats, layout calculations, preview stability, erase behaviors, and border updates, while preserving intentional map editor enhancements (such as contiguous Bresenham drag interpolation and modern Dear ImGui styling).

---

## 2. XML Parsing & Brush Model

- **Parity Alignment**:
  - The `draggable` attribute now defaults to `false` if omitted or malformed, matching RME's default behavior for doodads.
  - Placed items/alternatives are aligned 1-to-1 with RME's XML structures (no flat variation/chance arrays are synthesized).
  - Validation requires a non-negative `chance` on all top-level alternative items and composites. Negative chances are clamped to zero.
  - Composite tiles require both `x` and `y` attributes. Malformed composite tiles or empty composites are skipped.
  - Z-coordinates for composites are validated to ensure they remain within RME's bounds.
- **Implementation**:
  - Moved all doodad-specific XML parsing into [DoodadXmlHelper](file:///ImguiMapEditor/IO/DoodadXmlHelper.cpp) to avoid polluting the core `BrushXmlReader`.

---

## 3. Placement & Erase Planning

To maintain absolute parity, doodad stamp planning is split into two clean phases:

### Phase 1: Raw Stamp Layout Generation
- Done via `generateRawStamp` in `DoodadPlacementPlanner`.
- Generates the raw layout coordinates and item selections *completely independent* of the current map state.
- Retains the interaction variation / seed across cursor movements, guaranteeing that a preview remains perfectly stable as the cursor moves.
- The seed is only regenerated when a stamp is successfully placed (stamped) or when variation cycling is triggered.

### Phase 2: Map Projection & Filtering
- Done via `buildPlan` in `DoodadPlacementPlanner`.
- Projects the raw stamp layout onto the map's coordinates and filters tiles independently.
- Ensures that blocking/duplicate checks are applied per-tile according to RME's rules.

### Erase Behavior
- Erasing a doodad stamp uses `buildErasePlan` which returns the *unfiltered raw stamp footprint* positions.
- This ensures that blocking/blocked coordinates of the doodad stamp are cleared from the map even if the full doodad item does not exist at all coordinates.

---

## 4. Border Updates & Re-alignment

- **Redo Borders**:
  - When a doodad is placed or erased, we run border updates.
  - Parity dictates that we only run re-alignment on `Ground` and `Wall` brushes.
  - Re-alignment checks both owner ID metadata and legacy item bindings, including the ground slot of the tile, ensuring that neighboring walls/grounds connect properly without destroying independent decorations (such as wall decorations).

---

## 5. Verification & Testing

The redesign has been verified against a modular suite of smoke tests:
- `DoodadXmlSmoke`: Verifies XML parsing, draggable defaults, negative chance clamping, and composite validation.
- `DoodadPlanningSmoke`: Tests raw stamp layout generation, map projection, and raw footprint erase planning.
- `DoodadBorderSmoke`: Confirms that `redo_borders` updates grounds and walls correctly, checking owner IDs and legacy item bindings.
- `DoodadContextSmoke`: Verifies eyedropper context-picking, ensuring the correct doodad brush is selected from an item on the map.
- `DoodadPreviewSmoke`: Validates stable seeding during cursor movements.
- `AutoborderSmoke`: Validates that ground and wall autobordering connects and resolves correctly, and neighbor updates do not destroy decorations.
