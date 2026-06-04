---
name: imgui-best-practices
description: Dear ImGui 1.90+ best practices, deprecated API replacements, design patterns, and decision guides for C++ docking-based editors. Use when writing ImGui UI code, reviewing ImGui patterns, debugging ImGui layout issues, or when user mentions ImGui design, styling, or deprecated APIs.
---

# Dear ImGui 1.90+ Best Practices

## Agent Directive

> **When you see ImGui UI code that looks amateur, inconsistent, or violates these practices — fix it proactively.**
> Do not wait for the user to ask. Bad UI is a code smell equal to a bad algorithm.
> See [UI_DESIGN.md §8](UI_DESIGN.md) for the full "fix on sight" table.

## Quick Start — The 10 Golden Rules

1. **Always call `End()` after `Begin()`** — regardless of return value
2. **Always use `PushID()`/`PopID()` in loops** — never skip this
3. **Use `SetNextWindowSize(size, ImGuiCond_Once)`** — prevent first-frame flicker
4. **Keep UI stateless** — UI is a pure projection of your model/services
5. **Use `IsItemDeactivatedAfterEdit()`** — for commit-on-finish editing
6. **Use Tables, not Columns** — Tables API replaces legacy Columns
7. **Centralize your theme** — single `ApplyTheme()` function, semantic color names
8. **Use `DockBuilder` for default layouts** — run once, let .ini persist changes
9. **Create widget wrappers** — DRY, consistent, encapsulate style push/pop
10. **Use `ShowMetricsWindow()` and `ShowStyleEditor()`** — your best debugging tools

## Reference Files

| File | Contents |
|------|----------|
| [UI_DESIGN.md](UI_DESIGN.md) | **Production-grade UI/UX**: layout architecture, panel design, controls, visual systems, information density, interaction design, coherence checklist, "fix on sight" table |
| [DESIGN_PATTERNS.md](DESIGN_PATTERNS.md) | Good vs Bad design tables, "Instead of X use Y", common struggles, docking patterns, theming, widget abstractions |
| [DECISION_GUIDE.md](DECISION_GUIDE.md) | "When to use X?", "Why X > Y?", performance patterns, 1.90+ new features |
| [DEPRECATED.md](DEPRECATED.md) | All deprecated APIs from 1.90–1.92 with replacements |

## Key Decision Shortcuts

```
Need scrolling?          → BeginChild()
Need group-as-one-item?  → BeginGroup/EndGroup
Need aligned columns?    → BeginTable()  (NEVER legacy Columns)
Need disable widgets?    → BeginDisabled()  (NEVER manual gray-out)
Need static text?        → TextUnformatted()  (NEVER Text(user_str))
Need list >50 items?     → ImGuiListClipper
Need context menu?       → BeginPopupContextItem()
Need custom trigger popup? → OpenPopup() + BeginPopup()
Need stable window ID?   → "Display###StableID"
Need unique loop IDs?    → PushID(i) / PopID()
```

## UI Coherence Quick-Check

When writing or reviewing any ImGui panel, verify:
- Labels in fixed-width column, widgets fill remaining width (`-FLT_MIN`)
- Sections separated with `SeparatorText()` or `CollapsingHeader()`
- `AlignTextToFramePadding()` before every label next to a framed widget
- Empty states show guidance (`TextDisabled("Select an item...")`)
- Disabled widgets explain WHY via tooltip
- Colors from `Theme::` constants, never hardcoded `ImVec4`
- See [UI_DESIGN.md](UI_DESIGN.md) for the full checklist.

## Debug Toolkit

```cpp
ImGui::ShowMetricsWindow();              // Draw calls, windows, internal state
ImGui::ShowStyleEditor();                // Real-time theme tweaking
ImGui::ShowIDStackToolWindow();          // Hover widget → see full ID stack
io.ConfigDebugHighlightIdConflicts = true; // Visual ID conflict detection
ImGui::DebugStartItemPicker();           // Click widget → break at emitting code
```

