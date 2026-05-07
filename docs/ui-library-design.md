# Engine UI Library — Design

In-game UI library for the engine and MMO (per [tech-demo-checklist.md](tech-demo-checklist.md) Phase 0). **Data-driven** retained-mode widget library with a **visual editor in MMOEditor3D** — layouts in `.ui` markup files, themes in JSON, both hot-reloadable, both editable in text or in the editor canvas. ImGui stays for editor panels and dev overlays only; all in-game UI runs on this library.

This is the definitive design — no v1/v2 split. Everything documented here is part of the initial implementation scope. Features explicitly *not* included (HarfBuzz, gamepad, addon API, etc.) are listed at the end with reasoning.

Status: **design locked, implementation in progress.** Architectural decisions Q1-Q6 resolved 2026-05-06; Q7-Q15 (layering / ownership / build / protocol details) resolved 2026-05-06 same day.

## Locked decisions

### Library shape (Q1-Q6)

- **Q1 — Authoring: data-driven + visual editor.** Layouts in `.ui` files (XML), themes in JSON, both hot-reloadable. Visual `UIEditor` panel in MMOEditor3D edits the same files; text remains canonical.
- **Q2 — Layout: flex + anchor both.** Flex for menus / option screens; anchor + offset for HUDs.
- **Q3 — Text: MSDF.** Built offline with `msdf-atlas-gen`. Single shader for all sizes. Font fallback chain for missing glyphs. Shader outputs premultiplied alpha (`vec4(rgb * coverage, coverage)`).
- **Q4 — Localization: Latin atlas page initially (Spanish + English long-term); full string localization via `tr:key` lookup with locale files.** Multi-page architecture preserved so Cyrillic / Greek pages can be added later without code changes. No HarfBuzz / RTL / Indic shaping.
- **Q5 — Renderer: dedicated `UIRenderer`.** `Renderer2D` stays for in-world sprites.
- **Q6 — Gamepad: not included.** Input event struct shaped to accept it later if added.

### Layering and ownership (Q7-Q9)

- **Q7 — Namespace: `Onyx::UI` (engine-pure).** The library never imports `MMO::*`. MMO-specific glue (SocketId attribute resolution, `GameClient` bindings, mounted-state lookup) lives in a separate `MMOGame/Shared/UI/` adapter that depends on both. Library exposes `RegisterAttributeResolver(name, lambda)` so MMO can plug in `socket="OverheadName"` → opaque ID without the library knowing about sockets.
- **Q8 — `UI::Manager` owned by `Application`.** Instantiated next to `AssetManager`. `Application::GetUI()` accessor. `UILayer` is a thin layer that calls `Manager::Update()` / `DispatchInput()` / `Render()`. Engine-level so MMOEditor3D can render the `UIEditor` canvas without `GameClient`.
- **Q9 — `WorldUIAnchorSystem` owned by `Application`.** Engine subsystem at `Onyx/Source/Graphics/WorldUIAnchorSystem.{h,cpp}`. `SceneRenderer::BeginScene` bumps `cameraTickId`; each `Animator::Update()` bumps a per-animator `tickId`. Consumers (UI / FX / 3D audio / equipment) share the cache.

### Build and tooling (Q10-Q11)

- **Q10 — `pugixml` + `msdf-atlas-gen` + `efsw` via FetchContent.** msdf-atlas-gen also exposes a static library for runtime dynamic-glyph generation; CLI binary used as a build-time `add_custom_command` to bake atlases from `MMOGame/assets/ui/fonts/*.ttf`. Atlases are checked into the repo so a fresh clone runs without the build step.
- **Q11 — Testing: per-milestone manual scene only, no unit-test framework.** The "Test:" sections in each milestone are the validation script — run editor + client, exercise the documented scene, eyeball the result. Visual / perf regression deferred until something needs guarding.

### Protocol details (Q12-Q15)

- **Q12 — `AnimTickId` is per-`Animator`.** Counter incremented inside `Animator::Update()` after pose evaluation. Static entities (no animator) report tickId=0 forever → cache hits forever. Cache key: `(cameraTickId, entityId, socketSlot, animatorTickId)`.
- **Q13 — Mount detection via binding.** Surfaces as `{entity.mounted}` resolved by the MMO binding adapter. Fallback ladder lives in the widget; the widget receives `bool isMounted` from the bound entity proxy. Library stays engine-pure.
- **Q14 — `foreach` keys: unique within scope.** Duplicate key → warning + index-fallback for the offending run. Missing `key=` → index-based reconciliation (fine for append-only, ugly for reorders). Recommendation: always provide a key for reorder-prone lists. Iteration cap at 10,000 still applies.
- **Q15 — Hover/pressed/focused track at the event-handling widget.** State sits on the topmost ancestor with `onclick` / `draggable` / `focusable` on the hit path. Children below it don't independently get `:hover`. No propagation needed because hit-testing terminates at the right widget. Matches every shipping UI library; cleaner than CSS `:has()`.

### Editor and stage lifecycle (Q16-Q18)

- **Q16 — UIEditor primary navigation: stage selector, not file picker.** Top-bar dropdown picks **stage** (Login / CharSelect / In-Game / In-Game+Inventory / Combat-LowHP / etc.); selecting a stage mounts the right screens, activates the right theme, and loads the right mock profile in one click. Files are leaves inside stages; double-click a widget on canvas drills into its source `.ui`. Reasoning: in a real game screens compose (modal over HUD, tooltip over modal); file-by-file editing hides interactions. References: Unreal UMG tabs, FFXIV HUD layout editor, Figma pages.
- **Q17 — Client references screens by typed enum + manifest, not paths.** `UIStage::{Login, CharSelect, InGame, …}` mapped through `assets/ui/manifest.json` to `.ui` paths + theme + default mock + transitions. `m_UI->ShowStage(UIStage::Login)` mounts the configured screen. Manifest is hot-reloadable; renames don't require a client recompile. Server events are ID-mapped through the same manifest.
- **Q18 — Server is UI-agnostic.** Servers send typed events with payloads (`QuestOffer{npc, title, body, choices[]}`, `VendorOpen{npc, items[]}`, `SystemMessage{title, body}`); the client picks the matching `.ui` for each event type from the manifest. Decouples server/client teams; UI changes don't require server redeploy. For arbitrary GM-authored content, one generic `system_modal.ui` driven by `{title, body, button}` — never UI markup over the wire.

### Wrappers and locale rules

- **`Onyx::UI::Pending<T>` / `Stale<T>`** live in the library (`Onyx/Source/UI/Wrappers.h`). `GameClient` (MMO layer) wraps values returned from bindings; the library reads them and renders `:pending` / `:stale` pseudo-states. Library never imports MMO; MMO imports library.
- **CLDR plural rules embedded as a tiny subset.** No ICU. `LocaleRules.cpp` switches on locale tag and returns a `PluralCategory` for a count. en + es: `n == 1 ? one : other`. Stubs for de / fr / pt / it (same rule). New locales = new switch case (~5 LOC each). Skipped CLDR cost: ~30 MB.

---

## Foundation in the existing codebase

(Mapped from a thorough exploration pass on 2026-05-05, plus socket work landed 2026-05-06.)

- **`Renderer2D`** (`Onyx/Source/Graphics/Renderer2D.h`) — batched, screen-space-aware, ~160k quads max. Shader has a hardcoded `if/else` texture ladder (~4 textures effective), no scissor stack, no MSDF shader, no mask support. Stays for in-world 2D sprites; not extended for UI.
- **Text / fonts** — none outside ImGui. No `stb_truetype`, no FontAtlas, no GlyphAtlas. ImGui's atlas is not reusable.
- **Input** — polling-only via `Window::IsKeyPressed()` / mouse boolean arrays. No event structs, no dispatch, no stop-propagation. ImGui intercepts implicitly via `WantCaptureMouse/Keyboard`.
- **Existing in-game UI** — none. Login, character select, loading screens are all ImGui. In-game frame is 3D-only.
- **Frame order** — `OnUpdate` (3D render) → `ImGuiLayer.Begin` → `OnImGui` → `ImGuiLayer.End` (renders to backbuffer) → `Window::Update` (swap). The new UI slots after 3D, before ImGui.
- **Math** — `Vector2D/3/4D` + GLM. No `Rect2D` — we add one.
- **Asset loading** — `AssetManager` does async textures. No hot-reload yet — we add a watcher for `.ui`, theme, and locale files.
- **MMOEditor3D panels** — existing pattern is dockable ImGui panels with the `Layer` interface (Inspector, Outliner, Viewport, etc.). The `UIEditor` follows the same pattern.
- **Socket / attachment system** — landed in commit `2e393fa` (2026-05-06):
    - `Onyx::Attachment` / `AttachmentSet` (`Onyx/Source/Graphics/Attachment.h`) — engine-pure data: name + parent bone index (-1 = model-space) + offset transform. Dual lookup: `FindByName` (hashmap) and `FindByWellKnownSlot` (branch-free `std::array<int16_t, 64>`).
    - `MMO::SocketId` (`MMOGame/Shared/Source/Model/SocketId.h`) — 1-byte enum, 26 well-known IDs (`OverheadName=0` through `EquipQuiver=25`), `Custom=0xFF` for content-specific names. Bidirectional `SocketNameToId` / `SocketIdToName`.
    - `.omdl` bumped to **v3** with `ATCH` table (`MMOGame/Shared/Source/Model/OmdlFormat.h`); clean break from v2.
    - FBX importer detects `socket_<Name>` empties in both `Model` and `AnimatedModel`. Animated path walks node hierarchy upward to find the parent bone, folding non-bone parent transforms into the local.
    - Engine never imports `SocketId` — the MMO layer maps name → `SocketId` and assigns the well-known slot via `AssignWellKnownSlot(static_cast<uint8_t>(id), index)`. Clean layering.
- **Engine prerequisites still missing for M11** (called out below in the milestone) — landing as the pre-M1 sequence:
    - `AnimatedModel::GetMeshSpaceBonesPtr()` — accessor for `globalAnim[i]` (mesh-space animated bone transforms, distinct from skinning matrices). `AnimatedModel.h:141-146` flags this gap explicitly. Animator caches the array during `CalculateBoneTransform`.
    - `Animator::GetTickId()` — per-animator counter bumped inside `Update()`. Forms part of the projection cache key.
    - `WorldUIAnchorSystem` (`Onyx/Source/Graphics/WorldUIAnchorSystem.{h,cpp}`) — engine subsystem for projecting world points → screen with caching, culling, and Track/Latched modes. Owned by `Application`, used by UI / FX / 3D audio. `SceneRenderer::BeginScene` bumps `cameraTickId`.
    - Socket debug overlay in MMOEditor3D — colored dots at every socket on the selected entity, validates the chain end-to-end before consumers are written.

---

## Capabilities

### Core runtime

- **Widget tree** — parent/child, ownership, lifecycle (`OnAttach` / `OnDetach` / `OnUpdate` / `OnDraw` / `OnInput`), per-widget dirty-flag invalidation.
- **Layout** — flex + anchor + offset, switchable per widget.
- **Sizing** — min/max/preferred, content-driven for text widgets, aspect-locked, percent-of-parent.
- **Styling / theming** — separate from widget code, hot-reloaded, per-widget overrides, pseudo-states, inheritance.
- **Input** — full event abstraction: mouse move/down/up/wheel, key down/up/char, focus enter/leave, scroll. Modifier flags (Shift/Ctrl/Alt) on every event. Double-click (configurable threshold, default 250 ms). Long-press (default 600 ms). Hover-intent delay (default 400 ms before tooltip fires). Drag threshold (default 4 px before mouse-down→drag escalates).
- **Focus** — Tab / Shift-Tab / arrow nav between focusable widgets. `:focused` pseudo-state always renders a visible indicator.
- **Capture** — exclusive input claim for drags, modals, text input. `OnCaptureLost` event when capture ends (released or revoked by hot-reload).
- **Animation** — tweens on any property; easing library (cubic, elastic, back, bounce); state-transition triggers; sequenced timelines; `OnAnimationComplete` events.
- **Screen + overlay stack** — `SetScreen` replaces the active screen; `PushOverlay` / `PopOverlay` stacks modals on top.

### State flow and ownership

The library defaults to **unidirectional, read-only data flow** — UI observes game state, never mutates it. Mutation happens through registered event handlers calling C++ functions, which mutate game state through normal means.

- **`GameClient` owns truth.** All entity state, player state, settings live there. The UI library does not own application state.
- **Bindings are read-only by default.** `ctx.Add("hp", [&]{ return player.Hp(); })` — getter only. The widget can display the value but cannot change it.
- **Two-way bindings are explicit.** Form fields use `ctx.AddRW("user", &m_Form.user)` — read+write. Used only where the UI is the input surface (text input, slider drag, checkbox toggle).
- **Computed bindings cache automatically.** `{player.hp / player.maxHp}` is a computed expression. The expression tree is parsed once at load; the result is cached per frame and invalidated when any source changes.
- **Per-field dirty flags.** `RemoteEntity::SetHp()` flags only the `hp` source as dirty, not the entity. Bindings that don't reference `hp` skip re-eval entirely. Sources are named at registration time.

**Update phase order per frame:**
1. Network packets applied to `GameClient` (entity state, server events)
2. Client simulation tick (prediction, animation, particle, audio)
3. **UI binding sync** (re-eval dirty bindings, update widget values)
4. **UI input dispatch** (mouse, keyboard, focus, drag)
5. **UI render**

Bindings always read post-sim state. Handlers fired in step 4 see the same frame's state; their effects appear next frame.

**Cross-widget dependencies** are resolved through shared bindings, not widget-to-widget references. Two widgets reading `{target.hp}` see consistent values without coordinating.

**Pending / stale wrappers:**
- Bindings can return `Pending<T>` (data requested but not arrived) or `Stale<T>` (data returned but unconfirmed by server)
- Widgets render `:pending` / `:stale` pseudo-states with conventional styling (greyed-out, spinner, ghost text)
- Game code wraps values; library renders the convention

### Z-ordering and layering

UI compositing is layered with predictable rules so overlapping elements (modals, tooltips, dropdowns, world anchors) never produce visual surprises.

**Layer hierarchy (back to front):**
1. World-attached widgets (under HUD chrome)
2. HUD chrome (anchored screen elements)
3. Modals (pushed via `PushOverlay`)
4. Popups (dropdowns, context menus — z-above current modal, click-outside dismisses)
5. Tooltips (always topmost; never blocked by anything except other tooltips)
6. Toast notifications (above tooltips, ephemeral)
7. Drag ghost (above everything during drag)

**Within a layer:**
- **Tree order** is the default — later siblings draw above earlier siblings.
- **`z-order="N"` attribute** overrides tree order within a parent. Higher N draws later. Sorting is per-parent.

**Clipping escape:**
- Most widgets clip to their parent's bounds.
- **`Tooltip`, `Popup`, `Toast`, `ContextMenu`** carry `clip-escape="true"` and render to the topmost layer regardless of where they're attached.
- This is what lets a tooltip on a button inside a scrollview render above the scrollview, the parent panel, and the modal containing it.

**Popup stacking across screens:**
- A popup pushed from a modal stacks on top of that modal. Popup z-order is local to its layer; the parent screen's z-order determines its base layer.

### Render path

- **Dedicated `UIRenderer`** — separate batcher with per-batch shader switching, scissor / clip stack, premultiplied alpha, MSDF text shader, mask textures for non-rect clips. Shares lower-level GPU primitives with `Renderer2D`.
- **Single batched draw architecture** — entire HUD emits in 1-5 draw calls per frame. Draws grouped by shader, then texture, then scissor; state changes amortized.
- **9-slice / sliced sprites** — for resizable panels, button frames, window chrome.
- **Gradients, drop shadows, rounded rects** — shader primitives, sharp at any size.
- **GPU instancing** — repeated widget geometry (overhead bars, action-bar slots) drawn via instancing: one mesh, per-instance attribute buffer.

### Clipping and masking

Clipping has two modes with different cost profiles:

**Rect clip (scissor) — cheap:**
- `glScissor` rect, no batch break, no draw-call cost
- Used by `ScrollView`, `Container` with `clip="true"`, modal backdrops
- **Max scissor stack depth: 16**

**Non-rect clip (mask texture) — costly:**
- Forces a batch break (current batch ends, mask binds, next batch begins)
- Used for: rounded corners deeper than the shader can handle, circular minimaps, hex portraits
- **Performance budget: 4 mask binds per frame.** Beyond that, the renderer logs a warning.

**Rounded clip resolution rule (automatic):**
1. Shallow rounding (corner radius < 16 px, no nested clips): fragment-shader SDF discard. Free.
2. Deep nesting or large radius: falls back to mask texture.

**Batching across clips:**
- Scissor changes are batch-cheap
- Mask changes are batch-expensive (force batch break)

### Text system

MSDF rendering with full fallback infrastructure for unpredictable input (player names, chat, guild names).

**Atlas:**
- Built at CMake configure / build time with [`msdf-atlas-gen`](https://github.com/Chlumsky/msdf-atlas-gen) (FetchContent — CLI for offline bake, static lib for runtime dynamic glyphs)
- Initial ship: **Latin only** (Basic Latin U+0020-007F, Latin-1 Supplement U+00A0-00FF, Latin Extended-A U+0100-017F). Covers Spanish + English + most European Latin scripts (German, French, Portuguese, Italian, Polish).
- Multi-page architecture preserved: Cyrillic / Greek become additional `.atlas.bin` files when needed — no code changes
- One MSDF shader handles all sizes, all pages
- Shader outputs premultiplied alpha (`vec4(rgb * coverage, coverage)`)
- Atlases checked in to `MMOGame/assets/ui/fonts/` so a fresh clone runs without rebuilding them

**Font fallback chain:**
- Primary font + ordered fallback list per font slot
- Glyph lookup walks the chain — first font with the glyph wins
- Per-glyph fallback: a single string can mix glyphs from multiple fonts in one run
- Game-data text (player names) defaults to a wider fallback chain than chrome text

**Missing-glyph policy:**
- Glyph not in any chain font → render the **`.notdef` placeholder** ("tofu" box) from the primary font
- Console-log the missing codepoint (rate-limited per-codepoint)
- Editor inspector shows a warning on widgets containing missing-glyph strings

**Dynamic glyph generation:**
- Atlases ship prebuilt for the standard scripts
- Runtime glyph generator can render new glyphs into a separate dynamic atlas page using `msdf-atlas-gen` library
- Dynamic page evicts LRU when full

**Text measurement API:**
- `TextMetrics ui::MeasureText(stringView, fontId, size)` — returns size, baseline, line count, glyph rects
- Stable across frames

**Kerning and line height:**
- Kerning pairs from the font, applied during tessellation
- Line height = font ascender + descender + theme-configurable line-gap multiplier (default 1.2)

**Text vertex cache:**
- Tessellation cached per (string, font, size, color)
- Most names don't change frame-to-frame; cached vertex buffer re-used

### Built-in widget set

- **Layout:** `Container`, `Stack` (HBox / VBox), `Grid`, `ScrollView` (virtualized), `Spacer`, `TabView`
- **Display:** `Label`, `Image`, `Icon`, `ProgressBar`, `RoundedPanel`, `Divider`
- **Interactive:** `Button`, `Slider`, `Checkbox`, `Dropdown`, `TextInput`, `RadioGroup`
- **Composite:** `Tooltip`, `Modal`, `Popup`, `Toast`, `ContextMenu`
- **World-attached:** `Nameplate`, `OverheadBar`, `FloatingText`, `WorldMarker` — take `entity` + optional `socket` / `socket-mounted` / `anchor-mode` attributes; consume `WorldUIAnchorSystem` (engine layer)
- **Drag system:** `DragGhost` (auto-spawned), `DropTarget` (any widget can opt in via attributes)

### Drag and drop

First-class system. Required for inventory, action bar, character sheet, talent trees, AH listings, spellbook → action-bar binding.

**Drag start:**
- Mouse-down on a widget with `draggable="true"` arms a potential drag
- Drag begins when mouse moves > drag threshold (default 4 px) while button held
- Below threshold, the input is treated as a click

**Drag types:**
- Source declares its drag type as a string: `<itemicon draggable="true" drag-type="item" data="{item}"/>`
- Drop targets declare what they accept: `<actionslot drop-accept="item,spell"/>`
- Type strings are author-defined — `item`, `spell`, `quest-link`, `currency`, etc.

**Drag ghost:**
- During drag, a translucent copy of the source widget follows the cursor
- Override with `drag-ghost="ui/components/item_drag_ghost.ui"`

**Drop targets:**
- Highlight on hover when accepting (`:drop-target`)
- Reject visual when not accepting (`:drop-reject`)
- Drop fires `ondrop="handler"` event on target with `{ source-data, modifier-flags }` payload

**Cancel:**
- Escape key cancels drag (no drop)
- Drop on non-target = drop discarded
- Snap-back animation returns ghost to source (configurable)

**Modifier behaviors (surfaced on drop event):**
- Shift+drop: split stack
- Ctrl+drop: copy
- Alt+drop: bind to action

### Scrolling and virtualization

`ScrollView` is virtualized by default. Required for inventory grids (100+ items), chat windows (1000+ lines), auction house (10,000+ listings), quest log, friends list.

**Render-only-visible:**
- Children outside the visible scroll viewport are not drawn, not laid out, not bound-evaluated

**Widget recycling:**
- A scrollview maintains a pool sized to (visible count + overscan buffer)
- As items scroll into view, widgets are recycled (re-bound to new data) rather than constructed
- Per-item state (selection, focus) lives at the data level

**Smooth scrolling:**
- Scroll velocity decays with configurable friction
- Mouse wheel adds discrete velocity impulses

**Item size:**
- Fixed-height items: trivial virtualization
- Variable-height items: requires height cache; computed lazily, cached forever

**`foreach` + `ScrollView`:**
- `<scrollview><label foreach="msg in chat.messages" text="{msg.text}"/></scrollview>`
- Library detects the foreach inside scrollview and virtualizes automatically

### World-attached anchors

First-class. A widget anchors to a named **socket** on an entity's model — a bone-attached point authored as a `socket_<Name>` empty in the FBX, baked into `.omdl v3`. The library projects through `WorldUIAnchorSystem` (engine layer) every frame and feeds the screen position to the widget.

#### Socket source

`Onyx::AttachmentSet` (`Onyx/Source/Graphics/Attachment.h`) carries the data. The MMO layer maps authored names to `MMO::SocketId` (1-byte enum, `OverheadName=0` through `EquipQuiver=25`, `Custom=0xFF` for content-specific anchors). Lookup is branch-free for well-known IDs (`std::array<int16_t, 64>`), hashmap for Custom names. See [Foundation in the existing codebase](#foundation-in-the-existing-codebase) for what shipped 2026-05-06 vs what's still pending.

#### Widget attributes

```xml
<!-- Default fallback ladder: just provide entity, library handles the rest -->
<overheadbar entity="{npc}"
             hpratio="{npc.hp / npc.maxHp}"
             name="{npc.name}"/>

<!-- Override the socket explicitly -->
<overheadbar entity="{boss}"
             socket="OverheadName"
             socket-mounted="OverheadMounted"
             hpratio="{boss.hp / boss.maxHp}"/>

<!-- Custom socket (resolves via name lookup, no enum entry needed) -->
<beam entity="{boss}" socket="BossBeamOrigin"/>

<!-- Damage text: latched at spawn, animates in screen space afterward -->
<floatingtext entity="{evt.victim}"
              socket="Chest"
              anchor-mode="latched"
              text="{evt.amount}"/>
```

Attributes:
- **`entity="{ref}"`** — binding to the entity. Replaces a generic `worldpos`.
- **`socket="Name"`** — primary socket; default per widget type (`OverheadName` for nameplates, `Chest` for damage text, etc.).
- **`socket-mounted="Name"`** — alternate when entity is mounted; default per widget type.
- **`anchor-mode`** — `track` (default; resolve every frame, follows animation) or `latched` (resolve once at attach, then animate in screen space). Required for damage text to avoid jitter.

The loader resolves `socket="OverheadName"` to `MMO::SocketId::OverheadName` at parse time via `SocketNameToId`. Widgets store `SocketId` (1 byte), not the string. Custom names store both the `Custom` ID and the original name string for the hashmap lookup path.

#### Fallback ladder (built-in to overhead widgets)

```
1. socket-mounted (if entity is mounted and that socket exists on the model)
2. socket (primary)
3. AABB top + small padding (legacy / imported models without sockets)
```

Authors don't write the ladder in `.ui` — it's hardcoded in `OverheadBar` / `Nameplate` / similar built-ins. The same logic applies to all overhead-anchor widgets.

#### Track vs latched

- **Track** (default, for nameplates / cast bars / health bars): re-resolve socket → world → screen every frame. Anchor follows bone animation.
- **Latched** (for damage numbers / hit sparks): resolve once at widget attach, then animate in screen space. Without this, FCT jitters with every camera and animation movement (the WoW FCT-jitter complaint that addons fixed the same way).

For one-shot transient effects (combat events), `FloatingText` exposes an imperative spawn API:

```cpp
m_UI->SpawnFloatingText({
    .entity     = victim,
    .socket     = SocketId::Chest,
    .anchorMode = AnchorMode::Latched,
    .text       = std::to_string(damage),
    .color      = critical ? Color::Yellow : Color::White,
    .lifetime   = 1.5f,
    .animation  = FloatingTextAnim::RiseAndFade,
});
```

For ongoing event streams (combat log entries), `<floatingtext foreach="evt in damageEvents">` works equivalently.

#### Engine ownership

`WorldUIAnchorSystem` lives in the engine (`Onyx/Source/Graphics/`), not the UI library — multiple subsystems consume projected world anchors:

- UI library — overhead bars, nameplates, damage text, quest markers
- FX system — particle spawns at muzzles, beam endpoints
- 3D audio — positional sound at HandR, MountSeat
- Equipment scene-graph — gear meshes parented to EquipHelm / EquipShoulderL etc.

The UI library is one consumer of `WorldUIAnchorSystem::Project(entityId, socketId, anchorMode)`; the cache and culling logic live in the engine.

#### Cache hit rates per workload

Realistic expectations now that sockets follow bone animation:

| Workload | Hit rate |
|---|---|
| Idle entities (no animation) | ~95% (camera-bound only) |
| Walking entities | ~50% (bone moves with gait cycle) |
| Active-combat entities (full anim) | ~5-10% |
| Latched anchors (damage text after spawn) | 100% |

Cache key: `(cameraTickId, entityId, socketId, animTickId)`. Combined with frustum cull + GPU instancing, the perf budget holds for 100 visible entities even with several in active combat.

#### Render-through-walls convention

Occlusion test optional, off by default. Convention matches WoW's nameplate behavior; lets you see HP bars through corners.

### Localization

UI strings looked up by key, not hardcoded. Independent of glyph rendering.

```xml
<button text="{tr:login.button.signin}"/>
<label text="{tr:hud.tooltip.health}"/>
```

**Locale files** live alongside themes:
```
MMOGame/assets/ui/locale/
├── en.json
├── es.json
├── de.json
├── ru.json
```

```json
{
    "login.button.signin": "Sign In",
    "hud.tooltip.health": "Health",
    "hud.kills": "{count, plural, one {# kill} other {# kills}}"
}
```

**Resolution:**
- Active locale set by `Settings.Locale`
- Missing key in active locale → fall through to default locale (en)
- Missing in default too → render the key itself as a visible bug indicator

**Hot-reload:**
- Locale file watcher — on change, all `tr:` lookups refresh
- Locale switch in settings refreshes without restart

**Pluralization + interpolation:**
- ICU MessageFormat-style: `{tr:hud.kills:count=3}`
- Library uses an ICU subset supporting `{name}` substitution and `{name, plural, ...}` for cardinal plurals
- CLDR plural rules embedded as a hand-authored switch in `LocaleRules.cpp`. en + es ship initially (`n == 1 ? one : other`), with stubs for de / fr / pt / it (same rule). New locales add a switch case (~5 LOC). No ICU dependency.

### Accessibility

Hooks designed in from the start. Cheap to support, expensive to retrofit.

- **UI scale** — global multiplier (0.7x to 2.0x), per-user setting, hot-applies
- **Semantic color tokens** — `$primary`, `$danger`, `$success`, `$warning`, `$muted`, `$text`, `$bg`. Colorblind variants swap token values.
- **Focus visibility** — `:focused` always renders a visible indicator (default 2 px outline)
- **Text size override** — independent of UI scale
- **Reduced-motion mode** — disables decorative animations; functional ones (progress bars, cooldown sweeps) stay
- **High-contrast theme variant** — `themes/high-contrast.json`

### Audio hooks

UI publishes events to a named bus; the engine's audio subsystem subscribes. The library doesn't depend on audio directly.

**Theme-driven (most common):**
```json
{
    "button:hover":   { "sound": "ui_hover" },
    "button:pressed": { "sound": "ui_click" },
    "modal:open":     { "sound": "ui_modal_open" }
}
```

**Attribute-driven (per-widget override):**
```xml
<button onclick="cast" sound-click="spell_cast" sound-hover="spell_hover_zap"/>
```

**Engine-side subscription:**
```cpp
audio.SubscribeToUIBus([&](const UIAudioEvent& e){
    audio.PlayOneShot(e.soundName, AudioCategory::UI, e.volume);
});
```

**Built-in events:** pseudo-state entries, `Modal::OnOpen` / `OnClose`, `Toast::OnShow`, `DragSystem::OnDragStart` / `OnDrop`, `Tooltip::OnShow`.

### Stages, file layout, and server-driven UI

The library is organized around **stages** (login / character select / in-game / dead / etc.), not arbitrary file paths. Each stage names a root screen; overlays and server-event UI compose on top.

#### File layout

```
MMOGame/assets/ui/
├── manifest.json            ← stage → screen + theme + default mock; event → screen; resolutions
├── login/
│   ├── login.ui
│   ├── login.mock.json      ← preview-only fake bindings (gitignored)
│   └── login.notes.md
├── character_select/
│   ├── character_select.ui
│   └── character_select.mock.json
├── hud/
│   ├── hud.ui               ← root: action bar, unit frames, chat, minimap
│   ├── action_bar.ui        ← <include>'d into hud.ui
│   ├── unit_frame.ui
│   ├── chat.ui
│   └── hud.mock.json
├── inventory/
│   ├── inventory.ui         ← modal, pushed as overlay
│   ├── item_tooltip.ui
│   └── inventory.mock.json
├── system/
│   ├── system_modal.ui      ← generic server-message modal (Q18)
│   ├── quest_offer.ui       ← bound to server QuestOffer event
│   └── vendor.ui            ← bound to server VendorOpen event
├── components/
│   ├── unit_frame.ui        ← shared <include> targets
│   ├── tooltip.ui
│   ├── item_icon.ui
│   └── drag_ghost.ui
├── themes/
│   ├── dark.json
│   └── high_contrast.json
├── locales/
│   ├── en.json
│   └── es.json
├── mocks/
│   ├── low_hp_combat.json   ← reusable mock profile (cross-screen)
│   ├── full_inventory.json
│   └── dead_target.json
└── fonts/
    ├── Roboto-Medium.ttf
    └── Roboto-Medium.atlas.bin
```

Stages own their root `.ui` plus any inner files used only by that stage; cross-stage shared content lives in `components/`. Each screen can have a sibling `.mock.json` for editor-only preview values (gitignored — not shipped). Reusable mock profiles live in `mocks/` and are referenced from the manifest or selected in the editor's mock dropdown.

#### Stage management

```cpp
enum class UIStage : uint8_t {
    Login,
    CharacterSelect,
    Loading,
    InGame,
    DeathScreen,
    GameOver,
    Cinematic,
    Disconnected,
};
```

`UIStageController` (engine) owns:
- Enum-to-screen mapping (read from `manifest.json`)
- Stage transitions (mount new root, unmount old; preserve overlays where relevant)
- Overlay stack (inventory, settings, social — pushed on top of any stage)
- Server-event dispatch (`QuestOffer` from server → push `quest_offer.ui` as overlay, bound to event payload)

```cpp
m_UI->ShowStage(UIStage::Login);
m_UI->TransitionTo(UIStage::CharacterSelect);  // mounts char-select; unmounts login
m_UI->PushOverlay("inventory");                // pushes inv on top of current stage
m_UI->DispatchEvent("QuestOffer", payload);    // pushes quest_offer.ui bound to payload
```

#### Manifest schema

`assets/ui/manifest.json`:

```json
{
    "stages": {
        "Login":           { "screen": "login/login.ui",                       "theme": "dark", "mock": "default" },
        "CharacterSelect": { "screen": "character_select/character_select.ui", "theme": "dark" },
        "InGame":          { "screen": "hud/hud.ui",                           "theme": "dark", "mock": "default_combat" },
        "DeathScreen":     { "screen": "system/death.ui",                      "theme": "dark" }
    },
    "overlays": {
        "inventory":  "inventory/inventory.ui",
        "settings":   "settings/settings.ui",
        "social":     "social/social.ui"
    },
    "events": {
        "QuestOffer":  "system/quest_offer.ui",
        "VendorOpen":  "system/vendor.ui",
        "SystemModal": "system/system_modal.ui"
    },
    "resolutions": [
        { "name": "16:9 1080p",     "width": 1920, "height": 1080, "safeAreaInset": 0 },
        { "name": "16:9 1440p",     "width": 2560, "height": 1440, "safeAreaInset": 0 },
        { "name": "21:9 ultrawide", "width": 2560, "height": 1080, "safeAreaInset": 64 },
        { "name": "4:3 1024",       "width": 1024, "height": 768,  "safeAreaInset": 0 }
    ]
}
```

Manifest is hot-reloadable. New stages added by editing the JSON + extending the enum (the only client-side touch). Renames and theme/mock swaps don't require a client rebuild. The editor's stage dropdown reads this manifest as its source of truth.

#### Server-driven UI

Servers never reference `.ui` files. They send typed events with payloads; the client owns the rendering decision.

```cpp
// Server side — pure data:
client.Send<QuestOffer>({
    .npcId   = questgiver.Id(),
    .title   = quest.Title(),
    .body    = quest.Description(),
    .choices = { "Accept", "Decline", "Tell me more" },
});

// Client side — manifest dispatch:
ui.RegisterEventHandler<QuestOffer>([&](const QuestOffer& evt) {
    m_UI->DispatchEvent("QuestOffer", evt);  // pushes system/quest_offer.ui bound to evt
});
```

`system/quest_offer.ui` binds the payload:

```xml
<modal id="quest-offer">
    <label class="quest-title" text="{evt.title}"/>
    <label class="quest-body"  text="{evt.body}"/>
    <stack direction="horizontal" gap="8">
        <button foreach="choice in evt.choices" text="{choice}" onclick="quest_choice"/>
    </stack>
</modal>
```

**Why this shape:**
- Server team writes packet definitions; UI team writes `.ui` files. Independent iteration loops.
- The same protocol can drive a different client (custom HUD addon, mobile companion, headless test bot) without server changes.
- UI changes don't require server redeploy.
- Adversarial inputs are bounded: the server can't send arbitrary markup, only typed payload fields.

**Generic content escape hatch:** for genuinely arbitrary content (GM broadcasts, MOTD, error messages) use a single `system/system_modal.ui` driven by `{title, body, button}`. Servers send `SystemModal{title, body, button}`; the client renders in a fixed layout. Never put markup in the packet — even for "dynamic" UI, the schema stays typed.

### Authoring (data-driven, two paths)

#### Path 1 — Visual editor (`UIEditor` panel in MMOEditor3D)

Primary authoring surface for new screens. Composed of dockable panels mirroring the existing Inspector / Outliner pattern:

- **Canvas viewport** — renders the layout using the actual `UIRenderer`. Selection handles overlay; click widget → select. Move/resize gizmos with snap-to: parent edge, sibling edge, percent, grid.
- **Tree outliner** — hierarchical widget tree; reorder via drag, multi-select, search/filter (virtualized for >200 widgets).
- **Widget palette** — toolbox of every built-in widget; drag onto canvas or tree. Includes a "Components" tab listing reusable `<include>` files.
- **Property inspector** — type-aware editors. For socket attributes, dropdown autocompletes from `MMO::SocketId` enum at compile time; "Custom..." entry opens text field; custom values badged as `(custom)`.
- **Theme picker** — switches active theme JSON; live preview reflects style cascade.
- **Binding mock panel** — provides fake values for `{player.hp}` etc. so the preview looks alive without running the game.
- **Locale picker** — preview under any registered locale; exposes missing-key warnings inline.
- **Socket debug overlay** — when a world-attached widget is selected, renders the selected entity model with all its sockets as colored dots, labeled by name. Reuses the engine's debug overlay (M11 prerequisite). Useful for "why is my nameplate floating in space" debugging.
- **File browser** — lists `.ui`, theme, locale files under `MMOGame/assets/ui/`; open / create / duplicate / delete.

**Editor ↔ live game coordination:** the editor saves `.ui` → file watcher in any running game/client process detects → auto-reloads. Edit in MMOEditor3D and see changes in a separate live MMOClient window without restart.

**Undo/redo** integrates with Editor3D's existing undo stack.

**Round-trip save** preserves comments and attribute formatting where possible.

**Editor performance:**
- Tree virtualizes if >200 widgets
- Property inspector debounces typing (200 ms) before applying
- Canvas re-renders only on dirty (no idle redraw cost)

#### Path 2 — Direct text editing

`.ui` files are plain XML. Open in any text editor, save → same hot-reload. Right workflow for: small tweaks, programmatic patterns (`foreach`), code review (text diffs), bulk renames.

Both paths produce identical `.ui` files. Text is canonical.

#### Layout — `.ui` (XML markup)

XML chosen over JSON: tree structure maps to nested elements, attribute syntax less noisy than nested JSON, comments first-class, decades of editor support. Parser: [pugixml](https://pugixml.org) as vendor lib.

```xml
<!-- ui/hud/player_frame.ui -->
<stack id="player-frame" class="unit-frame" anchor="top-left:20,20" direction="vertical" padding="8">
    <image class="portrait" src="{player.portrait}" size="64,64"/>
    <label class="unit-name" text="{player.name}"/>
    <progressbar class="health-bar" value="{player.hp / player.maxHp}"/>
    <progressbar class="mana-bar"   value="{player.mp / player.maxMp}"/>
    <stack class="buff-row" direction="horizontal" gap="2">
        <bufficon foreach="aura in player.auras" key="{aura.id}" data="{aura}"/>
    </stack>
</stack>
```

#### Reusable components — `<include>`

```xml
<!-- ui/components/unit_frame.ui -->
<stack class="unit-frame" direction="vertical" padding="8">
    <image  class="portrait" src="{$portrait}" size="64,64"/>
    <label  class="unit-name" text="{$name}"/>
    <progressbar class="health-bar" value="{$hpRatio}"/>
</stack>
```

```xml
<!-- ui/hud.ui -->
<include src="ui/components/unit_frame.ui"
         portrait="{player.portrait}"
         name="{player.name}"
         hpRatio="{player.hp / player.maxHp}"
         anchor="top-left:20,20"/>
```

**Parameters:** include attributes become locals scoped to the included tree (`{$paramname}`).
**Composition:** includes can include other includes. Cycle detection at load time.
**Override:** standard attributes (`anchor`, `visible`, `class`) on `<include>` apply to the root of the included tree.

#### Theme — JSON

Style cascade. Selectors limited to class names + pseudo-states. Resolution order: **explicit attribute > class > pseudo-state** — deterministic, no CSS-style specificity rules.

```json
{
    "$primary":         "#3a7bd5",
    "$danger":          "#cc3333",
    "$text":            "#eeeeee",

    "unit-frame":       { "background": "#1a1a1aee", "border-radius": 4, "padding": 8 },
    "health-bar":       { "fill-color": "$danger", "background-color": "#33333388", "height": 16 },
    "health-bar:hover": { "fill-color": "$primary" }
}
```

#### Bindings

Expressions in `{...}` resolve against a registered context.

```cpp
ui.RegisterBindings("player", [&](BindingContext& ctx) {
    ctx.Add("hp",       [&]{ return player.Hp(); });
    ctx.Add("maxHp",    [&]{ return player.MaxHp(); });
    ctx.Add("name",     [&]{ return player.Name(); });
    ctx.Add("portrait", [&]{ return player.PortraitPath(); });
    ctx.Add("auras",    [&]{ return MakeArrayView(player.Auras()); });
});
```

Expression support: name lookup, arithmetic, comparisons, logical ops, member access. No function calls.

Per-field dirty flags ensure most evals are skipped.

#### Event handlers

```xml
<button text="Logout" onclick="logout"/>
```

```cpp
ui.RegisterHandler("logout", [&]{ Disconnect(); });
```

Handler receives `EventArgs` with: source widget, modifier flags, `data` payload, drop source data (for `ondrop`).

#### Hot-reload

File watcher (`efsw`, FetchContent) monitors `MMOGame/assets/ui/`. On `.ui` change, the affected subtree reloads. On theme change, all styles re-cascade. On locale change, all `tr:` lookups refresh. Bindings persist across reload.

#### C++ direct API

`ui.Add<Button>().Text("hi")` always works — same constructors and setters the loader calls. Used for dev tools, debug overlays, profiler panels.

### Performance architecture

Target: **0.2-0.5 ms for the in-game HUD** at 100 visible entities (overhead bars + nameplates + damage numbers + chrome). Comparable to optimized AAA UI; within budget for 60 FPS with headroom for 144 Hz.

These commitments shape the architecture from M1 onward.

#### Built into the design

| Optimization | Where | What it buys |
|---|---|---|
| Single batched draw architecture | M2 — `UIRenderer` | Entire HUD in 1-5 draw calls instead of 100+. Biggest single win. |
| Per-batch shader switching | M2 | Mid-frame switches without flushing everything. |
| Scissor before mask preference | M2 | Cheap scissor used first; mask only when geometry demands. |
| Pool allocators | M2/M3/M11 | Widgets, vertex buffers, glyph runs draw from pools. Zero allocation in steady state. |
| Text vertex cache | M3 — `Label` / MSDF | Tessellation cached per (string, font, size, color). Names that don't change produce zero CPU work. |
| Per-field dirty-flag bindings | M7 — `BindingContext` | Sources mark dirty by field name; bindings cache and skip re-eval. ~70% of evals avoided in steady state. |
| Computed binding cache | M7 | Expression results cached per frame; invalidated on source dirty. |
| `foreach` keyed reconciliation | M7 | Widget identity stable across list reorders. No tear-down/rebuild on every change. |
| `foreach` pre-cull | M7 | Skip entries beyond view distance / outside frustum before instantiating widgets. |
| Pre-evaluated expression trees | M7 | Expressions parsed to AST once at load; evaluated by direct dispatch. |
| `ScrollView` virtualization | M9 | Render-only-visible + widget recycling. Inventory/chat/AH stay flat. |
| **`WorldUIAnchorSystem` projection + cache** | M11 — owned by engine | World→screen projected once per (cameraTickId, entityId, socketId, animTickId); UI library is a consumer. Hit rate ~95% idle, ~50% walking, ~5-10% active combat. |
| Frustum + distance cull in `OnDraw` | M11 | Off-screen / too-far widgets early-out before any quad emission. |
| GPU-instanced repeated geometry | M11 — `OverheadBar`, action-bar slots | One mesh, per-instance attribute buffer. One draw call regardless of count. |

#### Profile-driven additions

Kept architecturally accessible. Implemented when measurement shows >0.05 ms gain.

- **SoA entity render data.** Cache-friendlier iteration. Local change inside the world-attached widget rendering path.
- **GPU-side projection.** Project entity world positions in a compute shader. CPU→GPU sync cost likely dominates at our entity count.
- **Expression JIT compilation.** AST traversal already ~100 ns; JIT saves ~50 ns. Marginal.

#### Frame budget reference

| Target | Frame budget | Typical 3D + post | Game logic + net + anim | Left for UI |
|---|---|---|---|---|
| 60 FPS | 16.67 ms | 8-12 ms | 2-4 ms | 1-3 ms |
| 144 FPS | 6.94 ms | 3-5 ms | 1-2 ms | 0.5-1 ms |
| 240 FPS | 4.17 ms | 2-3 ms | 0.5-1 ms | 0.2-0.5 ms |

### Error handling and fail-soft

UI failures must never crash the game. Hot-reload makes mid-edit failures inevitable; the library recovers gracefully.

| Failure | Behavior |
|---|---|
| Missing texture | Render magenta `MISSING TEXTURE: path` placeholder. Log warning. |
| Missing theme key | Fall through to default style. Log warning. |
| Unregistered event handler | Log warning, fire no-op. |
| Binding key not in context | Render empty string / 0 / false. Log warning (rate-limited). |
| Invalid expression syntax | Load-time error with line number. Affected widget skipped. |
| `.ui` parse error | Load-time error with line + column. Previous version stays loaded. |
| Hot-reload of file with new errors | Previous version stays loaded. Log error. |
| Circular include | Load-time error showing the cycle chain. File rejected. |
| `foreach` over null/missing source | Treat as empty list. Log warning. |
| `foreach` exceeds iteration cap | Stop at cap, log warning. |
| Mask texture creation fails | Fall back to scissor (rectangular clipping). Log warning. |
| Missing glyph in all fonts | Render `.notdef` placeholder. Log codepoint. |
| Missing locale key | Fall through to default locale; if also missing, render the key string. |
| Drag of unregistered type | Treat as click. Log warning. |
| Drop on incompatible target | Visual reject (`:drop-reject`); no event fires. |
| Socket name not found on model | Fall through ladder (mounted → primary → AABB). Log warning if AABB fallback used (rate-limited). |
| Animated socket queried without bone pose | Returns false from `ResolveWorld`; widget falls through ladder. |

**Logging tiers:** dev = all warnings to console with stack; release = warnings to file at lower frequency, errors always logged.

**Rate limiting:** same-message warnings throttled per N frames per origin.

### Lifetime and hot-reload semantics

Hot-reload is a primary workflow; widget lifetime rules are explicit.

**Identity preservation:** widgets with `id="foo"` retain identity across reloads. Without id are torn down and rebuilt. Authors should id widgets that hold transient state.

**Focus persistence:** if focused widget id still exists post-reload, focus is preserved. Otherwise moves to first focusable widget in tree order.

**Scroll position persistence:** `ScrollView` with id retains scroll offset across reloads.

**Animation handling:** in-progress animations on reloaded widgets cancel cleanly. New theme triggers animations to current state values.

**Input capture:** captured input released on reload. Drag in progress is cancelled (ghost discarded, source restored). `OnCaptureLost` fires.

**Pool widgets:** survive reload. Widgets in a foreach pool are bound to new data, not destroyed/rebuilt.

### Sandboxing

- No function calls in expressions
- No recursion possible
- No allocation in expression eval
- `foreach` iteration cap: 10,000
- Trusted source assumption (no third-party UI loaded)

### Integration

- The library lives in namespace `Onyx::UI`. MMO-specific glue (SocketId resolver, GameClient bindings, mounted-state lookup) lives in `MMOGame/Shared/UI/` and depends on the library.
- `Onyx::UI::Manager` is owned by `Application` (next to `AssetManager`). Accessor: `Application::GetUI()`.
- Slots into the frame as a new `UILayer` after 3D render and before ImGui's `RenderDrawData`. The layer calls `Manager::Update()` / `DispatchInput()` / `Render()`.
- Reads from `GameClient` state via `BindingContext` — no hand-written glue per screen.
- The `UIEditor` panel uses the same `UIRenderer` to preview layouts.

**ImGui coexistence:**
- Both systems share an `InputCaptureFlag` per frame
- When ImGui modal is open or `WantCaptureMouse/Keyboard` is set, UI library widgets receive no input
- When the UI library has captured input, ImGui's want-capture flags are forced false
- Render order: 3D → UI library → ImGui (ImGui draws over UI library, ensuring editor panels stay accessible)

---

## Implementation milestones

Each milestone ends with something testable. Don't start M(N+1) until M(N) demonstrably works.

### M1 — Widget foundation ✅ shipped 2026-05-06

- Widget base, tree, lifecycle (`OnAttach` / `OnDetach` / `OnUpdate` / `OnDraw` / `OnInput`) — `Onyx/Source/UI/Widget.{h,cpp}`
- `Rect2D` + layout-math primitives — `Onyx/Source/UI/Rect2D.h`, `Color.h`
- Input event abstraction (mouse, keyboard, modifier flags, double-click, long-press, hover-intent, drag threshold) — `Onyx/Source/UI/InputEvent.h`
- Focus / hover / capture state machine (with `OnCaptureLost`) — `Onyx/Source/UI/WidgetTree.{h,cpp}`. Hover/press tracked at the topmost interactive ancestor per Q15.
- Screen + overlay stack — `Onyx/Source/UI/ScreenStack.{h,cpp}` with layer enum (World / HUD / Modal / Popup / Tooltip / Toast / Drag)
- Z-order resolution + layer hierarchy — z-order attribute overrides tree-order within a parent; clip-escape widgets routed by ScreenStack to the topmost layer.
- `UILayer` slotted into frame loop — owned by `Application`, ticks between user layers' OnUpdate (3D) and ImGui::Begin
- ImGui input-capture coordination — UILayer reads `ImGuiIO::WantCaptureMouse/Keyboard` each frame; Manager drops platform mouse/keyboard events when set.

**Test:** Editor smoke test passed (clean launch, ~12 s with no errors). Manual interactive scene + Tooltip clip-escape demo deferred to land alongside M2 (UIRenderer) — M1 alone has no visible output.

### M2 — Render path (batched + clipping) ✅ shipped 2026-05-06

- `UIRenderer` (`Onyx/Source/UI/Render/UIRenderer.{h,cpp}`) with batched colored / gradient / textured quads + 9-slice + rounded rects via corner-fan tessellation
- Single batched buffer; flush on texture change. Currently ~1 draw call per scissor region; full ≤5/frame target validated as we scale up
- Scissor stack with depth 16 — overflow logs warning + ignores; pop returns to previous state cleanly
- Premultiplied alpha throughout (`vec4(rgb * coverage, coverage)`); blend mode `(GL_ONE, GL_ONE_MINUS_SRC_ALPHA)`
- White-pixel texture for solid quads keeps the shader uniform
- `DebugRect` widget (`Onyx/Source/UI/Widgets/DebugRect.{h,cpp}`) renders a rounded panel with hover/press visuals — used as the M2 smoke widget

**Mask textures** (non-rect clip) deferred to when a widget needs them — the design's automatic scissor→mask fallback is straightforward to add when the first consumer (circular minimap, hex portrait) lands. Pool allocators deferred — pre-allocated 64k-quad buffer is sufficient until profiling shows otherwise.

**Test:** Editor smoke test passed. Test screen with a clipping panel containing an inner rect shows in the editor; clean run for 14 s. Visual interactive verification (hover changes color, click logs) deferred to M9 when full scenes ship.

### M3 — Text system ⚠️ shipped 2026-05-06 (visually unverified)

**Code path complete and the bake works**, but glyphs aren't appearing on screen yet — needs investigation in the next session.

- `msdfgen` via vcpkg manifest (`extensions` feature only — FreeType + libpng, no Skia).
- `Onyx::UI::FontAtlas` (`Onyx/Source/UI/Text/FontAtlas.{h,cpp}`) — runtime baker + loader. First run generates the Latin atlas from `Resources/Fonts/Roboto-Medium.ttf` using msdfgen + a shelf packer; caches to `Resources/Fonts/Roboto-Medium.atlas.bin`. Verified writes a 3 MB binary with 319 glyphs at 1024×1024 RGB, 32 px bake, 4 px range.
- MSDF fragment shader (~10 lines) inline in `UIRenderer.cpp` — sample → median → screen-pixel-distance → coverage.
- `UIRenderer::DrawText` / `MeasureText` — UTF-8 decode (1/2/3-byte), per-glyph quads, alignment.
- `Onyx::UI::Label` widget.

**What's broken / what to investigate next session:**
1. With a temporary render-order swap to put UI on top of ImGui, the colored `DebugRect`s render but `Label` glyphs don't appear. Cause unconfirmed — possible: Y-axis sign in the plane-bounds conversion (`out.planeMin = (g.planeL, -g.planeT)`), atlas-data correctness (msdfgen `SDFTransformation` projection units), or shader uniform `u_PxRange` not propagating into the text batch.
2. `UIRenderer::Flush` crashes inside `nvoglv64.dll` (access violation reading 0x0) when navmesh debug view is enabled. Suspected cause: GL state interference between navmesh debug rendering and `glDrawElements`. Mitigation: removed the test scene from `Editor3DLayer` so the main `Manager` has no active screen → `Flush` early-returns on empty geometry. Pending user verification that this stops the crash.

**Path forward — UIEditor panel preview:**

Rather than continue debugging via the render-order swap, add a `UIEditorPanel` (M12 first cut) that hosts a `WidgetTree` inside an ImGui-backed framebuffer. Its own `UIRenderer` instance, isolated from main render, gives a stable visual sandbox to debug text rendering AND moves us toward M12 deliverable.

- `MMOGame/Editor3D/Source/Panels/UIEditorPanel.{h,cpp}` — orphan header exists; `.cpp` to write next session.
- Architecture: own `Onyx::Framebuffer` + `Onyx::UI::UIRenderer` + `Onyx::UI::WidgetTree`. ImGui::Image of the framebuffer color attachment. Mouse coords forwarded from `ImGui::IsItemHovered + GetMousePos - imageOrigin`.

**Deferred to later passes:** font fallback chain, missing-glyph `.notdef`, dynamic glyph page, kerning, multi-line wrapping, text vertex cache.

### M4 — Layout ✅ shipped 2026-05-06

- `Onyx/Source/UI/Layout.h` — `LayoutSpec` (Fixed / Fill / Percent / Aspect modes, min / max constraints, anchor + offset, flexGrow, padding, gap), `ContainerMode` (Free / StackHorizontal / StackVertical), `AnchorEdge` (9-position).
- Layout pass in `WidgetTree::PerformLayout` — runs at the start of `Update()`; root auto-fits to viewport unless explicit bounds were set; depth-first recurse.
- Stack containers split slack between flex children proportionally to `flexGrow`. Cross-axis defaults to widget's resolved size; `Fill` mode stretches.
- Free containers position children via `anchor + offset` (9 anchor edges).
- Convenience builders on `Widget` (`FixedSize` / `FillSize` / `Anchor` / `Padding` / `Gap` / `FlexGrow`) for readable C++ scene construction.
- Content-driven sizing for `Label` deferred until M3 ships text.

**Test:** Editor test scene rebuilt with a Stack panel anchored top-left containing two flex children (one fixed-height + Fill width, one flex-grow=1 fully filling). Viewport-aware: panel stays at (40, 60) regardless of window size; layout reruns on every Update so resizes reflow automatically. Build + 10 s editor smoke run passed clean.

### M5 — `.ui` loader + includes
- pugixml integrated as vendor lib
- Widget factory registry
- Attribute parser (color, units, anchor, expression, file paths, **socket names → `MMO::SocketId` via `SocketNameToId`**)
- `<include>` parsing with parameter passing and cycle detection
- File watcher → per-file subtree reload
- Round-trip preservation
- Error reporter with line numbers + context
- Near-miss warning when a socket attribute value doesn't match any well-known name (hint at `SocketIdToName` enum entries)

**Test:** the M4 mock screen rebuilt as a `.ui` file using one `<include>`. Edit the file mid-run; subtree reloads. Save the parsed tree back; diff vs original is empty. Create a circular include; load-time error names the cycle. `socket="OverheadName"` resolves to `SocketId::OverheadName`; `socket="MuzleR"` (typo) loads as Custom with a near-miss warning.

### M6 — Theming + animation + audio + accessibility
- JSON theme parser
- Style cascade (class + pseudo-state); resolution order explicit > class > pseudo-state
- Pseudo-states: `:hover` `:pressed` `:focused` `:disabled` `:checked` `:pending` `:stale` `:drop-target` `:drop-reject`
- Semantic color tokens
- Tween system + easing library
- State-transition animations
- Audio event publishing (theme + attribute paths) → `UI` audio bus
- Global UI scale multiplier
- Reduced-motion mode
- Focus-indicator visibility guarantee
- High-contrast theme variant shipped

**Test:** hot-reload theme color mid-frame; button animates to hover state over 80 ms. Toggle UI scale 1.0 → 1.5; layout reflows, fonts re-rasterize. Toggle reduced-motion; decorative animations stop, functional ones continue. Hover button; UI bus event fires (verifiable via mock subscriber).

### M7 — Bindings + sandboxing
- Expression parser (lookup, arithmetic, comparison, logical, member access)
- Pre-evaluated expression trees
- `BindingContext` registry with per-field dirty-flag plumbing
- Computed binding cache (per frame, invalidated on source dirty)
- `foreach` with keyed reconciliation + pre-cull
- `if` directives
- `Pending<T>` / `Stale<T>` value wrappers + pseudo-state mapping
- Event handler registry by name
- Iteration cap (10,000) with warning
- Update phase ordering enforced

**Test:** `player_frame.ui` with `{player.hp}`, `{player.name}`, `foreach aura in player.auras`. Bound to a fake `LocalPlayer`. Wrap a binding in `Pending`; widget renders `:pending` state. Force foreach over 50,000 items; capped at 10,000 with warning. Insert duplicate `key=` in a foreach; warning fires and the duplicate run falls back to index-based diff.

### M8 — Drag and drop system
- Drag threshold, drag-arm state machine
- Drag types + drop-accept registries
- `DragGhost` rendering (default + override path)
- `:drop-target` / `:drop-reject` pseudo-states
- Modifier-flag surfacing on drop event
- Cancel via Escape, snap-back animation

**Test:** two `<itemicon>` widgets and three `<actionslot>` drop targets (one accepts `item`, one accepts `spell`, one accepts both). Drag item → all `item`-accepting slots highlight. Drop on accepting slot fires handler with source data + modifier flags. Shift+drop sets `split=true`. Escape mid-drag cancels with snap-back.

### M9 — Built-in widget library + virtualized ScrollView
- All built-ins: `Container` `Stack` `Grid` `ScrollView` `Spacer` `TabView` `Label` `Image` `Icon` `ProgressBar` `RoundedPanel` `Divider` `Button` `Slider` `Checkbox` `Dropdown` `TextInput` `RadioGroup` `Tooltip` `Modal` `Popup` `Toast` `ContextMenu`
- ScrollView virtualization (render-only-visible + recycling pool + scroll smoothing + lazy height cache)
- Tooltip with hover-intent delay + clip-escape rendering
- Modal with `PushOverlay` integration
- The proof-widget vertical slice from the checklist

**Test:** the checklist test verbatim — HP bar drains over 5 s, theme hot-reloaded mid-run, FloatingText spawns on click, focus moves between buttons. Plus: `ScrollView` with 10,000 items scrolls smoothly, only ~30 widgets exist at once. Tooltip on a button inside a scrollview inside a modal renders above all of them.

### M10 — Localization
- `tr:key` resolution against active locale
- Locale file parser (JSON)
- ICU-subset MessageFormat: `{name}` substitution + `{name, plural, ...}` cardinal plurals
- Fallback chain: active locale → default locale → key string
- Locale file watcher; locale switch hot-applies
- Locale picker dependency for UIEditor (M12)

**Test:** ship two locales (en, es). Toggle between them at runtime; all `tr:` text refreshes. Remove a key from `es`; widget falls back to `en`. Remove from both; renders the key string visibly. Plural form tested with 0, 1, 2, 5 counts.

### M11 — World-attached widgets

Consumes the engine's socket and projection systems. UI library work is mostly the widget side; engine work is prerequisite.

**Engine prerequisites** (not part of UI library scope; status as of 2026-05-06):

| Status | Item | Location |
|---|---|---|
| ✅ | `Onyx::Attachment` / `AttachmentSet` | `Onyx/Source/Graphics/Attachment.h`, `.cpp` (commit `2e393fa`) |
| ✅ | `MMO::SocketId` enum + `SocketNameToId` / `SocketIdToName` | `MMOGame/Shared/Source/Model/SocketId.h`, `.cpp` |
| ✅ | `.omdl v3` with ATCH attachments table | `MMOGame/Shared/Source/Model/OmdlFormat.h` (`OMDL_VERSION = 3`) |
| ✅ | `OmdlReader` / `OmdlWriter` v3 support | `MMOGame/Shared/Source/Model/OmdlReader.cpp`, `OmdlWriter.cpp` |
| ✅ | FBX importer detection of `socket_<Name>` empties (Model + AnimatedModel) | `Onyx/Source/Graphics/Model.cpp`, `AnimatedModel.cpp` |
| ✅ | Editor3D exporter populates attachments in `.omdl` writes | `MMOGame/Editor3D/Source/World/EditorWorldSystem.cpp` |
| 🔨 | **`AnimatedModel::GetMeshSpaceBonesPtr()`** — accessor for `globalAnim[i]` (mesh-space animated bone transforms, distinct from skinning matrices). Animator caches the array during `CalculateBoneTransform` alongside the skinning matrices. Required by `AttachmentSet::ResolveWorld` for animated sockets. Currently `AnimatedModel::GetBoneMatricesPtr` returns *skinning matrices* (`globalInverse * globalAnim * offset`) — the header at `AnimatedModel.h:141-146` flags this gap. | `Onyx/Source/Graphics/AnimatedModel.h`, `.cpp`; `Animator.cpp` |
| 🔨 | **`Animator::GetTickId()`** — counter incremented per `Update()`. Forms part of the projection cache key; static models report 0 forever. | `Onyx/Source/Graphics/Animator.h`, `.cpp` |
| 🔨 | **`WorldUIAnchorSystem`** — engine subsystem: project world point → screen, cache by `(cameraTickId, entityId, socketSlot, animTickId)`, frustum + distance cull, support `Track` and `Latched` anchor modes. Owned by `Application`; `SceneRenderer::BeginScene` bumps `cameraTickId`. Used by UI library + FX + 3D audio. | `Onyx/Source/Graphics/WorldUIAnchorSystem.h`, `.cpp` (new) |
| 🔨 | **Debug overlay** — colored dot per socket on selected entity, labeled by name. Validates the chain end-to-end before consumers. Reused by UIEditor (M12). | Engine + Editor3D integration |

**UI library work (M11 itself):**
- Widget attribute parsing for `entity`, `socket`, `socket-mounted`, `anchor-mode`. Strings resolve to `MMO::SocketId` at parse time via `SocketNameToId`. Widgets store `SocketId` (1 byte). Custom names store both `Custom` ID and the original name string.
- Built-in widgets: `Nameplate`, `OverheadBar`, `FloatingText`, `WorldMarker`
- Built-in fallback ladder hardcoded in overhead widgets: `socket-mounted (if mounted) → socket → AABB top + padding`
- `Track` mode: per-frame `WorldUIAnchorSystem::Project` call
- `Latched` mode: project once at attach, store screen anchor, animate in screen space
- `FloatingText` imperative spawn API: `m_UI->SpawnFloatingText({.entity, .socket, .anchorMode, .text, .color, .lifetime, .animation})`
- GPU-instanced bar geometry — quad mesh stored once; per-instance buffer holds {screenPos, size, hpRatio, color, alpha}; one draw call for all overhead bars regardless of count
- Render-through-walls convention (occlusion off by default)

**Test:** place 100 creature spawns with mixed states (idle, walking, in combat, mounted). Verify:
- All 100 nameplates render at correct overhead positions; mounted entities use `OverheadMounted` socket
- UI cost ≤0.5 ms with all 100 visible (after frustum cull + instancing); ≤0.2 ms with 20 visible
- `WorldUIAnchorSystem` cache hit rate: ≥90% on idle entities, ~50% on walking entities
- Single draw call for all overhead bars regardless of count
- Damage `FloatingText` (latched) does NOT jitter when camera or entity moves
- Creature without sockets (legacy model) falls back to AABB-top anchor; nameplate still appears in a sensible place
- Custom socket on a boss model resolves correctly via name lookup

### M12 — `UIEditor` workspace in MMOEditor3D

The UIEditor is a **workspace mode** of MMOEditor3D, not a single panel — when activated, the docking layout reflows to a UI-authoring preset. Primary navigation is the **stage selector** (Q16): pick a stage and the canvas mounts that stage's screens with the right theme and mock; the file browser is secondary.

#### Workspace mockup

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────┐
│ MMOEditor3D                                            [Workspace: World │ ▶UI │ Terrain │ Animate]│
├────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ Stage: [In-Game + Inventory ▼]   Mock: [LowHP-Combat ▼]   Theme: [Dark ▼]   Locale: [en ▼]        │
│ Res:   [1920×1080 (16:9) ▼]   ☑ Safe area   ☐ Z-X-Ray   ☐ Overflow   ⏵ Live: MMOClient connected │
├──────────────────────────┬───────────────────────────────────────────────────┬─────────────────────┤
│ HIERARCHY            ⋮   │                                                   │ INSPECTOR        ⋮  │
│                          │                                                   │                     │
│ ▾ Stage: HUD+Inv         │  ┌────── Canvas — 1920 × 1080 ──────────────┐    │ <button>            │
│   ▾ root                 │  │                                            │    │ id        close-inv │
│     ▸ player-frame       │  │ ╔════════════════════════════════════════╗ │    │ class     btn       │
│     ▸ target-frame       │  │ ║      [HUD chrome + open inventory      ║ │    │ anchor    TopRight  │
│     ▸ action-bar         │  │ ║       modal + tooltip on a buff icon] ║ │    │ offset    8,8       │
│     ▸ chat               │  │ ║                                        ║ │    │ size      32,32     │
│   ▾ overlay: Inventory   │  │ ║      safe-area frame (dotted)          ║ │    │ text      tr:close  │
│     ▾ inv-frame          │  │ ║                                        ║ │    │ onclick   close-inv │
│       ▸ item-grid        │  │ ║      z-x-ray (off): widgets render     ║ │    ├─────────────────────┤
│       ▸ close-btn ●      │  │ ║      naturally; (on): each layer       ║ │    │ STYLE CASCADE       │
│   ▾ tooltip layer        │  │ ║      tinted distinctly                 ║ │    │  .btn               │
│     ▸ tooltip:buff-1     │  │ ╚════════════════════════════════════════╝ │    │    bg     #3a7bd5   │
│                          │  │                                            │    │    pad    8         │
│ ── COMPONENTS            │  │  Selection handles + snap-to-edge guides   │    │  .btn:hover         │
│   • unit_frame.ui        │  └────────────────────────────────────────────┘    │    bg     #5a9bff ★ │
│   • tooltip.ui           │                                                   │  inline             │
│   • item_icon.ui         │  [⊟ Fit]  [100%]  [50%]  [Custom %]   100%        │    text   tr:close  │
│   • drag_ghost.ui        │                                                   │                     │
├──────────────────────────┴───────────────────────────────────────────────────┴─────────────────────┤
│ PALETTE     [▾ Layout]    Stack  Grid  Container  ScrollView  Spacer  TabView                      │
│             [▾ Display]   Label  Image  Icon  ProgressBar  RoundedPanel  Divider                   │
│             [▾ Input]     Button Slider Checkbox Dropdown TextInput RadioGroup                     │
│             [▾ Composite] Tooltip Modal Popup Toast ContextMenu                                    │
│             [▾ World]     Nameplate OverheadBar FloatingText WorldMarker                           │
├────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ FILES   │   THEMES   │   LOCALES   │   BINDINGS MOCKS   │   SOCKETS DEBUG                          │
│ assets/ui/inventory/inventory.ui — modified ●                                                      │
└────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

#### Region breakdown

**Top toolbar — the stage simulator chrome:**
- **Stage selector** — picks composed view (Login / CharSelect / In-Game / In-Game+Inventory / Combat-LowHP / etc.). Drives the canvas root by mounting the stage's screen + overlays per `manifest.json`.
- **Mock profile** — selects the active mock JSON for binding preview. Reusable across stages (`mocks/low_hp_combat.json`).
- **Theme** — switches active theme JSON; live cascade re-resolves.
- **Locale** — switches active locale; `tr:` keys re-resolve.
- **Resolution** — locked-aspect canvas; sizes per `manifest.json` resolution list (16:9, 21:9, 4:3, mobile-portrait).
- **Safe-area toggle** — dotted inset frame at the resolution's safe-area distance.
- **Z-X-Ray toggle** — color-codes layers (HUD/Modal/Popup/Tooltip/Toast/Drag); makes overlap and clipping issues visible at a glance.
- **Overflow toggle** — shows widgets anchored outside the visible canvas (off-screen safe-area diagnostics).
- **Live indicator** — green when MMOClient is running with the same `manifest.json`; saves trigger client hot-reload.

**Hierarchy (left top)** — the composed widget tree of the current stage. Top-level entries are stage root + each pushed overlay + tooltip layer; lets you debug stacking by inspecting the actual draw order.

**Components (left bottom)** — `<include>` files browser. Drag-to-canvas inserts an `<include>` element; double-click drills into the component source.

**Canvas (center)** — actual `UIRenderer` rendering the stage at locked aspect. Selection handles, snap-to-edge guides (parent edge, sibling edge, percent, grid), 9-position anchor preset visualizer (Godot Control style).

**Inspector (right top)** — type-aware property editors per attribute. Anchors get a 9-position grid widget; colors get a swatch picker; socket attributes get an autocomplete dropdown from `MMO::SocketId`.

**Style cascade (right bottom)** — for the selected widget, shows which class / pseudo-state / inline override contributed which value. Overrides marked `★`. Lets designers debug "why is this red" without grep'ing themes. (Pattern from Unity UI Builder.)

**Palette (bottom strip)** — categorized widget toolbox. Drag-to-canvas creates an instance with sensible defaults. (Pattern from Unreal UMG.)

**Bottom tabs** — secondary modes that don't deserve permanent screen space: Files (file browser), Themes (theme JSON editor), Locales (locale file editor), Bindings Mocks (mock profile editor), Sockets (socket debug overlay when world-attached widget selected).

#### Deliverables

- Stage selector top-bar + manifest-driven mounting
- Multi-resolution canvas + safe-area frame
- Z-order x-ray mode (color-coded layer overlay)
- Mock profile dropdown + per-screen + reusable JSONs
- Live-client connection indicator + bidirectional file watcher coordination
- Canvas viewport (uses real `UIRenderer`)
- Selection handles + move/resize gizmos with snap (parent edge, sibling edge, percent, grid)
- Tree outliner (drag-reorder, multi-select, search/filter, virtualized for >200 widgets)
- Widget palette (drag/drop) with Components tab listing `<include>` files
- Property inspector (type-aware editors per attribute, debounced 200 ms)
    - **Socket attributes**: dropdown autocompletes from `MMO::SocketId` enum at compile time; "Custom..." entry opens text field; custom values badged as `(custom)`
- Theme picker / live preview
- Binding mock panel
- Locale picker
- **Socket debug overlay**: when a world-attached widget is selected, the canvas viewport renders the bound entity model with all its sockets as colored dots, labeled by name. Reuses the engine debug overlay from M11 prerequisites.
- File browser (`.ui` + theme + locale files)
- Undo/redo integrated with Editor3D's existing undo stack
- Round-trip save (relies on M5 round-trip preservation)

**Test:** open the editor in UI workspace mode. Pick stage `In-Game + Inventory + Combat-LowHP`. Canvas shows HUD with low HP, inventory modal, and a tooltip — all composed in actual z-order. Toggle Z-X-Ray; layers tint distinctly. Toggle resolution to 21:9; safe-area frame moves; anchored elements re-flow. Drag a `Button` from palette into the inventory modal; edit `text`, `class`, `onclick` in inspector; save. With MMOClient running, the new button appears live (file watcher hot-reload). Switch locale; `tr:` text refreshes. Open a 500-widget `.ui`; tree remains responsive. Select an `<overheadbar entity="{npc}">`; canvas shows the npc model with sockets labeled.

After M12, the library + editor are complete. Phase 1 of the tech demo unblocks.

---

## Recommended pre-M1 sequence

Before starting M1, the engine prerequisites for M11 should land — they're independent of the UI library milestones and unblock multiple downstream consumers (FX, 3D audio, equipment scene-graph). Specifically:

1. **`AnimatedModel::GetMeshSpaceBonesPtr()`** — small isolated change; cache `globalAnim[i]` alongside the skinning matrix evaluation, expose a getter. Currently the missing piece called out by the commit message of `2e393fa` and the comment at `AnimatedModel.h:141-146`. (~30-60 lines.)
2. **`WorldUIAnchorSystem`** in `Onyx/Source/Graphics/` — the projection + cache + cull + Track/Latched layer. Standalone subsystem. (~200-400 lines.)
3. **Debug overlay** for socket visualization — a small render mode that draws labeled dots at every socket on a selected entity. Validates the entire chain (importer → `.omdl` → load → resolve → project → render) before any UI consumer touches it.

These three engine deliverables cleanly precede UI library M1 and don't conflict with it. M1-M10 can then proceed without depending on any of them; M11 picks them up when both reach the same point.

---

## Testing strategy

Per-milestone manual scene only. The "Test:" section in each milestone is the validation script — run editor + client, exercise the documented scene, eyeball the result. No unit-test framework, no CI gates, no PNG baselines, no perf regression bench. Visual / perf regression is deferred until something needs guarding; the design supports adding them later (offscreen render path is in place from M2, projection-cache hit-rate counter from M11).

The milestone tests collectively cover:
- Layout math + flex / anchor reflow (M4)
- Expression parser + dirty propagation (M7)
- Hot-reload state preservation — focus / scroll / animation cancel (M5, M6)
- Input event routing + ImGui handoff (M1)
- Drag-drop sequences — start, hover, drop, cancel (M8)
- Socket resolution against idle / animated / mounted / missing-socket cases (M11)
- Locale lookup + plural resolution (M10)

---

## Not included

These are out of scope by design — either huge separate projects, or counter to design decisions made above.

- **HarfBuzz / RTL (Arabic, Hebrew) / Indic shaping.** Months of integration. Adding it would be a separate localization project.
- **Gamepad input.** Input event struct shaped to accept it cleanly if added; no controller backend ships here.
- **Player UI customization / addon API.** WoW-style addon ecosystem is its own project; would also require tighter sandboxing than current.
- **Vector graphics (SVG).** Use sprite atlases or shader primitives.
- **3D widget transforms.** UI is 2D screen-space; world-attached widgets are still 2D quads facing the camera.
- **IME for CJK typing.** Display works (atlas covers it if added); typing input doesn't.
- **Function calls in expressions.** Bindings are pure data access + arithmetic. Logic lives in C++ event handlers.
- **CSS-style specificity rules.** Resolution is deterministic: explicit > class > pseudo-state.
- **Full ICU MessageFormat (selectordinal, date, currency formatting).** Library uses an ICU subset.
- **Network-aware UI plumbing.** Pending/Stale wrappers + visual conventions are provided; the wrap/unwrap logic lives in `GameClient`, not the UI library.
- **Server-authored sockets.** All sockets come from `.omdl` (artist-authored). Servers can reference sockets by ID (`MMO::SocketId`) in packets, but cannot create new sockets at runtime.

---

## Alternative considered: RmlUi

[RmlUi](https://github.com/mikke89/RmlUi) is a mature C++ HTML+CSS UI library (MIT, 15+ years, used in shipping games — Hellpoint, Stunt Rally, others). Adopting it would replace ~70% of M1-M9 with a vendor library plus a ~500 LOC OpenGL backend.

**For:** faster to ship, battle-tested, big community, handles edge cases we'd grow into (CJK shaping, accessibility hooks, RTL).

**Against:** ~50k LOC dependency, different naming conventions (won't match `m_PascalCase` / `PascalCase`), less control over performance tuning (the 0.2-0.5 ms target is harder through someone else's abstractions), harder to integrate tightly with `GameClient` state, brings its own opinions about input dispatch and event lifetime, no editor that integrates with MMOEditor3D, no socket-attachment integration with the new `Onyx::Attachment` system.

**Decision: build our own.** Rationale: tight engine integration, full performance control, alignment with project naming/style, custom editor that lives inside MMOEditor3D, and the broader engine philosophy of building the core systems we want to own.
