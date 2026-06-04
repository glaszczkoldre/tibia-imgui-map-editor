# Dear ImGui Decision Guide & Advanced Techniques

> "When to use X?", "Why X is better than Y?", performance patterns, and 1.90+ new features.
> Sources: Official imgui.h (v1.92.9), imgui_demo.cpp, GitHub issues, FAQ.

---

## A. Decision Scenarios — "When to Use X?"

### A1. BeginChild vs BeginGroup vs Indent

| Feature | `BeginChild` | `BeginGroup` | `Indent` |
|---------|-------------|-------------|----------|
| Scrolling | ✅ Independent scrollbars | ❌ | ❌ |
| Own coordinate system | ✅ Resets cursor | ❌ Locks X start pos | ❌ Shifts X only |
| IsItemHovered on whole block | ❌ (it's a window) | ✅ Captures bounding box | ❌ |
| Overhead | Higher (sub-window) | Very low | Minimal |

**Decision tree:**
```
Need independent scrolling?
  → YES → BeginChild()
  → NO → Need block as single item (SameLine, IsItemHovered on group)?
           → YES → BeginGroup/EndGroup
           → NO → Just visual nesting?
                    → YES → Indent/Unindent
                    → NO → Plain layout (SameLine, Dummy, Spacing)
```

```cpp
// BeginChild — scrollable panel
ImGui::BeginChild("Scroll", ImVec2(0, 200), ImGuiChildFlags_Borders);
for (int i = 0; i < 100; i++) ImGui::Text("Line %d", i);
ImGui::EndChild();

// BeginGroup — treat block as single item
ImGui::BeginGroup();
ImGui::Text("Name:"); ImGui::Text("HP: 100"); ImGui::Text("MP: 50");
ImGui::EndGroup();
ImGui::SameLine();
ImGui::Button("Edit");  // Appears to the right of the entire group

// Indent — visual nesting
ImGui::Text("Settings:");
ImGui::Indent();
ImGui::Checkbox("Shadows", &shadows);
ImGui::Unindent();
```

### A2. BeginTable vs Manual Layout

**Always prefer `BeginTable`** for aligned multi-column content.

| Scenario | Use |
|----------|-----|
| Label + value pairs (settings) | `BeginTable` with 2 columns |
| Data grid / spreadsheet | `BeginTable` with sorting, headers |
| Simple "button A, button B" on same line | `SameLine()` |
| Property editor (label + widget) | `BeginTable` with WidthFixed + WidthStretch |

### A3. Selectable vs Button vs TreeNode

| Widget | State | Best For |
|--------|-------|----------|
| **Button** | Transient click | Actions: "Save", "Delete", toolbar |
| **Selectable** | External bool | Item lists, menus, palette picks |
| **TreeNode** | Internal open/close | File trees, object hierarchies |

```cpp
// TreeNode + Selectable for selectable hierarchy
ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
if (node.is_leaf) flags |= ImGuiTreeNodeFlags_Leaf;
if (node.is_selected) flags |= ImGuiTreeNodeFlags_Selected;
bool open = ImGui::TreeNodeEx(node.name.c_str(), flags);
if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    selected_node = &node;
if (open) { /* children */ ImGui::TreePop(); }
```

### A4. InputText vs InputTextMultiline

| Use Case | Widget |
|----------|--------|
| Single-line entry (name, search, path) | `InputText` or `InputTextWithHint` |
| Multi-line content (script, notes) | `InputTextMultiline` |
| Single-line with placeholder | `InputTextWithHint` |

```cpp
// Single-line with hint
ImGui::InputTextWithHint("##Search", "Search items...", buf, sizeof(buf));

// Multi-line with std::string (requires imgui_stdlib.h)
#include "misc/cpp/imgui_stdlib.h"
ImGui::InputTextMultiline("##Script", &script_text, ImVec2(-FLT_MIN, 300));
```

### A5. OpenPopup vs BeginPopupContextItem

| Pattern | When |
|---------|------|
| `BeginPopupContextItem()` | Right-click context menu on previous item (automatic) |
| `OpenPopup()` + `BeginPopup()` | Custom trigger (left-click, hotkey, game event) |

```cpp
// AUTOMATIC right-click context menu
ImGui::Selectable("My Item");
if (ImGui::BeginPopupContextItem()) {
    if (ImGui::MenuItem("Delete")) { /* ... */ }
    ImGui::EndPopup();
}

// MANUAL custom trigger
if (ImGui::Button("Options"))
    ImGui::OpenPopup("OptionsPopup");  // Call ONCE on trigger!
if (ImGui::BeginPopup("OptionsPopup")) {
    ImGui::MenuItem("Option A");
    ImGui::EndPopup();
}
```

> **Critical:** `OpenPopup` and `BeginPopup` must share the same ID stack level.

### A6. PushID: int vs string vs pointer

| Method | Best For | Stability |
|--------|----------|-----------|
| `PushID(int)` | Loop iteration with index | Stable if list order is stable |
| `PushID(const char*)` | Named semantic scopes | Always stable |
| `PushID(const void*)` | Object instances | Stable if objects persist in memory |

### A7. Columns (Legacy) vs Tables

**Never use legacy `Columns()` for new code.** Tables supersede it completely.

| Feature | Legacy Columns | Tables API |
|---------|---------------|------------|
| Status | **Obsolete** | **Recommended** |
| Sorting | ❌ | ✅ Built-in |
| Resizing | Limited, buggy | ✅ Full support |
| Reordering | ❌ | ✅ Built-in |
| Scroll freezing | ❌ | ✅ `TableSetupScrollFreeze` |
| Works with Clipper | ❌ | ✅ |

### A8. SetNextWindowSize vs SetNextWindowSizeConstraints

| Function | Purpose | User can resize? |
|----------|---------|-----------------|
| `SetNextWindowSize(size, cond)` | Set exact size | Only if `cond` allows |
| `SetNextWindowSizeConstraints(min, max)` | Set resize boundaries | Yes, within bounds |

```cpp
// Initial size, user resizes freely afterward
ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);

// Constrain resize range
ImGui::SetNextWindowSizeConstraints(ImVec2(200, 150), ImVec2(800, 600));

// Force aspect ratio via callback
ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(FLT_MAX, FLT_MAX),
    [](ImGuiSizeCallbackData* data) {
        data->DesiredSize.y = data->DesiredSize.x * 9.0f / 16.0f;
    });
```

### A9. IsItemHovered vs IsWindowHovered

| Function | Scope | Use |
|----------|-------|-----|
| `IsItemHovered()` | Last submitted widget | Tooltips, per-widget feedback |
| `IsWindowHovered()` | Current window area | Panel detection, input routing |

```cpp
// Per-item tooltip (modern 1.90+ helper)
ImGui::Button("Hover me");
ImGui::SetItemTooltip("This button does something");  // Handles delay automatically

// Window-level check
if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
    handle_scroll_input();

// App-level: should ImGui capture mouse?
if (!ImGui::GetIO().WantCaptureMouse)
    game_handle_mouse();  // DON'T use IsWindowHovered for this!
```

### A10. Docking vs Child Windows for Panels

| Feature | Docking | Child Windows |
|---------|---------|---------------|
| User rearrangeable | ✅ Drag, tab, split | ❌ |
| Layout persistence | ✅ Auto (.ini) | Manual |
| Best for | IDE/editor layouts | Fixed sub-regions |

**Rule:** Multiple panels (viewport, properties, palette) → Docking.
Fixed sub-sections within a single panel → `BeginChild`.

---

## B. Comparison Scenarios — "Why X is Better Than Y"

### B1. Tables vs Legacy Columns

Tables are better in **every dimension**: features, performance, maintenance.
Legacy Columns are marked obsolete and will never receive fixes.

### B2. BeginDisabled vs Manual Graying Out

```cpp
// ❌ BAD — manual graying (incomplete, still clickable!)
if (disabled) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
if (ImGui::Button("Save") && !disabled) { /* ... */ }  // STILL CLICKABLE!
if (disabled) ImGui::PopStyleColor();

// ✅ GOOD — BeginDisabled (correct, complete)
ImGui::BeginDisabled(disabled);
if (ImGui::Button("Save")) { /* guaranteed not called when disabled */ }
ImGui::EndDisabled();
```

`BeginDisabled` is better because:
- Applies `style.DisabledAlpha` to ALL child widgets automatically
- **Blocks ALL input** (click, keyboard, shortcuts)
- Works with `SetNextItemShortcut` (auto-suppressed when disabled)
- Tooltips still work with `ImGuiHoveredFlags_AllowWhenDisabled`
- Nestable (cannot re-enable within disabled scope)

### B3. Stack-based ID (PushID) vs String Hashing (##)

| Approach | Best For |
|----------|----------|
| `PushID(i)` / `PopID()` | Loops, dynamic lists |
| `"Label##UniqueID"` | Static UI with duplicate visible labels |
| `"##HiddenID"` | Widgets with no visible label |

### B4. Text vs TextUnformatted

`TextUnformatted` is **always better** for static strings:
- **Faster**: Skips printf parsing
- **Safer**: No crash if string contains `%`
- **Flexible**: Accepts `(begin, end)` pointers

```cpp
// ❌ CRASH if user_string contains "%s"
ImGui::Text(user_string.c_str());

// ✅ Fast, safe
ImGui::TextUnformatted(user_string.c_str());

// ✅ For formatted output — explicit format
ImGui::Text("HP: %d / %d", current_hp, max_hp);

// ✅ For pre-formatted std::string
auto label = std::format("Count: {}", count);
ImGui::TextUnformatted(label.c_str());
```

### B5. ListClipper vs Manual Visibility Checks

`ImGuiListClipper` is **dramatically better** for large uniform-height lists:
- O(1) visible range calculation per frame
- Only calls code for visible items
- Works with Tables seamlessly

Manual checks only for: variable-height items where clipper can't work.

---

## C. Performance Patterns

### C1. ListClipper with Tables — The Gold Standard

```cpp
if (ImGui::BeginTable("BigTable", 4,
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable)) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 100.0f);
    ImGui::TableHeadersRow();

    if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
        if (specs->SpecsDirty) { SortData(data, specs); specs->SpecsDirty = false; }
    }

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(data.size()));
    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
            auto& item = data[row];
            ImGui::TableNextRow();
            if (ImGui::TableSetColumnIndex(0)) ImGui::Text("%d", item.id);
            if (ImGui::TableSetColumnIndex(1)) ImGui::TextUnformatted(item.name.c_str());
            if (ImGui::TableSetColumnIndex(2)) ImGui::TextUnformatted(item.type.c_str());
            if (ImGui::TableSetColumnIndex(3)) ImGui::Text("%.2f", item.value);
        }
    }
    ImGui::EndTable();
}
```

### C2. Minimizing String Allocation in Hot Paths

```cpp
// ❌ BAD — allocates every frame for every item
for (int i = 0; i < 10000; i++) {
    std::string label = "Item " + std::to_string(i);
    ImGui::TextUnformatted(label.c_str());
}

// ✅ GOOD — ImGui's built-in printf, no heap alloc
for (int i = 0; i < 10000; i++) {
    ImGui::Text("Item %d", i);
}
```

### C3. Query State Once, Not Per-Widget

```cpp
// ❌ BAD — redundant per-widget query
for (int i = 0; i < 1000; i++) {
    if (ImGui::IsWindowFocused()) ImGui::Text("Item %d", i);
}

// ✅ GOOD — query once, reuse
bool focused = ImGui::IsWindowFocused();
for (int i = 0; i < 1000; i++) {
    if (focused) ImGui::Text("Item %d", i);
}
```

### C4. Check TableSetColumnIndex Return Value

```cpp
// ✅ Skip invisible column content
if (ImGui::TableSetColumnIndex(0)) ImGui::Text("ID: %d", id);
if (ImGui::TableSetColumnIndex(1)) ImGui::TextUnformatted(name.c_str());
```

### C5. Performance Rules Summary

1. **Test in Release mode** — Debug C++ can be 10-50x slower
2. **Use ListClipper** for any list > ~50 items
3. **Use `TextUnformatted`** for static/pre-formatted strings
4. **Check `TableSetColumnIndex()` return** to skip invisible columns
5. **Don't allocate strings** in UI loops — use `Text()` format or pre-compute
6. **Cache data** that doesn't change between frames (sorted arrays, formatted strings)

---

## D. ImGui 1.90+ New Features

### D1. Multi-Select API (v1.91.0)

Storage-agnostic multi-selection: Ctrl+Click, Shift+Click, box selection.

```cpp
static ImGuiSelectionBasicStorage selection;

ImGuiMultiSelectFlags ms_flags =
    ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d;

ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(ms_flags, selection.Size, items_count);
selection.ApplyRequests(ms_io);

ImGuiListClipper clipper;
clipper.Begin(items_count);
while (clipper.Step()) {
    for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++) {
        ImGui::SetNextItemSelectionUserData(n);
        bool is_selected = selection.Contains((ImGuiID)n);
        ImGui::PushID(n);
        ImGui::Selectable(items[n].name, is_selected);
        ImGui::PopID();
    }
}

ms_io = ImGui::EndMultiSelect();
selection.ApplyRequests(ms_io);
```

### D2. Shortcut Routing System (v1.90+)

```cpp
// Global shortcut — works regardless of focused window
if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal))
    SaveFile();

// Focused shortcut (default)
if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z))
    Undo();

// Attach shortcut to menu item (displays shortcut text)
ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_N);
if (ImGui::MenuItem("New")) CreateNew();

// Auto-suppressed when disabled!
ImGui::BeginDisabled(!can_save);
ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_S);
if (ImGui::MenuItem("Save")) Save();
ImGui::EndDisabled();
```

### D3. ImGuiChildFlags (v1.90)

```cpp
// Border (replaces bool parameter)
ImGui::BeginChild("Panel", size, ImGuiChildFlags_Borders);

// Auto-resize to content
ImGui::BeginChild("Auto", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);

// Frame-style child (looks like ListBox)
ImGui::BeginChild("Framed", ImVec2(0, 200), ImGuiChildFlags_FrameStyle);
```

### D4. Tooltip Helpers (v1.90+)

```cpp
// Simple (handles hover delay automatically)
ImGui::Button("Info");
ImGui::SetItemTooltip("Help text");

// Rich tooltip
ImGui::Button("Info");
if (ImGui::BeginItemTooltip()) {
    ImGui::Text("Detailed:");
    ImGui::BulletText("Point 1");
    ImGui::EndTooltip();
}
```

### D5. Debug Tools

```cpp
ImGui::ShowIDStackToolWindow();                       // Hover widget → see full ID stack
ImGui::ShowMetricsWindow();                           // Draw commands, windows, internal state
ImGui::GetIO().ConfigDebugHighlightIdConflicts = true; // Visual ID conflict detection
ImGui::DebugStartItemPicker();                        // Click widget → break at emitting code
```

### D6. Font System (v1.92)

```cpp
ImGui::PushFont(my_font, 24.0f);     // Render at 24px
ImGui::Text("Large text");
ImGui::PopFont();

ImGui::PushFont(nullptr, 12.0f);     // Keep current font, change size
ImGui::Text("Small text");
ImGui::PopFont();

// CORRECT: Use style.FontSizeBase for relative sizing
ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 2.0f);
// WRONG: GetFontSize() would double-apply scaling
```

### D7. Other 1.90+ Additions

| Feature | Version | Notes |
|---------|---------|-------|
| `SetNextItemShortcut()` | 1.90+ | Attach keyboard shortcut to next item |
| `BeginItemTooltip()` / `SetItemTooltip()` | 1.90+ | Rich/simple tooltip helpers |
| `ImGuiChildFlags_AutoResizeX/Y` | 1.90 | Auto-size child windows |
| `ImGuiChildFlags_FrameStyle` | 1.90 | Child that looks like a ListBox |
| `TableAngledHeadersRow()` | 1.90+ | Angled column headers |
| `TextLink()` / `TextLinkOpenURL()` | 1.91+ | Hyperlink-style text |
| `PushStyleVarX()` / `PushStyleVarY()` | 1.91+ | Modify single axis of ImVec2 vars |
| `ConfigDebugHighlightIdConflicts` | 1.91+ | Visual ID conflict detection |

---

## E. Common Anti-Patterns

### E1. Missing PushID in Loops

```cpp
// ❌ All buttons share same ID
for (auto& item : items) if (ImGui::Button("Edit")) { /* always first item */ }

// ✅ Unique ID per iteration
for (int i = 0; i < items.size(); i++) {
    ImGui::PushID(i);
    if (ImGui::Button("Edit")) { edit(items[i]); }
    ImGui::PopID();
}
```

### E2. Calling OpenPopup Every Frame

```cpp
// ❌ Popup flickers or never appears
if (show_popup) ImGui::OpenPopup("MyPopup");  // Every frame!

// ✅ Call once on trigger
if (just_triggered) ImGui::OpenPopup("MyPopup");
```

### E3. Mismatched Begin/End

```cpp
// ❌ Conditional End — stack corruption if condition changes
if (cond) ImGui::BeginChild("X");
/* ... */
if (cond) ImGui::EndChild();

// ✅ Always match
ImGui::BeginChild("X");
if (cond) { /* content */ }
ImGui::EndChild();  // Always called
```

### E4. Text() with User Strings

```cpp
// ❌ CRASH if user_input contains "%"
ImGui::Text(user_input.c_str());

// ✅ Safe
ImGui::TextUnformatted(user_input.c_str());
// or
ImGui::Text("%s", user_input.c_str());
```

---

## F. Drag and Drop Patterns

```cpp
// SOURCE
if (ImGui::Selectable(item.name.c_str(), item.selected)) { /* select */ }
if (ImGui::BeginDragDropSource()) {
    int id = item.id;
    ImGui::SetDragDropPayload("ITEM_DND", &id, sizeof(int));
    ImGui::Text("Moving: %s", item.name.c_str());
    ImGui::EndDragDropSource();
}

// TARGET
if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ITEM_DND")) {
        int dropped_id = *(const int*)payload->Data;
        HandleDrop(dropped_id, target_slot);
    }
    ImGui::EndDragDropTarget();
}
```

**Rules:**
- Payload data is memcpy'd — use POD types or IDs, not pointers to temporaries
- One item can be both source and target
- Use `ImGuiDragDropFlags_AcceptBeforeDelivery` to preview before release
