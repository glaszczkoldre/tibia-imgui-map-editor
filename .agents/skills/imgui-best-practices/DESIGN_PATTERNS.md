# Dear ImGui Design Patterns Reference

> Best practices for coherent UI design in complex C++ docking-based editors.
> Sources: Official ImGui FAQ, Omar Cornut's talks, imgui_demo.cpp, community patterns.

---

## A. Good Design vs Bad Design

### A1. Window Management

| Aspect | ❌ Bad | ✅ Good |
|--------|-------|---------|
| Begin/End pairing | Skip `End()` when `Begin()` returns false | Always call `End()` regardless of return value |
| Initial window size | No size hint → flicker on first frame | `SetNextWindowSize(size, ImGuiCond_Once)` |
| Window positioning | `SetNextWindowPos()` every frame | `SetNextWindowPos(pos, ImGuiCond_Once)` or let .ini handle it |
| Content when collapsed | Submit all widgets even when collapsed | Check `Begin()` return, skip content if false |
| Window naming | Dynamic titles that change every frame | `###` for stable IDs with dynamic display: `"FPS: 60###PerfWin"` |

```cpp
// ❌ BAD — Missing End() when collapsed, no size hint
if (ImGui::Begin("Properties")) {
    ImGui::Text("Hello");
    ImGui::End();  // SKIPPED when Begin() returns false!
}

// ✅ GOOD — Always End(), initial size, skip content when collapsed
ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);
if (ImGui::Begin("Properties")) {
    ImGui::Text("Hello");  // Only when visible
}
ImGui::End();  // ALWAYS called

// ❌ BAD — Window ID changes every frame → layout resets
auto title = std::format("FPS: {:.1f}", fps);
ImGui::Begin(title.c_str());

// ✅ GOOD — Display text changes but ID is stable
auto title = std::format("FPS: {:.1f}###PerformanceWindow", fps);
ImGui::Begin(title.c_str());
```

### A2. Widget Layout

| Aspect | ❌ Bad | ✅ Good |
|--------|-------|---------|
| Property grids | Manual `SameLine()` + hardcoded offsets | `BeginTable()` with fixed + stretch columns |
| Alignment | `SetCursorPosX()` with magic numbers | Tables or `SameLine(offset)` with computed values |
| Spacing | Hardcoded pixel values | `ImGui::GetStyle().ItemSpacing` and style variables |
| Full-width widgets | Fixed pixel widths | `SetNextItemWidth(-FLT_MIN)` for full available width |
| Scrolling regions | Giant window with everything | `BeginChild()` for scrollable sub-regions |

```cpp
// ❌ BAD — Fragile manual layout, magic numbers
ImGui::Text("Position X");
ImGui::SameLine(120);
ImGui::SetNextItemWidth(100);
ImGui::DragFloat("##PosX", &pos.x);

// ✅ GOOD — Table-based property layout
if (ImGui::BeginTable("Properties", 2, ImGuiTableFlags_BordersInnerV)) {
    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();
    ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Position X");
    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::DragFloat("##PosX", &pos.x);

    ImGui::EndTable();
}
```

### A3. ID System

| Aspect | ❌ Bad | ✅ Good |
|--------|-------|---------|
| Loop items | Same label for all items | `PushID(index)` / `PopID()` per iteration |
| Duplicate labels | Two buttons both "Delete" | `"Delete##item1"`, `"Delete##item2"` |
| Empty labels | `Button("")` | `Button("##hidden_btn")` |
| Pointer-based IDs | Loop index (unstable on reorder) | `PushID(item_pointer)` for stable identity |
| Debugging | Guessing which ID conflicts | `ShowMetricsWindow()` → ID Stack Tool |

```cpp
// ❌ BAD — All "Delete" buttons share same ID, only first works
for (auto& item : items) {
    ImGui::Text("%s", item.name.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Delete")) { removeItem(item); }  // ID collision!
}

// ✅ GOOD — Unique ID scope per iteration
for (int i = 0; i < items.size(); ++i) {
    ImGui::PushID(i);
    ImGui::Text("%s", items[i].name.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Delete")) { removeItem(items[i]); }
    ImGui::PopID();
}
```

**Auto-ID scopes (no manual PushID needed inside):**
- `Begin()` / `End()` (windows)
- `TreeNode()` / `TreePop()`
- `BeginTable()` + `TableNextColumn()`

### A4. State Management

| Aspect | ❌ Bad | ✅ Good |
|--------|-------|---------|
| Business logic in UI | Complex logic inside `if (Button(...))` | UI calls controller/service methods |
| Static locals | `static float value = 0;` in render | State in dedicated model/service classes |
| Global state | Global variables for UI state | Inject dependencies via constructor |
| Expensive operations | File I/O in button callback | Async task + poll in main loop |
| Data editing | Update model on every keystroke | `IsItemDeactivatedAfterEdit()` for commit-on-finish |

```cpp
// ❌ BAD — Business logic in UI, fires on every keystroke
static char buf[256];
if (ImGui::InputText("Name", buf, sizeof(buf))) {
    entity->setName(buf);           // fires on EVERY character
    history->pushUndo(/*...*/);     // hundreds of undo entries!
}

// ✅ GOOD — Commit only when editing finishes
static char buf[256];
ImGui::InputText("Name", buf, sizeof(buf));
if (ImGui::IsItemDeactivatedAfterEdit()) {
    entity->setName(buf);           // fires ONCE
    history->pushUndo(/*...*/);     // single clean undo entry
}

// ❌ BAD — Domain manipulation from UI
void DrawPanel() {
    if (ImGui::Button("Apply")) {
        document->modify(selection);  // Direct domain access from UI!
    }
}

// ✅ GOOD — Delegate to service
void Panel::draw() {
    if (ImGui::Button("Apply")) {
        controller_.applyAction(selection_);  // UI talks to service layer
    }
}
```

### A5. Color / Style Handling

| Aspect | ❌ Bad | ✅ Good |
|--------|-------|---------|
| Hardcoded colors | `ImVec4(1,0,0,1)` scattered in code | Central theme with named constants |
| Unbalanced Push/Pop | Forgetting `PopStyleColor()` on early return | RAII scope guards |
| Per-widget styling | Manual `PushStyleColor` everywhere | Wrapper functions that encapsulate style |
| Magic numbers | `style.FrameRounding = 5.0f;` in random places | Single `ApplyTheme()` at startup |

```cpp
// ❌ BAD — Colors everywhere, easy to leak Push/Pop
ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
if (ImGui::Button("Delete")) { /* ... */ }
ImGui::PopStyleColor(2);

// ✅ GOOD — Centralized theme + wrapper
namespace Theme {
    inline constexpr ImVec4 Danger        = {0.80f, 0.20f, 0.20f, 1.00f};
    inline constexpr ImVec4 DangerHovered = {0.90f, 0.30f, 0.30f, 1.00f};
    inline constexpr ImVec4 DangerActive  = {0.70f, 0.15f, 0.15f, 1.00f};
}

bool DangerButton(const char* label) {
    ImGui::PushStyleColor(ImGuiCol_Button, Theme::Danger);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::DangerHovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::DangerActive);
    bool clicked = ImGui::Button(label);
    ImGui::PopStyleColor(3);
    return clicked;
}
```

**RAII Scope Guard Pattern:**
```cpp
class StyleColorGuard {
public:
    StyleColorGuard(ImGuiCol idx, const ImVec4& col) { ImGui::PushStyleColor(idx, col); }
    ~StyleColorGuard() { ImGui::PopStyleColor(); }
    StyleColorGuard(const StyleColorGuard&) = delete;
    StyleColorGuard& operator=(const StyleColorGuard&) = delete;
};

// Usage — exception-safe, early-return safe
void DrawWindow() {
    StyleColorGuard bg(ImGuiCol_WindowBg, Theme::PanelBackground);
    // ... if we return early, style is still popped
}
```

### A6. Docking Workspace

| Aspect | ❌ Bad | ✅ Good |
|--------|-------|---------|
| No dockspace | Floating windows only | `DockSpaceOverViewport()` as root |
| No default layout | Random window placement | `DockBuilder` API for first-run layout |
| DockBuilder every frame | Rebuilds layout constantly | `first_time` flag + .ini persistence |
| Dynamic window names | Titles change → DockBuilder mismatch | Stable names matching `DockBuilderDockWindow()` |
| No reset option | User can't restore defaults | "Reset Layout" that re-runs DockBuilder |

---

## B. "Instead of X, Use Y"

### B1. Window & Widget API

| Instead of... | Use... | Why |
|--------------|--------|-----|
| `Begin()`/skip `End()` based on return | Always call `End()` after `Begin()` | Begin/End must always be paired |
| `Begin("Window")` with no size | `SetNextWindowSize(size, ImGuiCond_Once)` | Prevents first-frame flicker |
| `SetNextWindowPos()` every frame | `SetNextWindowPos(pos, ImGuiCond_Once)` | Lets user reposition, respects .ini |
| Hardcoded `ImVec4(r,g,b,a)` | `GetStyleColorVec4(ImGuiCol_*)` | Respects theme changes |
| `Text()` for static/user strings | `TextUnformatted()` | No printf parsing, no `%` crash |
| Manual `SameLine()` grids | `BeginTable()` | Proper alignment, resizable columns |
| Legacy `Columns()` | `BeginTable()` | Columns is obsolete, Tables is maintained |

### B2. ID Management

| Instead of... | Use... | Why |
|--------------|--------|-----|
| Same label for different widgets | `"Label##UniqueID"` | Unique ID, same visual label |
| Manual string suffix | `PushID(int)` in loops | Cleaner, no allocation |
| `PushID(loop_index)` for reorderable lists | `PushID(item_pointer)` | Stable when list reorders |
| `Button("")` | `Button("##unique_name")` | Empty label = parent ID = collision |
| Dynamic title for persistent window | `"Dynamic###StableID"` | `###` makes everything before it display-only |

### B3. State & Logic

| Instead of... | Use... | Why |
|--------------|--------|-----|
| `static` variables in UI functions | State in model/service classes | Testable, debuggable |
| Update model on every keystroke | `IsItemDeactivatedAfterEdit()` | Single commit, clean undo |
| Blocking I/O in button callback | `std::async` + poll in main loop | UI stays responsive |
| Global singletons | Constructor injection | Explicit deps, testable |
| Business logic in `if (Button())` | `controller.doAction()` | Separation of concerns |

### B4. Style & Theming

| Instead of... | Use... | Why |
|--------------|--------|-----|
| `PushStyleColor()` without matching Pop | RAII scope guard class | Exception-safe, early-return safe |
| Per-widget inline color pushes | Wrapper functions (`DangerButton()`) | Consistent, DRY, centralized |
| Default ImGui font | `AddFontFromFileTTF()` with Inter/Roboto | Professional appearance |
| Text-only buttons | Icon font (FontAwesome) | Reduces clutter, modern feel |
| Trial-and-error style tweaking | `ShowStyleEditor()` at runtime | Real-time preview |

### B5. Modern API Replacements

| Instead of... | Use... | Version |
|--------------|--------|---------|
| `BeginChild("id", size, true)` (border bool) | `BeginChild("id", size, ImGuiChildFlags_Borders)` | 1.90+ |
| Manual gray-out with PushStyleColor | `BeginDisabled()` / `EndDisabled()` | 1.90+ |
| `IsItemHovered() + SetTooltip()` | `SetItemTooltip()` | 1.90+ |
| `IsItemHovered() + BeginTooltip()` | `BeginItemTooltip()` | 1.90+ |
| `io.KeyMap[]`, `io.KeysDown[]` | `IsKeyPressed(ImGuiKey_*)` | 1.90+ |
| Manual shortcut checking | `Shortcut(ImGuiMod_Ctrl \| ImGuiKey_S)` | 1.90+ |
| `Image()` with tint/border params | `ImageWithBg()` | 1.91.9+ |

---

## C. Common Struggles and Solutions

### C1. Flickering Windows

**Symptom:** Window appears at wrong size for one frame, then snaps.

**Cause:** No initial size hint → ImGui auto-sizes on first frame.

```cpp
// ✅ FIX — Set initial size only on first appearance
ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
if (ImGui::Begin("My Window", &open)) { /* ... */ }
ImGui::End();

// ✅ FIX — For auto-resize that oscillates, force scrollbar
ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysVerticalScrollbar;
```

### C2. ID Conflicts

**Symptom:** Clicking one widget activates another. Tree nodes expand in sync.

```cpp
// ✅ DIAGNOSIS — Add to your debug menu:
ImGui::ShowMetricsWindow();
// Metrics → Tools → ID Stack Tool → hover problem widget

// ✅ Enable visual highlighting:
ImGui::GetIO().ConfigDebugHighlightIdConflicts = true;
```

### C3. Layout Issues in Complex Panels

```cpp
// Full-width input field
ImGui::SetNextItemWidth(-FLT_MIN);
ImGui::InputText("##search", buf, sizeof(buf));

// Percentage-width (50% of available)
ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
ImGui::DragFloat("##value", &val);

// Scrollable sub-region with fixed header
ImGui::Text("Fixed Header");
ImGui::Separator();
ImGui::BeginChild("ScrollRegion", ImVec2(0, 0), ImGuiChildFlags_Borders);
for (int i = 0; i < 100; ++i) ImGui::Text("Item %d", i);
ImGui::EndChild();

// Side-by-side resizable panels
if (ImGui::BeginTable("Split", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
    ImGui::TableSetupColumn("Left",  ImGuiTableColumnFlags_WidthStretch, 0.6f);
    ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthStretch, 0.4f);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    // Left panel...
    ImGui::TableNextColumn();
    // Right panel...
    ImGui::EndTable();
}
```

### C4. Docking Configuration Problems

**Checklist when docking doesn't work:**

```cpp
// 1. Enabled docking?
io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

// 2. Created a dockspace?
ImGui::DockSpaceOverViewport(ImGui::GetID("MyDockSpace"), ImGui::GetMainViewport());

// 3. Window names EXACTLY match DockBuilder calls?
ImGui::DockBuilderDockWindow("Properties", dock_right);  // Must match Begin("Properties")

// 4. DockBuilder runs only ONCE?
static bool init = true;
if (init) { init = false; /* DockBuilder setup */ }

// 5. .ini being saved? Check io.IniFilename != nullptr

// 6. Reset layout feature:
void ResetLayout() {
    ImGui::ClearIniSettings();
    layout_initialized_ = false;  // Re-run DockBuilder next frame
}
```

### C5. State Synchronization Across Docked Windows

**Pattern: Shared state via controller, not cross-window references.**

```cpp
// Controller mediates between panels
class SelectionController {
public:
    void select(EntityID id) {
        state_.selected_id = id;
        on_selection_changed_.fire(state_);
    }
    const SelectionState& getState() const { return state_; }
private:
    SelectionState state_;
    Callback<const SelectionState&> on_selection_changed_;
};

// Panel A WRITES selection
void ViewportPanel::draw() {
    if (clicked_entity) selection_controller_.select(entity_id);
}

// Panel B READS selection
void PropertiesPanel::draw() {
    const auto& sel = selection_controller_.getState();
    if (!sel.has_selection) { ImGui::TextDisabled("No selection"); return; }
    // Show properties...
}
```

---

## D. Docking Workspace Patterns

### D1. DockBuilder Layout Pattern

```cpp
void BuildDefaultLayout(ImGuiID dockspace_id, ImVec2 size) {
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, size);

    //  ┌──────────────────┬──────────────────┐
    //  │                  │  Panel A / B      │
    //  │   Central        │  (tabbed)         │
    //  │                  ├──────────────────│
    //  │                  │  Panel C          │
    //  ├──────────────────┴──────────────────│
    //  │  Panel D / Panel E (tabbed)          │
    //  └─────────────────────────────────────┘

    ImGuiID main = dockspace_id;
    ImGuiID right  = ImGui::DockBuilderSplitNode(main, ImGuiDir_Right, 0.25f, nullptr, &main);
    ImGuiID bottom = ImGui::DockBuilderSplitNode(main, ImGuiDir_Down,  0.25f, nullptr, &main);
    ImGuiID right_bottom = ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.4f, nullptr, &right);

    ImGui::DockBuilderDockWindow("Central",     main);
    ImGui::DockBuilderDockWindow("Panel A",     right);
    ImGui::DockBuilderDockWindow("Panel B",     right);         // Tabbed with Panel A
    ImGui::DockBuilderDockWindow("Panel C",     right_bottom);
    ImGui::DockBuilderDockWindow("Panel D",     bottom);
    ImGui::DockBuilderDockWindow("Panel E",     bottom);        // Tabbed with Panel D

    ImGui::DockBuilderFinish(dockspace_id);
}
```

### D2. Save/Load Workspace Configurations

```cpp
class WorkspaceManager {
public:
    void saveWorkspace(const std::string& name) {
        size_t size = 0;
        const char* data = ImGui::SaveIniSettingsToMemory(&size);
        workspaces_[name] = std::string(data, size);
    }
    void loadWorkspace(const std::string& name) {
        if (auto it = workspaces_.find(name); it != workspaces_.end())
            ImGui::LoadIniSettingsFromMemory(it->second.c_str(), it->second.size());
    }
    void init() { ImGui::GetIO().IniFilename = nullptr; }  // Manual .ini management
private:
    std::unordered_map<std::string, std::string> workspaces_;
};
```

---

## E. Professional Theming

### E1. Dark Theme Template

```cpp
void ApplyEditorDarkTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    // Structure
    style.WindowRounding = 4.0f;  style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;    style.TabRounding = 3.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 4);

    // Backgrounds
    c[ImGuiCol_WindowBg]   = {0.12f, 0.12f, 0.12f, 1.00f};
    c[ImGuiCol_PopupBg]    = {0.10f, 0.10f, 0.10f, 0.98f};
    c[ImGuiCol_FrameBg]    = {0.18f, 0.18f, 0.18f, 1.00f};
    c[ImGuiCol_FrameBgHovered] = {0.24f, 0.24f, 0.24f, 1.00f};
    c[ImGuiCol_FrameBgActive]  = {0.28f, 0.28f, 0.28f, 1.00f};

    // Title / Tabs
    c[ImGuiCol_TitleBg]       = {0.08f, 0.08f, 0.08f, 1.00f};
    c[ImGuiCol_TitleBgActive] = {0.12f, 0.12f, 0.12f, 1.00f};
    c[ImGuiCol_Tab]           = {0.14f, 0.14f, 0.14f, 1.00f};
    c[ImGuiCol_TabHovered]    = {0.26f, 0.59f, 0.98f, 0.60f};
    c[ImGuiCol_TabSelected]   = {0.20f, 0.20f, 0.20f, 1.00f};

    // Accent (blue)
    c[ImGuiCol_Button]        = {0.22f, 0.22f, 0.22f, 1.00f};
    c[ImGuiCol_ButtonHovered] = {0.26f, 0.59f, 0.98f, 0.80f};
    c[ImGuiCol_ButtonActive]  = {0.26f, 0.59f, 0.98f, 1.00f};
    c[ImGuiCol_Header]        = {0.22f, 0.22f, 0.22f, 1.00f};
    c[ImGuiCol_HeaderHovered] = {0.26f, 0.59f, 0.98f, 0.60f};
    c[ImGuiCol_HeaderActive]  = {0.26f, 0.59f, 0.98f, 0.80f};

    // Docking
    c[ImGuiCol_DockingPreview] = {0.26f, 0.59f, 0.98f, 0.50f};
    c[ImGuiCol_DockingEmptyBg] = {0.08f, 0.08f, 0.08f, 1.00f};
}
```

### E2. Font Setup with Icons

```cpp
void SetupFonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("fonts/Inter-Regular.ttf", 15.0f);

    // Merge icon font
    static const ImWchar icon_ranges[] = { 0xf000, 0xf8ff, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = 16.0f;
    io.Fonts->AddFontFromFileTTF("fonts/fa-solid-900.ttf", 14.0f, &icons_config, icon_ranges);
    io.Fonts->Build();
}
```

---

## F. Widget Abstraction Patterns

### F1. Property Table Wrappers

```cpp
namespace UIWidgets {

bool BeginPropertyTable(const char* str_id, float label_width = 120.0f) {
    if (!ImGui::BeginTable(str_id, 2, ImGuiTableFlags_BordersInnerV))
        return false;
    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, label_width);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    return true;
}

void EndPropertyTable() { ImGui::EndTable(); }

void PropertyLabel(const char* label) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
}

bool PropertyDragFloat(const char* label, float* v, float speed = 0.1f) {
    PropertyLabel(label);
    ImGui::PushID(label);
    bool changed = ImGui::DragFloat("##v", v, speed);
    ImGui::PopID();
    return changed;
}

bool PropertyCheckbox(const char* label, bool* v) {
    PropertyLabel(label);
    ImGui::PushID(label);
    bool changed = ImGui::Checkbox("##v", v);
    ImGui::PopID();
    return changed;
}

void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

} // namespace UIWidgets

// Usage:
if (UIWidgets::BeginPropertyTable("object_props")) {
    UIWidgets::PropertyDragFloat("X", &pos.x);
    UIWidgets::PropertyDragFloat("Y", &pos.y);
    UIWidgets::PropertyCheckbox("Visible", &obj.visible);
    UIWidgets::EndPropertyTable();
}
```

### F2. RAII Window Wrapper

```cpp
class ScopedWindow {
public:
    ScopedWindow(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0)
        : visible_(ImGui::Begin(name, p_open, flags)) {}
    ~ScopedWindow() { ImGui::End(); }
    explicit operator bool() const { return visible_; }
    ScopedWindow(const ScopedWindow&) = delete;
    ScopedWindow& operator=(const ScopedWindow&) = delete;
private:
    bool visible_;
};

// Usage — End() guaranteed even on early return
void DrawPanel(bool* open) {
    ScopedWindow win("Properties", open);
    if (!win) return;  // Collapsed — End() still called by destructor
    ImGui::Text("Content");
}
```

> **Caveat (Omar Cornut):** Some `Begin*`/`End*` pairs have different semantics.
> `BeginMenu`/`EndMenu` should only call `EndMenu` if `BeginMenu` returns true.
> `Begin`/`End` always pairs. Know which pattern each API uses before wrapping.

---

## G. Useful Extensions for Complex Apps

| Extension | Purpose | Use Case |
|-----------|---------|----------|
| **ImPlot** | 2D plotting | Performance graphs, data visualization |
| **ImGuiColorTextEdit** | Syntax-highlighted editor | Script / config editing |
| **ImGuiFileDialog** | Cross-platform file dialogs | Open / save workflows |
| **ImNodes / imnodes** | Node graph editor | Visual scripting, event graphs |
| **ImGuiNotify** | Toast notifications | Operation feedback |
| **ImSpinner** | Loading spinners | Async operation feedback |

---

## Key References

| Resource | URL |
|----------|-----|
| ImGui FAQ | github.com/ocornut/imgui/blob/master/docs/FAQ.md |
| imgui_demo.cpp | github.com/ocornut/imgui/blob/master/imgui_demo.cpp |
| ShowStyleEditor() | Built-in real-time theme tweaking |
| ShowMetricsWindow() | Built-in ID debugging & draw call inspection |
| pthom's imgui_explorer | pthom.github.io/imgui_explorer |
