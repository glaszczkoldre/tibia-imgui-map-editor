# Rework Item Database Into One Final Source Of Truth

## Summary

Replace the current mixed `ItemType` model with a data-oriented item definition pipeline:

```text
source adapters -> source fragments -> resolver -> final item definition store -> final views
```

All OTB, DAT, SRV, XML, and future Proto/assets data must be normalized into one final `ItemDefinition` shape before editor/runtime code can see it. Runtime code must never ask whether a property came from OTB, DAT, SRV, XML, or Proto/assets, and must never combine duplicate fields such as `group + is_ground`, `flags + is_stackable`, or `item_type + group`.

## Key Architecture Changes

- Add a deep item database module with source fragments, a resolver, and a final store.
- Refactor `ClientDataService` so it orchestrates loading only. It must not merge field-by-field itself.
- Replace `ClientDataService::mergeOtbWithDat()` with `ItemDefinitionResolver`.
- Keep loading modes, but make them all resolve into the same final store:
  - `DatOtb`: OTB server metadata + DAT visuals + XML overrides.
  - `DatOnly`: DAT provides both server id and client id, then XML may override metadata.
  - `DatSrv`: SRV server metadata + DAT visuals + XML overrides.
  - `ProtoAssets`: future adapter produces the same server/client fragments and uses the same resolver/store.
- Replace duplicate runtime fields with final helpers:
  - `isGround()`, `isContainer()`, `isDoor()`, `isSplash()`, `isFluidContainer()`.
  - `hasFlag(ItemFlag::...)`.
  - Final visual fields for sprites, dimensions, animation, offset, light, and minimap.

## Final Source Mapping

### Final Identity And Classification

| Final field | OTB/SRV source | DAT source | XML source | Rule |
|---|---|---|---|---|
| `server_id` | OTB/SRV id | DAT id in DAT-only | XML id only for DAT-only remap | Final primary id used by maps. |
| `client_id` | OTB client id / SRV id | DAT id | XML `clientid` override | Resolver picks one final client id before DAT lookup. |
| `group` | OTB/SRV group | DAT group hints in DAT-only | XML explicit group override | In `DatOtb`/`DatSrv`, server metadata wins unless XML explicitly overrides. In `DatOnly`, DAT wins. |
| `type` | OTB/SRV type | DAT type hints where available | XML `type` override | One final secondary type. No separate caller-visible duplicate classifier. |
| `name/article/description/editor_suffix` | OTB/SRV text | none | XML overrides | XML text overrides when present. |

### Final Flags

| Final `ItemFlag` | OTB source | DAT canonical source | SRV/XML source | Rule |
|---|---|---|---|---|
| `Unpassable` | OTB bit 0 | `UNPASSABLE` | SRV `unpass`, XML `unpassable` | OR into final flags. |
| `BlockMissiles` | OTB bit 1 | `BLOCK_MISSILE` | SRV `unthrow`, XML `blockprojectile` | OR. |
| `BlockPathfinder` | OTB bit 2 | `BLOCK_PATHFINDER` | SRV `avoid`, XML `walkstack` | OR. |
| `HasElevation` | OTB bit 3 | `HAS_ELEVATION` with elevation > 0 | XML if needed | Set when final elevation > 0. |
| `Useable` | OTB bit 4 | `MULTI_USE`, `USABLE` | XML if present | Normalize to one final use flag policy. |
| `Pickupable` | OTB bit 5 | `PICKUPABLE` | SRV `take`, XML `pickupable` | OR. |
| `Moveable` | OTB bit 6 | inverse of DAT `UNMOVEABLE` | inverse SRV `unmove` | Final means moveable. DAT/SRV unmove clears it only in modes where no OTB authority exists. |
| `Stackable` | OTB bit 7 | `STACKABLE` | SRV `cumulative` | OR. |
| `FloorChangeDown/N/E/S/W` | OTB bits 8-12 | `FLOOR_CHANGE` generic only | XML directional floorchange | Preserve directional flags when source has them; generic DAT flag becomes final `FloorChange`. |
| `AlwaysOnBottom` | OTB bit 13 / stack order | `GROUND_BORDER`, `ON_BOTTOM` | SRV `clip/bottom/top` | Final flag plus final `AlwaysOnTopOrder` attribute. Rename away from misleading OTB “AlwaysOnTop”. |
| `CanReadText` | OTB readable bit / text len | `WRITABLE`, `WRITABLE_ONCE` | SRV `text/write`, XML `readable` | OR. |
| `CanWriteText` | OTB writeable group/type or text len | `WRITABLE`, `WRITABLE_ONCE` | SRV `write/writeonce`, XML `writeable` | OR. |
| `Rotatable` | OTB bit 15 | `ROTATABLE` | SRV `rotate`, XML `rotateto` | Final `Rotatable` requires either flag or `RotateTo`; helpers define exact behavior. |
| `IsHangable` | OTB bit 16 | `HANGABLE` | SRV `hang` | OR. |
| `HookEast` | OTB bit 17 | `HOOK_EAST` | SRV `hookeast` | OR. |
| `HookSouth` | OTB bit 18 | `HOOK_SOUTH` | SRV `hooksouth` | OR. |
| `CanNotDecay` / `Decays` | OTB bit 19 | none | XML duration/decay fields | Store final decay policy once. |
| `AllowDistRead` | OTB bit 20 | none | SRV/XML `allowdistread` | OR. |
| `ClientCharges` | OTB bit 22 | `CHARGEABLE` where applicable | SRV `rune`, XML charges/showcharges | Normalize to final charge flags and `Charges` attribute. |
| `IgnoreLook` | OTB bit 23 | `IGNORE_LOOK` | XML if present | OR. |
| `Animation` | OTB bit 24 | animation data / `ANIMATE_ALWAYS` | none | Final flag set when visual animation exists. |
| `FullTile` | OTB bit 25 | `FULL_GROUND` | XML if present | OR. |
| `ForceUse` | OTB bit 26 | `FORCE_USE` | XML if present | OR. |
| `Translucent` | none | `TRANSLUCENT` | none | Final visual/gameplay flag. |
| `DontHide` | none | `DONT_HIDE` | none | Final floor-visibility flag. |
| `OnTop` | none | `ON_TOP` | none | Final render ordering flag. |
| `NoMoveAnimation`, `Wrappable`, `Unwrappable`, `TopEffect`, `Cloth`, `MarketItem`, `LensHelp` | none | corresponding DAT flags | XML only if supported | Preserve as final flags/attributes even if few callers use them today. |
| Editor-only flags | brush XML / editor metadata | none | none | Keep out of core item identity; store in separate editor metadata table. |

### Final Attributes

| Final attribute | Source | Rule |
|---|---|---|
| `WaySpeed` | OTB speed, DAT ground speed, XML speed | OTB/SRV authoritative in server modes; DAT fills missing/DatOnly; XML explicit override. |
| `Volume` | OTB/SRV/XML container size | Final `Volume`; no `is_container` boolean. |
| `MaxTextLen` | OTB/DAT/XML | Max explicit source after precedence. |
| `LightLevel/LightColor` | OTB/DAT/XML | Final pair; DAT may fill visuals, XML may override. |
| `AlwaysOnTopOrder` | OTB stack order, DAT on-bottom/on-top, SRV clip/bottom/top | One final render-order attribute. |
| `Elevation` | OTB flag plus DAT elevation | Final elevation value sets `HasElevation`. |
| `MinimapColor` | DAT/OTB/XML | One final value. |
| `OffsetX/OffsetY` | DAT | Final visual offset. |
| `RotateTo` | OTB/XML/SRV | Final rotation target. |
| `SlotPosition`, `WeaponType`, `Weight`, `Attack`, `Defense`, `Armor`, `Charges`, `DecayTo`, `StopDuration`, `ShootRange`, `AmmoType`, `WareId`, `Market*`, `ClothSlot`, `DefaultAction`, `LensHelp` | OTB/SRV/XML/DAT depending on availability | Store once in final attributes; callers never parse source-specific fields. |

### Final Visual Data

| Final visual field | Source | Rule |
|---|---|---|
| `width/height/layers/pattern_x/pattern_y/pattern_z/frames` | DAT / Proto assets | Required for renderable items. |
| `sprite_ids` | DAT / Proto assets | Final sprite list only. |
| `idle/walk frame groups` | DAT / Proto assets | Final visual animation data. |
| `animation_mode/loop_count/start_frame/frame_durations/total_duration` | DAT / Proto assets | Final animation data; `total_duration` computed once in resolver. |
| `cached_sprite_region` | Sprite optimization pass | Keep as runtime cache, not source data. |

## Codebase-Wide Cleanup

- Convert parsers so they output fragments, not partially-final `ItemType` objects:
  - `OtbReader` returns server fragments.
  - `SrvReader` returns server fragments.
  - `DatReaderBase` returns DAT fragments using final canonical names.
  - `ItemXmlReader` returns XML override fragments, not direct mutations.
- Delete `ItemXmlReader::load(..., std::vector<ItemType>&, index)` style mutation.
- Delete `ClientDataService::mergeOtbWithDat()` and all source-specific field copies from it.
- Replace `ClientDataService` item storage with `ItemDefinitionStore`.
- Keep current lookup method names only as compatibility wrappers during migration; returned data must be final.
- Update all callers:
  - `Domain::Tile` uses `definition.isGround()` only.
  - Brushes use final `isGround()` and editor brush metadata only.
  - Rendering uses final visual fields and final render flags.
  - Search uses final helpers/flags only.
  - OTBM read/write uses final group/type/flags for count, splash, fluid, container, door, teleport.
  - Floor visibility uses final flags/helpers, never `is_ground`.
- Delete comments and logic that say “check both OTB and JSON/DAT”.
- Add compile-time protection:
  - no public `is_ground`, `is_stackable`, `is_pickupable`, `is_fluid_container`, etc. fields on final definitions.
  - source fragment headers live under IO/loader internals and are not included by rendering/brush/domain callers.
- Keep editor/brush ownership metadata separate from item identity.

## Test Plan

- Add resolver unit tests with tiny synthetic fragments:
  - OTB ground + DAT non-ground resolves as ground in `DatOtb`.
  - DAT ground + OTB non-ground resolves as non-ground in `DatOtb`.
  - DAT ground resolves as ground in `DatOnly`.
  - XML explicit group override changes final group.
  - DAT movement flags do not override OTB moveability in `DatOtb`.
  - DAT light/elevation/visuals fill final attributes.
  - XML text/slot/weapon/rotation/charges overrides apply once.
- Add source adapter tests:
  - OTB flag bits map to final fragment flags.
  - DAT canonical flags map to fragment group/type/flags/attributes.
  - SRV flags map to server fragments.
  - XML ranges and single-item nodes produce override fragments without mutating final storage.
- Add integration regression tests:
  - Sand dune `8317` does not become a real ground brush seed unless final resolved group and editor metadata say it should.
  - Border/specific-case/ground-equivalent ids cannot change final item classification.
  - Brush XML chance-zero entries register editor metadata but do not alter final source truth.
  - Ctrl+doodad erase still uses selected doodad ownership only.
- Run existing smoke suites:
  - `BrushSmoke`
  - `AutoborderSmoke`
  - `DoodadBorderSmoke`
  - `DoodadPlanningSmoke`
  - `DoodadContextSmoke`
  - `DoodadPreviewSmoke`
  - `DoodadXmlSmoke`
  - OTBM read/write round-trip tests
- Add a static cleanup check:

```powershell
rg "is_ground|is_stackable|is_pickupable|is_fluid_container|item_type ==" ImguiMapEditor
```

The check must find only source fragments, tests, or intentional migration comments.

## Assumptions And Defaults

- Keep OTB/DAT, DAT-only, SRV/DAT, and future Proto/assets modes.
- No caller-level backward compatibility branches: modes vary only before the resolver.
- RME_Readonly is the architectural reference: source fragments resolve into one final definition row; runtime views ask the final row only.
- In server-backed modes, OTB/SRV/Proto server metadata is authoritative for identity/group/type, while DAT is authoritative for visuals and DAT-only visual flags.
- XML is an explicit override/addition layer, not a hidden second classifier.
- Editor brush metadata is separate from final item truth; brush XML can associate items with brushes but must not redefine whether an item is ground/container/fluid/etc.
