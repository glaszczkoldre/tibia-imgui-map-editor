# Dear ImGui Deprecated APIs & Breaking Changes: v1.90.0 → v1.92.9

> Sources: [CHANGELOG.txt](https://github.com/ocornut/imgui/blob/master/docs/CHANGELOG.txt), [imgui.h obsolete section](https://github.com/ocornut/imgui/blob/master/imgui.h)
>
> **Tip**: Define `IMGUI_DISABLE_OBSOLETE_FUNCTIONS` in `imconfig.h` to force compile errors on all deprecated usage.

---

## 1. BeginChild API Overhaul

The largest API transition: `bool border` and window flags → dedicated `ImGuiChildFlags`.

| Version | Old (Deprecated) | New (Replacement) |
|---------|-------------------|-------------------|
| **1.90.0** | `BeginChild("Name", size, true)` | `BeginChild("Name", size, ImGuiChildFlags_Border)` |
| **1.90.0** | `BeginChild("Name", size, false)` | `BeginChild("Name", size, 0)` |
| **1.90.0** | `BeginChild(..., ImGuiWindowFlags_AlwaysUseWindowPadding)` | `BeginChild(..., ImGuiChildFlags_AlwaysUseWindowPadding, 0)` |
| **1.90.0** | `BeginChildFrame()` / `EndChildFrame()` | `BeginChild(..., ImGuiChildFlags_FrameStyle)` / `EndChild()` |
| **1.90.9** | `BeginChild(..., ImGuiWindowFlags_NavFlattened)` | `BeginChild(..., ImGuiChildFlags_NavFlattened, 0)` |
| **1.91.1** | `ImGuiChildFlags_Border` | `ImGuiChildFlags_Borders` (renamed for consistency) |
| **1.92.5** | All legacy child flag names | **Commented out** — compile errors |

```cpp
// BEFORE (pre-1.90)
BeginChild("Panel", ImVec2(200, 300), true);
BeginChild("Nav", ImVec2(0, 0), false, ImGuiWindowFlags_NavFlattened);
BeginChildFrame(id, ImVec2(200, 100));

// AFTER (1.92+)
BeginChild("Panel", ImVec2(200, 300), ImGuiChildFlags_Borders);
BeginChild("Nav", ImVec2(0, 0), ImGuiChildFlags_NavFlattened);
BeginChild(id, ImVec2(200, 100), ImGuiChildFlags_FrameStyle);
```

---

## 2. Font System Revolution (1.92.0)

**The largest breaking change since 2015.** Font system now supports dynamic sizing.

### PushFont() Signature

| Old | New | Notes |
|-----|-----|-------|
| `PushFont(ImFont*)` | `PushFont(ImFont*, float size)` | Size parameter required |
| `PushFont(NULL)` (revert to default) | `PushFont(NULL, size)` (keep font, change size) | Old NULL behavior removed |

```cpp
// BEFORE (pre-1.92)
PushFont(myFont);
PushFont(NULL);  // revert to default

// AFTER (1.92+)
PushFont(myFont, 0.0f);                       // Change font, keep current size
PushFont(NULL, 20.0f);                        // Keep font, change size
PushFont(myFont, 20.0f);                      // Change both
PushFont(myFont, style.FontSizeBase * 2.0f);  // Relative sizing (CORRECT)
// PushFont(myFont, GetFontSize() * 2.0f);    // WRONG: double-applies scaling
```

### Font Data Structures

| Old | New | Version |
|-----|-----|---------|
| `ImFont::FontSize` | **Removed** | 1.92.0 |
| `ImFont::Scale` | **Obsoleted** | 1.92.0 |
| `ImFont::ConfigData[]` | `ImFont::Sources[]` | 1.92.0 |
| `ImFont::Glyphs[]` | `ImFontBaked::Glyphs[]` | 1.92.0 |
| `ImFont::FindGlyph()` | `ImFontBaked::FindGlyph()` | 1.92.0 |
| `ImGui::GetFont()` (for glyph data) | `ImGui::GetFontBaked()` | 1.92.0 |
| `io.FontGlobalScale` | `style.FontScaleMain` | 1.92.0 |
| `SetWindowFontScale()` | `PushFont(NULL, style.FontSizeBase * factor)` | 1.92.0 |

### Font Atlas

| Old | New |
|-----|-----|
| `ImFontAtlas::TexWidth/Height` | `ImFontAtlas::TexData->Width/Height` |
| `ImFontAtlas::TexPixelsAlpha8/RGBA32` | `ImFontAtlas::TexData->GetPixels()` |
| `ImFontAtlas::GetTexDataAsRGBA32()` | **Obsoleted** (new texture protocol) |
| `ImFontAtlas::Build()` | **Obsoleted** |
| `ImFontAtlas::SetTexID()` | **Obsoleted** |
| `ImFontAtlas::FontBuilderFlags` | `ImFontAtlas::FontLoaderFlags` |
| `ImFontConfig::GlyphExtraSpacing.x` | `ImFontConfig::GlyphExtraAdvanceX` (1.91.9) |
| All `GetGlyphRangesXXXX()` | **Obsoleted** (glyph ranges now unnecessary) |

### FreeType

```cpp
// BEFORE
io.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();

// AFTER (1.92.0+)
io.Fonts->SetFontLoader(ImGuiFreeType::GetFontLoader());
```

---

## 3. Texture System Changes

| Version | Old | New |
|---------|-----|-----|
| **1.91.4** | `ImTextureID` defaults to `void*` | Defaults to `ImU64` |
| **1.92.0** | `ImTextureID` in API functions | `ImTextureRef` (auto-constructs from `ImTextureID`) |
| **1.92.0** | `ImDrawCmd::TextureId` | `ImDrawCmd::TexRef` |

```cpp
// Still works due to implicit conversion:
ImGui::Image((ImTextureID)(intptr_t)myGLTexture, ImVec2(100, 100));
```

---

## 4. IO → PlatformIO Migration

| Version | Old (ImGuiIO) | New (ImGuiPlatformIO) |
|---------|---------------|----------------------|
| **1.91.1** | `io.GetClipboardTextFn` | `platform_io.Platform_GetClipboardTextFn` |
| **1.91.1** | `io.SetClipboardTextFn` | `platform_io.Platform_SetClipboardTextFn` |
| **1.91.1** | `io.PlatformOpenInShellFn` | `platform_io.Platform_OpenInShellFn` |
| **1.91.1** | `io.PlatformSetImeDataFn` | `platform_io.Platform_SetImeDataFn` |
| **1.91.1** | `io.PlatformLocaleDecimalPoint` | `platform_io.Platform_LocaleDecimalPoint` |

**Note:** Callback signatures changed from `void* user_data` to `ImGuiContext* ctx`.

---

## 5. Input System Removal (1.91.5)

Pre-1.87 IO system **commented out entirely**:

| Removed | Replacement |
|---------|-------------|
| `io.KeyMap[]` | `io.AddKeyEvent()` |
| `io.KeysDown[]` | `IsKeyDown()` |
| `io.NavInputs[]` | `io.AddKeyAnalogEvent()` |
| `ImGuiNavInput` enum | Removed |
| `GetKeyIndex()` | Direct `ImGuiKey_XXX` usage |
| `ImGuiKey_COUNT` | `ImGuiKey_NamedKey_BEGIN` / `ImGuiKey_NamedKey_END` |

---

## 6. DrawList API Changes

### Parameter Reordering (1.92.8)

| Function | Change |
|----------|--------|
| `AddRect` | `(... rounding, flags, thickness)` → `(... rounding, thickness, flags)` |
| `AddPolyline` | `(... col, flags, thickness)` → `(... col, thickness, flags)` |
| `PathStroke` | `(col, flags, thickness)` → `(col, thickness, flags)` |

```cpp
// BEFORE
draw_list->AddRect(p_min, p_max, color, rounding, ImDrawFlags_None, border);
draw_list->AddPolyline(pts, count, color, ImDrawFlags_Closed, 2.0f);

// AFTER (1.92.8+)
draw_list->AddRect(p_min, p_max, color, rounding, border);
draw_list->AddPolyline(pts, count, color, 2.0f, ImDrawFlags_Closed);
```

### Other DrawList Changes

| Version | Old | New |
|---------|-----|-----|
| **1.90.5** | `AddEllipse(..., float rx, float ry)` | `AddEllipse(..., ImVec2 radius)` |
| **1.90.0** | `ImDrawCornerFlags_XXX` | `ImDrawFlags_RoundCornersXXX` (commented out) |
| **1.92.8** | `ImDrawCallback_ResetRenderState` | `GetPlatformIO().DrawCallback_ResetRenderState` |

---

## 7. Image API Changes (1.91.9)

| Old | New |
|-----|-----|
| `Image(tex, size, uv0, uv1, tint_col, border_col)` | `Image(tex, size, uv0, uv1)` — border via `style.ImageBorderSize` |
| N/A | `ImageWithBg(tex, size, uv0, uv1, bg_col, tint_col)` — new function |
| Old `ImageButton(ImTextureID, ...)` | `ImageButton(const char* str_id, ...)` (commented out in 1.91.1) |

---

## 8. Style & Color Renames

| Version | Old | New |
|---------|-----|-----|
| **1.90.9** | `ImGuiCol_TabActive` | `ImGuiCol_TabSelected` |
| **1.90.9** | `ImGuiCol_TabUnfocused` | `ImGuiCol_TabDimmed` |
| **1.90.9** | `ImGuiCol_TabUnfocusedActive` | `ImGuiCol_TabDimmedSelected` |
| **1.91.4** | `ImGuiCol_NavHighlight` | `ImGuiCol_NavCursor` |
| **1.91.8** | `ImGuiColorEditFlags_AlphaPreview` | Removed (now default). Use `AlphaOpaque` for old default. |
| **1.91.9** | `style.TabMinWidthForCloseButton` | `style.TabCloseButtonMinWidthUnselected` |
| **1.91.7** | `ImGuiTreeNodeFlags_SpanTextWidth` | `ImGuiTreeNodeFlags_SpanLabelWidth` |

---

## 9. Flag Renames & Removals

| Version | Old | New |
|---------|-----|-----|
| **1.90.7** | `ImGuiMod_Shortcut` | `ImGuiMod_Ctrl` (auto-remapped on macOS) |
| **1.90.8** | `ImGuiButtonFlags_MouseButtonDefault_` | **Removed** |
| **1.90.9** | `ImGuiDragDropFlags_SourceAutoExpirePayload` | `ImGuiDragDropFlags_PayloadAutoExpire` |
| **1.91.0** | `ImGuiModFlags` / `ImGuiModFlags_Ctrl` etc. | **Commented out** (use `ImGuiMod_*`) |
| **1.91.0** | `PushButtonRepeat()`/`PopButtonRepeat()` | `PushItemFlag(ImGuiItemFlags_ButtonRepeat, true)` |
| **1.91.0** | `PushTabStop()`/`PopTabStop()` | `PushItemFlag(ImGuiItemFlags_NoTabStop, ...)` |
| **1.91.0** | `ImGuiSelectableFlags_DontClosePopups` | `ImGuiSelectableFlags_NoAutoClosePopups` |
| **1.92.2** | `ImGuiTabBarFlags_FittingPolicyResizeDown` | `ImGuiTabBarFlags_FittingPolicyShrink` |
| **1.92.4** | `ImGuiTreeNodeFlags_AllowItemOverlap` | `ImGuiTreeNodeFlags_AllowOverlap` (commented out) |
| **1.92.4** | `ImGuiSelectableFlags_AllowItemOverlap` | `ImGuiSelectableFlags_AllowOverlap` (commented out) |
| **1.92.4** | `ImGuiListClipper::IncludeRangeByIndices()` | `IncludeItemsByIndex()` (commented out) |
| **1.92.5** | `ImGuiKey_ModCtrl/Shift/Alt/Super` | `ImGuiMod_Ctrl/Shift/Alt/Super` (commented out) |
| **1.92.5** | `SetItemAllowOverlap()` | `SetNextItemAllowOverlap()` (commented out) |
| **1.92.7** | `ImGuiMultiSelectFlags_SelectOnClick` | `ImGuiMultiSelectFlags_SelectOnAuto` |

---

## 10. Function Renames & Removals

| Version | Old | New |
|---------|-----|-----|
| **1.90.0** | `ShowStackToolWindow()` | `ShowIDStackToolWindow()` |
| **1.90.0** | `GetWindowContentRegionWidth()` | `GetContentRegionAvail().x` (commented out) |
| **1.90.0** | `IM_OFFSETOF()` | `offsetof()` |
| **1.90.1** | `CalcListClipping()` | `ImGuiListClipper` (**Removed**) |
| **1.90.7** | `CaptureKeyboardFromApp()` | `SetNextFrameWantCaptureKeyboard()` (commented out) |
| **1.90.7** | `CaptureMouseFromApp()` | `SetNextFrameWantCaptureMouse()` (commented out) |
| **1.91.0** | `GetContentRegionMax()` | `GetContentRegionAvail() + GetCursorScreenPos() - GetWindowPos()` |
| **1.91.0** | `GetWindowContentRegionMin()` | `GetCursorScreenPos() - GetWindowPos()` |
| **1.91.4** | `ImGuiConfigFlags_NavEnableSetMousePos` | `io.ConfigNavMoveSetMousePos` |
| **1.91.4** | `ImGuiConfigFlags_NavNoCaptureKeyboard` | `io.ConfigNavCaptureKeyboard` (inverted!) |
| **1.92.6** | `IM_ARRAYSIZE()` | `IM_COUNTOF()` |
| **1.92.7** | Old `Combo()`/`ListBox()` getter callback | New `const char* (*)(void*, int)` signature (commented out) |

### Content Region Replacement Formulas

```cpp
// BEFORE → AFTER
GetWindowContentRegionMax().x - GetCursorPos().x
  → GetContentRegionAvail().x

GetWindowContentRegionMax().x + GetWindowPos().x
  → GetCursorScreenPos().x + GetContentRegionAvail().x

GetContentRegionMax()
  → GetContentRegionAvail() + GetCursorScreenPos() - GetWindowPos()
```

---

## 11. Shortcut & Input System

| Version | Change |
|---------|--------|
| **1.90.7** | `ImGuiMod_Shortcut` removed. Use `ImGuiMod_Ctrl` (auto-remapped on macOS). |
| **1.90.7** | `Shortcut()`, `SetNextItemShortcut()` made public. |
| **1.91.5** | Pre-1.87 IO (`KeyMap[]`, `KeysDown[]`, `NavInputs[]`) fully removed. |

---

## 12. Callback Signature Changes

| Version | Old Signature | New Signature |
|---------|---------------|---------------|
| **1.90.0** | `bool (*getter)(void*, int, const char**)` | `const char* (*getter)(void*, int)` |
| **1.91.0** | `io.SetPlatformImeDataFn(viewport, data)` | `platform_io.Platform_SetImeDataFn(ctx, viewport, data)` |
| **1.91.1** | `io.GetClipboardTextFn(void*)` | `platform_io.Platform_GetClipboardTextFn(ImGuiContext*)` |

---

## 13. Popup API Changes (1.92.6)

Default `popup_flags` parameter changed meaning:

```cpp
// BEFORE: default = 1 (MouseButtonRight)
BeginPopupContextItem("foo", 0);  // 0 = LEFT button

// AFTER (1.92.6+): default = 0 (also = MouseButtonRight!)
BeginPopupContextItem("foo", 0);  // 0 = RIGHT button!
// To use LEFT button:
BeginPopupContextItem("foo", ImGuiPopupFlags_MouseButtonLeft);
```

**Rule:** Always use named `ImGuiPopupFlags_MouseButtonXXX` constants, never literal 0 or 1.

---

## 14. Hashing Changes

| Version | Change | Impact |
|---------|--------|--------|
| **1.91.6** | CRC32-adler → CRC32c | .ini data may be lost. Use `IMGUI_USE_LEGACY_CRC32_ADLER`. |
| **1.92.6** | `###` no longer includes "###" in hash | `GetID("Hello###World") == GetID("World")`. Invalidates .ini with `###`. |

---

## 15. Backend-Specific Breaking Changes

### Vulkan

| Version | Change |
|---------|--------|
| **1.90.3** | `RenderPass` moved to `ImGui_ImplVulkan_InitInfo` struct |
| **1.91.9** | `LoadFunctions()` added `api_version` parameter |
| **1.92.4** | `init_info.RenderPass` → `init_info.PipelineInfoMain.RenderPass` |
| **1.92.8** | Redesigned to separate ImageView + Sampler |

### DirectX 12

| Version | Change |
|---------|--------|
| **1.91.6** | `Init()` now takes `ImGui_ImplDX12_InitInfo` struct |

### SDL

| Version | Change |
|---------|--------|
| **1.90.3** | `ImGui_ImplSDL2_NewFrame(SDL_Window*)` removed |
| **1.90.7** | `RenderDrawData()` added `SDL_Renderer*` parameter |

---

## 16. Miscellaneous

| Version | Item | Details |
|---------|------|---------|
| **1.90.0** | `io.MetricsActiveAllocations` | **Removed** |
| **1.90.2** | `ImGuiIO::ImeWindowHandle` | **Commented out** |
| **1.92.0** | `AddCustomRectRegular()`/`AddCustomRectFontGlyph()` | `AddCustomRect()` |
| **1.92.7** | `Separator()` | Now has non-zero height (affects layout) |

---

## 17. Migration Checklist

Use when upgrading from pre-1.90 to 1.92+:

- [ ] Build with `IMGUI_DISABLE_OBSOLETE_FUNCTIONS` to find all deprecated usage
- [ ] **BeginChild**: `bool border` → `ImGuiChildFlags_Borders`, window flags → child flags
- [ ] **PushFont**: Add size parameter to all `PushFont()` calls
- [ ] **ImTextureID**: Update casts from `void*` to `(ImTextureID)(intptr_t)`
- [ ] **IO**: Move clipboard/IME/shell functions from `ImGuiIO` to `ImGuiPlatformIO`
- [ ] **Input**: Ensure backend uses `io.AddKeyEvent()` not `io.KeysDown[]`
- [ ] **DrawList**: Update `AddRect`, `AddPolyline`, `PathStroke` parameter order
- [ ] **Image**: Remove `tint_col`/`border_col` from `Image()`, use `ImageWithBg()`
- [ ] **ImageButton**: Ensure using the `str_id` overload
- [ ] **Colors**: `TabActive` → `TabSelected`, `NavHighlight` → `NavCursor`
- [ ] **Flags**: Update all renamed flags (see §9)
- [ ] **Content region**: Replace `GetContentRegionMax()` family with `GetContentRegionAvail()`
- [ ] **Shortcuts**: Remove `ImGuiMod_Shortcut`, use `ImGuiMod_Ctrl`
- [ ] **Popups**: Use named `ImGuiPopupFlags_MouseButtonXXX`, never literal 0 or 1
- [ ] **Font config**: `GlyphExtraSpacing.x` → `GlyphExtraAdvanceX`
- [ ] **Backend**: Check your backend's breaking changes (Vulkan, DX12, SDL)
- [ ] **Combo/ListBox**: Update getter callback signature
