# Production-Grade ImGui UI/UX Design

> How to make Dear ImGui applications look and feel like Photoshop, Figma, Blender, or Unreal Editor.
> This is the difference between a hobby tool and a professional one.

---

## AGENT DIRECTIVE

> **When you see UI code that violates these principles — fix it proactively.**
> Do not wait for the user to ask. If a panel has no spacing, inconsistent widget widths,
> missing labels, hardcoded sizes, or raw unstyled widgets — refactor it as part of your work.
> A professional tool demands professional UI. Treat bad UI as a code smell equal to a bad algorithm.

---

## 1. Layout Architecture

### 1.1. The Visual Hierarchy Pyramid

Every professional tool follows the same hierarchy:

```
┌─────────────────────────────────────────────────┐
│  SHELL          Menu bar, toolbar, status bar    │  ← Fixed chrome
├──────────────────┬──────────────────────────────┤
│  NAVIGATION      │  CONTENT                      │  ← Docked panels
│  (tree, list,    │  (viewport, canvas,           │
│   palette)       │   central workspace)          │
│                  ├──────────────────────────────│
│                  │  INSPECTOR / PROPERTIES       │  ← Detail panels
├──────────────────┴──────────────────────────────┤
│  FEEDBACK        Log, status, notifications      │  ← Bottom bar
└─────────────────────────────────────────────────┘
```

**Rules:**
- Navigation panels go LEFT or TOP — user scans left-to-right, top-to-bottom
- The largest panel is the CONTENT area — this is where the user works
- Inspector/Properties go RIGHT — detail view of selected content
- Feedback/Log goes BOTTOM — low priority, available on demand
- Status bar goes at the very bottom — always visible, never interactive

### 1.2. Panel Sizing Principles

| Principle | ❌ Bad | ✅ Good |
|-----------|-------|---------|
| Panel width | Hardcoded `400px` | Proportional split via DockBuilder (`0.25f`) |
| Widget width | Fixed `200px` for inputs | `SetNextItemWidth(-FLT_MIN)` (fill available) |
| Label width | Variable per-label | Fixed column via `BeginTable` with `WidthFixed` |
| Min size | No minimum → panel collapses to nothing | `SetNextWindowSizeConstraints(min, max)` |
| Spacing | Random padding values | Derive from `style.ItemSpacing`, `style.FramePadding` |

### 1.3. Responsive Layouts

Professional tools adapt to window size. ImGui does this naturally IF you avoid hardcoded sizes.

```cpp
// ❌ BAD — Breaks when panel resizes
ImGui::SetNextItemWidth(200.0f);
ImGui::InputText("##name", buf, sizeof(buf));

// ✅ GOOD — Fills available width
ImGui::SetNextItemWidth(-FLT_MIN);
ImGui::InputText("##name", buf, sizeof(buf));

// ✅ GOOD — Percentage of available width
float avail = ImGui::GetContentRegionAvail().x;
ImGui::SetNextItemWidth(avail * 0.6f);
ImGui::InputText("##name", buf, sizeof(buf));
ImGui::SameLine();
ImGui::SetNextItemWidth(-FLT_MIN);  // Remaining space for second widget
ImGui::InputText("##suffix", buf2, sizeof(buf2));
```

---

## 2. Window & Panel Design

### 2.1. Every Panel Must Have

| Element | Why | How |
|---------|-----|-----|
| **Clear title** | User must instantly know what this panel does | Descriptive name: "Item Properties", not "Panel 2" |
| **Consistent padding** | Visual rhythm, breathing room | Use `style.WindowPadding` — never `ImVec2(0,0)` unless viewport |
| **Section separators** | Break content into scannable groups | `SeparatorText("Section Name")` (1.90+) or `Separator()` + `Text()` |
| **Scroll behavior** | Long content must not clip | `BeginChild()` with scroll, or natural window scroll |
| **Empty state** | User must know why a panel is blank | `TextDisabled("No selection")` or placeholder guidance |

### 2.2. Panel Anti-Patterns

```cpp
// ❌ BAD — Wall of widgets, no grouping, no labels, no breathing room
ImGui::DragFloat("##1", &x);
ImGui::DragFloat("##2", &y);
ImGui::DragFloat("##3", &z);
ImGui::ColorEdit4("##4", col);
ImGui::Checkbox("##5", &flag);
ImGui::InputText("##6", buf, sizeof(buf));

// ✅ GOOD — Grouped, labeled, spaced, scannable
ImGui::SeparatorText("Transform");
if (UIWidgets::BeginPropertyTable("transform")) {
    UIWidgets::PropertyDragFloat("X", &x);
    UIWidgets::PropertyDragFloat("Y", &y);
    UIWidgets::PropertyDragFloat("Z", &z);
    UIWidgets::EndPropertyTable();
}

ImGui::SeparatorText("Appearance");
if (UIWidgets::BeginPropertyTable("appearance")) {
    UIWidgets::PropertyLabel("Color");
    ImGui::ColorEdit4("##color", col, ImGuiColorEditFlags_NoInputs);
    UIWidgets::PropertyCheckbox("Visible", &flag);
    UIWidgets::EndPropertyTable();
}

ImGui::SeparatorText("Metadata");
if (UIWidgets::BeginPropertyTable("metadata")) {
    UIWidgets::PropertyLabel("Name");
    ImGui::InputText("##name", buf, sizeof(buf));
    UIWidgets::EndPropertyTable();
}
```

### 2.3. Empty States

Every panel that can be empty MUST communicate why and what the user should do.

```cpp
// ❌ BAD — Blank panel, user is confused
if (selection.empty()) return;

// ✅ GOOD — Helpful empty state
if (selection.empty()) {
    // Center the message vertically
    float avail_h = ImGui::GetContentRegionAvail().y;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail_h * 0.4f);

    // Centered, dimmed guidance
    float text_w = ImGui::CalcTextSize("Select an item to view properties").x;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - text_w) * 0.5f);
    ImGui::TextDisabled("Select an item to view properties");
    return;
}
```

---

## 3. Control Design

### 3.1. Property Editors — The Core Pattern

Every professional tool (Photoshop, Unity, Blender, Unreal) uses the same property editor layout:

```
┌──────────────────────────────────────────────┐
│  Label (fixed width)  │  Widget (stretches)  │
├───────────────────────┼──────────────────────┤
│  Position X           │  [====== 42.0 ======]│
│  Position Y           │  [====== 18.5 ======]│
│  Rotation             │  [====== 90.0 ======]│
│  Name                 │  [Entity_001        ]│
│  Visible              │  [✓]                 │
└───────────────────────┴──────────────────────┘
```

**Implementation:** Always use a 2-column table with `WidthFixed` label + `WidthStretch` value.

```cpp
if (ImGui::BeginTable("props", 2, ImGuiTableFlags_BordersInnerV)) {
    ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
    ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);

    // Every row follows the same pattern:
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();  // CRITICAL: Align label with widget
    ImGui::TextUnformatted("Position X");
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);  // CRITICAL: Fill column
    ImGui::DragFloat("##pos_x", &pos.x, 0.1f);

    ImGui::EndTable();
}
```

### 3.2. Widget Width Rules

| Scenario | Width | Why |
|----------|-------|-----|
| Single widget in column | `-FLT_MIN` (fill all) | Clean alignment, no ragged edges |
| Label + widget on same line | Remaining space after label | Label takes fixed space |
| Multiple widgets on one row | Split available width | `avail * 0.5f` each, or use table columns |
| Small toggle/checkbox | Don't set width | Checkboxes have natural small size |
| Color picker inline | Use `ImGuiColorEditFlags_NoInputs` | Compact color swatch, no input fields |

### 3.3. Alignment Rules

```cpp
// CRITICAL: Always align text labels to frame padding
// Without this, labels sit at text baseline while widgets sit at frame baseline
ImGui::AlignTextToFramePadding();  // Call BEFORE the text
ImGui::TextUnformatted("Label");
ImGui::SameLine();
ImGui::DragFloat("##val", &val);
```

### 3.4. Button Design

| Pattern | When | Example |
|---------|------|---------|
| Text button | Primary actions | `ImGui::Button("Save")` |
| Icon button | Toolbar, compact | `ImGui::Button(ICON_FA_SAVE)` with small frame padding |
| Icon + text | Important actions | `ImGui::Button(ICON_FA_SAVE " Save")` |
| Danger button | Destructive actions | Red-styled wrapper: `DangerButton("Delete")` |
| Small button | Inline, per-row | `ImGui::SmallButton("X")` |
| Disabled button | Unavailable | `BeginDisabled(!can_save); Button("Save"); EndDisabled();` |

**Toolbar pattern:**
```cpp
// Compact toolbar with icon buttons
ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));

if (ImGui::Button(ICON_FA_FILE)) newFile();
ImGui::SameLine();
if (ImGui::Button(ICON_FA_FOLDER_OPEN)) openFile();
ImGui::SameLine();
ImGui::BeginDisabled(!has_changes);
if (ImGui::Button(ICON_FA_SAVE)) saveFile();
ImGui::EndDisabled();
ImGui::SameLine();
ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
ImGui::SameLine();
if (ImGui::Button(ICON_FA_UNDO)) undo();
ImGui::SameLine();
if (ImGui::Button(ICON_FA_REDO)) redo();

ImGui::PopStyleVar(2);
```

### 3.5. Input Commit Patterns

| Pattern | Widget | Use |
|---------|--------|-----|
| **Live update** | `DragFloat`, `SliderFloat`, `ColorEdit` | Preview changes in real-time (visual props) |
| **Commit on finish** | `InputText`, `InputFloat` | Commit when user presses Enter/Tab or clicks away |
| **Undo boundary** | Any editor widget | Push undo ONCE on `IsItemDeactivatedAfterEdit()` |

```cpp
// Live preview + single undo on commit
ImGui::DragFloat("##val", &value, 0.1f);
if (ImGui::IsItemActivated())
    undo_snapshot = value;  // Capture state when drag starts
if (ImGui::IsItemDeactivatedAfterEdit())
    history.push(undo_snapshot, value);  // Single undo entry for entire drag
```

---

## 4. Visual Design System

### 4.1. Spacing & Rhythm

Professional UIs have **consistent visual rhythm**. Every measurement derives from the base grid.

```cpp
// Define your spacing scale from style
const float unit  = style.ItemSpacing.y;      // Base unit (~4px)
const float gap   = style.ItemSpacing.y * 2;  // Section gap (~8px)

// Use SeparatorText for section breaks (1.90+)
ImGui::SeparatorText("Section Name");

// Use Spacing() or Dummy() for custom gaps
ImGui::Dummy(ImVec2(0, gap));  // Vertical spacer

// NEVER use magic pixel values
// ❌ ImGui::Dummy(ImVec2(0, 12));
// ✅ ImGui::Dummy(ImVec2(0, style.ItemSpacing.y * 3));
```

### 4.2. Color System

A coherent UI uses a **small, intentional color palette** — not random colors.

| Role | Purpose | Example |
|------|---------|---------|
| **Background** | Window/panel fills | 3-4 shades of neutral gray |
| **Surface** | Frames, inputs, cards | Slightly lighter than background |
| **Primary** | Active state, selected items, accent | One brand color (e.g., blue) |
| **Danger** | Delete, error, destructive | Red family |
| **Success** | Confirmation, positive state | Green family |
| **Warning** | Caution, unsaved changes | Amber/orange family |
| **Text** | Primary, secondary, disabled | White → gray → dim gray |

```cpp
// Semantic color constants — SINGLE SOURCE OF TRUTH
namespace Theme {
    // Text hierarchy
    inline constexpr ImVec4 TextPrimary   = {1.00f, 1.00f, 1.00f, 1.00f};
    inline constexpr ImVec4 TextSecondary = {0.70f, 0.70f, 0.70f, 1.00f};
    inline constexpr ImVec4 TextDisabled  = {0.45f, 0.45f, 0.45f, 1.00f};

    // Semantic actions
    inline constexpr ImVec4 Danger        = {0.80f, 0.20f, 0.20f, 1.00f};
    inline constexpr ImVec4 Success       = {0.20f, 0.72f, 0.35f, 1.00f};
    inline constexpr ImVec4 Warning       = {0.90f, 0.70f, 0.15f, 1.00f};
    inline constexpr ImVec4 Primary       = {0.26f, 0.59f, 0.98f, 0.80f};
}
```

### 4.3. Typography

| Level | Purpose | Implementation |
|-------|---------|----------------|
| **Heading** | Section titles, panel titles | `SeparatorText()` or `PushFont(bold_font, size)` |
| **Body** | Labels, descriptions | Default font at default size |
| **Caption** | Hints, secondary info | `TextDisabled()` or smaller font |
| **Monospace** | Values, paths, IDs | Separate monospace `ImFont*` |

```cpp
// Visual hierarchy with text
ImGui::SeparatorText("Transform");                        // Section heading
ImGui::TextUnformatted("Position");                       // Body label
ImGui::TextDisabled("World coordinates, in pixels");      // Caption hint
```

### 4.4. Icons as First-Class Citizens

Professional tools use icons to reduce cognitive load. Text labels alone create visual clutter.

```cpp
// ❌ BAD — Text-only toolbar, hard to scan
if (ImGui::Button("New")) {}
ImGui::SameLine();
if (ImGui::Button("Open")) {}
ImGui::SameLine();
if (ImGui::Button("Save")) {}

// ✅ GOOD — Icons with tooltips
if (ImGui::Button(ICON_FA_FILE)) newFile();
ImGui::SetItemTooltip("New (Ctrl+N)");
ImGui::SameLine();
if (ImGui::Button(ICON_FA_FOLDER_OPEN)) openFile();
ImGui::SetItemTooltip("Open (Ctrl+O)");
ImGui::SameLine();
if (ImGui::Button(ICON_FA_SAVE)) saveFile();
ImGui::SetItemTooltip("Save (Ctrl+S)");
```

---

## 5. Information Density

### 5.1. Progressive Disclosure

Don't show everything at once. Reveal complexity on demand.

| Technique | When | ImGui Implementation |
|-----------|------|---------------------|
| **Collapsing headers** | Group optional sections | `CollapsingHeader("Advanced")` |
| **Tree nodes** | Hierarchical data | `TreeNode("Details")` |
| **Tabs** | Parallel content categories | `BeginTabBar` / `BeginTabItem` |
| **Tooltips** | Extra info on hover | `SetItemTooltip()` or `BeginItemTooltip()` |
| **Context menus** | Secondary actions | `BeginPopupContextItem()` |
| **Modal dialogs** | Rare, important actions | `OpenPopup()` + `BeginPopupModal()` |

```cpp
// Progressive disclosure example — Property panel
ImGui::SeparatorText("General");
PropertyDragFloat("X", &x);
PropertyDragFloat("Y", &y);

// Advanced section collapsed by default
if (ImGui::CollapsingHeader("Advanced", ImGuiTreeNodeFlags_DefaultOpen)) {
    PropertyDragFloat("Rotation", &rot);
    PropertyDragFloat("Scale", &scale);
}

// Rarely-used section closed by default
if (ImGui::CollapsingHeader("Debug Info")) {
    ImGui::TextDisabled("ID: %d", entity_id);
    ImGui::TextDisabled("Chunk: (%d, %d)", chunk_x, chunk_y);
}
```

### 5.2. Data Tables — Not Just for Data

Use `BeginTable` for ANY multi-column layout, not just data grids:

```cpp
// Two-column settings layout
if (ImGui::BeginTable("settings", 2, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextRow();

    ImGui::TableNextColumn();
    ImGui::SeparatorText("Display");
    ImGui::Checkbox("Show grid", &show_grid);
    ImGui::Checkbox("Show guides", &show_guides);
    ImGui::SliderFloat("Zoom", &zoom, 0.1f, 10.0f);

    ImGui::TableNextColumn();
    ImGui::SeparatorText("Behavior");
    ImGui::Checkbox("Snap to grid", &snap);
    ImGui::Checkbox("Auto-save", &auto_save);
    ImGui::SliderInt("Undo depth", &undo_depth, 10, 500);

    ImGui::EndTable();
}
```

### 5.3. Lists with Visual Weight

```cpp
// ❌ BAD — Flat list, everything looks the same
for (auto& item : items) {
    ImGui::Text("%s", item.name.c_str());
}

// ✅ GOOD — Visual hierarchy, scannable, interactive
for (int i = 0; i < items.size(); i++) {
    ImGui::PushID(i);

    // Row with icon, name, and type badge
    bool selected = (selected_idx == i);
    if (ImGui::Selectable("##row", selected, ImGuiSelectableFlags_SpanAllColumns)) {
        selected_idx = i;
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(items[i].icon);
    ImGui::SameLine();
    ImGui::TextUnformatted(items[i].name.c_str());
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f);
    ImGui::TextDisabled("%s", items[i].type.c_str());

    ImGui::PopID();
}
```

---

## 6. Interaction Design

### 6.1. Feedback — Every Action Must Respond

| User Action | Expected Feedback |
|-------------|-------------------|
| Hover over interactive item | Cursor change, highlight, tooltip |
| Click button | Visual press state (ImGui does this) |
| Complete operation | Status bar update or notification |
| Error occurs | Colored error text, tooltip on disabled item |
| Long operation | Progress indicator or spinner |
| Destructive action | Confirmation dialog |

```cpp
// Tooltip on disabled button explaining WHY it's disabled
ImGui::BeginDisabled(!can_export);
if (ImGui::Button("Export")) { exportDocument(); }
if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    ImGui::SetTooltip("Save the document before exporting");
ImGui::EndDisabled();
```

### 6.2. Confirmation for Destructive Actions

```cpp
// Never delete without confirmation
if (ImGui::Button("Delete All")) {
    ImGui::OpenPopup("ConfirmDelete");
}

if (ImGui::BeginPopupModal("ConfirmDelete", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
    ImGui::TextUnformatted("This will permanently delete all selected items.");
    ImGui::TextDisabled("This action cannot be undone.");
    ImGui::Spacing();

    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    // Danger-styled confirm button
    ImGui::PushStyleColor(ImGuiCol_Button, Theme::Danger);
    if (ImGui::Button("Delete", ImVec2(120, 0))) {
        performDelete();
        ImGui::CloseCurrentPopup();
    }
    ImGui::PopStyleColor();

    ImGui::EndPopup();
}
```

### 6.3. Keyboard Navigation

Professional tools are keyboard-first. ImGui supports this natively — don't break it.

```cpp
// Attach shortcuts to menu items (displays shortcut text automatically)
ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_Z);
if (ImGui::MenuItem("Undo", nullptr, false, can_undo)) undo();

ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_Y);
if (ImGui::MenuItem("Redo", nullptr, false, can_redo)) redo();

// Tab navigation between inputs works automatically
// Don't break it by using NoTabStop unless you have a good reason
```

---

## 7. Coherence Checklist

Run this checklist on every panel you build or review:

### Layout
- [ ] Labels are left-aligned in a fixed-width column
- [ ] Value widgets fill remaining width (`-FLT_MIN`)
- [ ] No hardcoded pixel widths (use proportional or `-FLT_MIN`)
- [ ] `AlignTextToFramePadding()` before every label next to a framed widget
- [ ] Sections separated with `SeparatorText()` or `CollapsingHeader()`
- [ ] Consistent padding (never `WindowPadding = {0,0}` unless viewport/canvas)

### Controls
- [ ] Every interactive widget has a visible or tooltip label
- [ ] Disabled widgets explain WHY via `IsItemHovered(AllowWhenDisabled)` + tooltip
- [ ] Destructive actions use danger-styled buttons and confirmation
- [ ] `IsItemDeactivatedAfterEdit()` for undo boundaries on text inputs
- [ ] `BeginDisabled()` for unavailable actions (never manual gray-out)

### Visual
- [ ] Colors come from theme constants, not hardcoded `ImVec4`
- [ ] Empty states show guidance text (`TextDisabled`)
- [ ] No default ImGui font — custom font loaded (Inter, Roboto, etc.)
- [ ] Icons for toolbar buttons, text labels for important actions
- [ ] Consistent rounding (`WindowRounding`, `FrameRounding`, `TabRounding`)

### Behavior
- [ ] `PushID`/`PopID` in every loop
- [ ] `SetNextWindowSize(size, ImGuiCond_Once)` for every window
- [ ] Scroll in long lists via `BeginChild` or natural window scroll
- [ ] `ListClipper` for lists > 50 items
- [ ] No flickering (initial size set, no layout oscillation)

---

## 8. Common UI Smells — Fix These on Sight

| Smell | Fix |
|-------|-----|
| Raw `DragFloat("##1", &x)` with no label | Add label column via property table |
| Wall of widgets without sections | Add `SeparatorText()` between groups |
| `Text("No items")` for empty state | Center the message, use `TextDisabled`, add guidance |
| Hardcoded `ImVec2(200, 0)` button size | `ImVec2(-FLT_MIN, 0)` or `ImVec2(0, 0)` (auto-size) |
| Missing `AlignTextToFramePadding()` | Labels misaligned with widgets → add the call |
| `PushStyleColor` with raw `ImVec4(r,g,b,a)` | Use `Theme::` constants |
| No tooltip on icon-only buttons | Add `SetItemTooltip()` after every icon button |
| `Checkbox` / `RadioButton` without clear label | Use descriptive label text, not `##hidden` |
| Delete/remove without confirmation | Add confirmation popup for destructive actions |
| Input applies on every keystroke | Use `IsItemDeactivatedAfterEdit()` for commit |
| Panel blank when nothing selected | Add empty state: "Select X to view Y" |
| Inconsistent widget widths in a column | Use `BeginTable` with `WidthFixed` + `WidthStretch` |
| Manual `SameLine(120)` grid alignment | Replace with `BeginTable` |
