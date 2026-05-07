# Onyx — Tech Demo Checklist

Stripped-down roadmap for the first end-to-end demo. The goal is a one-button export-and-play scenario authored entirely in MMOEditor3D and playable in MMOClient via "Run Locally".

## Demo flow (what the designer authors)

1. Player spawns at a designer-placed PlayerSpawn.
2. Sees an NPC with `!` overhead indicator.
3. Talks to NPC → accepts a kill quest.
4. Walks to a designer-placed trigger volume → monsters spawn.
5. Uses an action-bar ability to kill them.
6. Returns to NPC → quest completes → NPC's portal opens.
7. Enters portal → loads dungeon instance → fights a boss.

## Already in place (no work needed)

- Editor: terrain authoring, object placement (spawn / trigger / portal / static / lights / particles / sound), gizmos, undo/redo, autosave
- Editor: "Run Locally" exports `Data/`, applies migrations, spawns LoginServer + WorldServer + Client subprocesses
- WorldServer: spell + aura + combat + loot + AI (`DataDrivenAI`), trigger volume system, C++20 scripting architecture, `creature_spawn` + `player_create_info` DB loaders
- Client: isometric camera, networked entity rendering with full 3D position + orientation, login → character select → enter-world flow
- LoginServer: account creation, auth, session token, character creation with race/class
- Network protocol packets (`EntityState`, `S_EnterWorld`, `S_EntitySpawn`, `S_EntityUpdate`)

## Demo phases (ordered by testability)

Each phase ends in something you can run, watch, and evaluate. Don't start phase N+1 until phase N is observably working — every phase delivers a playable milestone.

The dependency chain is what drives the order: HUD/menus need a UI library, abilities are pointless without enemies that fight back, enemies can't fight back without pathing, pathing needs a navmesh, and so on.

---

### Phase 0 — Engine UI library + visual editor

Foundation for every shipping HUD, menu, and panel. **Data-driven** retained-mode library with a **visual editor in MMOEditor3D** — layouts in `.ui` markup files, themes in JSON, locale strings in JSON, all hot-reloadable. ImGui stays but only for editor panels and dev overlays. See [docs/ui-library-design.md](ui-library-design.md) for the full design and the M1-M12 milestone breakdown.

Phase 0 ships the complete library + editor as the definitive design — no v1/v2 split. All capabilities (state-flow rules, z-order + clip-escape, drag-and-drop, virtualized scrolling, font fallback, localization, accessibility, audio hooks, fail-soft error handling, hot-reload lifetime semantics, sandboxing) and all performance commitments (batched draws, per-field dirty bindings, projection cache, frustum cull, GPU instancing, text vertex cache) are folded into the relevant milestones, not deferred.

Vertical slice (everything below ships before Phase 1 starts):

- [ ] **Widget foundation** — tree, lifecycle, screen + overlay stack, z-order rules, layer hierarchy (world / chrome / modal / popup / tooltip / toast / drag), input events with modifier flags + double-click + long-press + hover-intent + drag threshold, focus/hover/capture with `OnCaptureLost`, ImGui input-capture coordination
- [ ] **Render backend** — dedicated `UIRenderer`; single batched draw architecture (≤5 draws/frame), per-batch shader switching, scissor stack (depth 16), mask textures with automatic scissor→mask fallback, premultiplied alpha, pool allocators
- [ ] **Text system** — MSDF atlas (Latin + Cyrillic + Greek, multi-page), font fallback chain, missing-glyph `.notdef` fallback + codepoint logging, dynamic glyph generation for rare codepoints, text measurement API, kerning + line-height rules, text vertex cache
- [ ] **Layout system** — flex + anchor + offset, sizing (min/max/preferred/aspect/percent), content sizing for `Label`
- [ ] **Loader + includes** — XML `.ui` parser (pugixml), widget factory, attribute parser, `<include>` with parameter passing + cycle detection, file-watcher hot-reload, round-trip formatting preservation
- [ ] **Theming + accessibility** — JSON theme cascade with class + pseudo-state, semantic color tokens, global UI scale, reduced-motion mode, focus-indicator visibility guarantee, high-contrast theme variant
- [ ] **Animation + audio** — tweens with easing curves, state-transition animations, theme/attribute audio hook publishing to `UI` bus
- [ ] **Bindings + sandboxing** — expression parser, pre-evaluated AST, `BindingContext` with **per-field** dirty flags, computed binding cache, `foreach` with keyed reconciliation + pre-cull, `if` directives, `Pending<T>` / `Stale<T>` wrappers + pseudo-states, event handler registry, iteration cap (10,000), update-phase ordering
- [ ] **Drag and drop** — drag threshold, drag types + drop-accept registries, `DragGhost` (default + override), `:drop-target` / `:drop-reject` pseudo-states, modifier-flag surfacing, Escape-cancel + snap-back
- [ ] **Built-in widget library** — full set including virtualized `ScrollView` (render-only-visible + recycling + smooth scroll), `Tooltip` with hover-intent + clip-escape, `Modal`, `Popup`, `Toast`, `ContextMenu`, `TabView`, etc.
- [ ] **Localization** — `tr:key` lookup, locale files (JSON), ICU-subset MessageFormat (substitution + cardinal plurals), fallback chain (active → default → key string), locale watcher + hot-switch
- [ ] **World-attached widgets** — `Nameplate`, `OverheadBar`, `FloatingText`, `WorldMarker` with per-(camera, entity) projection cache, frustum + distance cull, GPU-instanced bar geometry
- [ ] **Error handling + lifetime** — fail-soft on missing texture/theme/handler/binding/glyph/locale, hot-reload preserves focus + scroll position + pooled widgets, in-progress drags cancel cleanly
- [ ] **`UIEditor` panel in MMOEditor3D** — canvas viewport (renders via real `UIRenderer`), selection + snap gizmos, virtualized tree outliner (drag-reorder + search), widget palette + Components tab, type-aware property inspector (debounced), theme + locale + binding mock pickers, file browser, undo/redo integrated with Editor3D, round-trip save
- [ ] **Testing infrastructure** — unit (layout math, expressions, dirty propagation), integration (offscreen render hash, hot-reload state, drag sequences), visual regression (PNG baselines), performance regression (CI alerts on >10% UI-cost rise)

**Test:** Open `ui/test/scratch.ui` in the `UIEditor`. Drag a `HealthBar` onto canvas; edit theme class, binding, and `tr:` text in the inspector; save. With MMOClient running in another window, the new bar appears live, drains over 5 seconds. `FloatingText` spawns on click. Theme color hot-reloaded mid-run; locale switched mid-run; both apply without restart. Place 100 creature spawns; UI cost ≤0.5 ms with all visible. Drag an item icon onto an action slot; modifier-flag-aware drop fires correct handler. ScrollView with 10,000 chat lines scrolls smoothly with only ~30 widgets in memory. Tooltip on button inside scrollview inside modal renders above all. Round-trip save preserves formatting and comments.

---

### Phase 1 — Mobs that walk and attack

Without this, abilities have nothing to target and combat is invisible.

- [x] **NavMesh** — editor bake step (Recast/Detour) → WorldServer consumption at startup. See [navmesh.md](navmesh.md).
- [ ] **Waypoint / patrol path** — placement in editor + execution in WorldServer
- [ ] **Engine animation state machine** — idle / walk / run / death states with transitions
- [ ] **Mob auto-attack** — basic melee swing when player is in range
- [ ] **Server-authoritative movement basics** — server is the source of truth for NPC positions on the wire
- [ ] **Health bars** (player + target) — consumes Phase 0 `HealthBar` widget; world→screen projection for over-head bars
- [ ] **Damage numbers** — consumes Phase 0 `FloatingText` widget; spawned on combat events

Mob entity, AI base + scripts, `creature_spawn` DB loader, and `CreatureTemplate` are already in place — see [docs/mmogame-server.md](mmogame-server.md). Phase 1 is purely about pathing, animation, and HUD wiring.

**Test:** Place a hostile creature spawn near a player spawn. Run Locally. The creature wanders its waypoint path. When you get close, it pathfinds to you and attacks. You watch your HP drop, both HP bars work, either of you eventually dies and the death animation plays.

---

### Phase 2 — Player can fight back with abilities

- [ ] **Engine animation state machine** — extend with attack / cast states + blend
- [ ] **Attachment points / sockets** — weapon attached to character hand
- [ ] **Action bar UI** in MMOClient — built on Phase 0 UI library (slot widget, drag/drop, keybind hooks)
- [ ] **Cast bar UI** in MMOClient — built on Phase 0 UI library (progress widget driven by server cast events)
- [ ] **Cooldown indicators** — radial sweep overlay on action-bar slot widgets
- [ ] **Ability / spell editor** in MMOEditor3D — author 3-4 player abilities + 1-2 mob abilities (replaces editing `AbilityData.cpp`)
- [ ] **Creature template editor** in MMOEditor3D — author HP, abilities, faction, loot table for the demo creatures (replaces editing `CreatureTemplate.cpp`)

**Test:** Author 4 player abilities and 3 creature templates in the editor. Run Locally. Engage a mob, cast all four abilities, watch FX + animations + damage numbers + cooldowns. Mobs cast back. Kill the mob and pick up loot.

---

### Phase 3 — NPC + quest loop

- [ ] **NPC role on spawn** — Inspector flag in editor: "quest-giver", linked to a specific quest entry
- [ ] **Quest editor** in MMOEditor3D — minimal kill-quest schema: target `creature_template`, count, reward (XP + 1 item)
- [ ] **Quest system** in MMOWorldServer — accept / progress tracking / complete / reward delivery / persistence per character
- [ ] **NPC gossip / dialogue** server-side
- [ ] **Quest-giver overhead indicator** — `!` available, `?` turn-in
- [ ] **Dialogue UI** in MMOClient — talk to NPC, accept, turn in
- [ ] **Quest tracker HUD** in MMOClient — current objective + count

**Test:** Place a quest-giver NPC, link a "kill 3 X" quest. Run Locally. NPC has `!` overhead. Talk → accept → tracker shows `0/3`. Kill 3 mobs (using Phase 1+2). Tracker updates each kill. Return → NPC has `?` → turn in → receive reward. Log out and back in — quest progress / completion persists.

---

### Phase 4 — Encounter design (triggers + portals + dungeons)

The pieces that make the demo a full scenario instead of a single encounter.

- [ ] **DB loader for `trigger_volume`** in MMOWorldServer
- [ ] **Trigger → "spawn creatures" action** — editor Inspector picks template + count; server handler spawns them on enter
- [ ] **DB loader for portals** in MMOWorldServer
- [ ] **Portal destination binding** — editor Inspector picks target map ID (placement already exists)
- [ ] **Instance map lifecycle** in MMOWorldServer — load dungeon on portal use, despawn on empty/timeout (single-player scope is fine for the demo)
- [ ] **Loading screen** between maps in MMOClient

**Test:** Build the full demo map. Run Locally. Walk into the trigger zone → mobs appear → kill them. Quest completes from Phase 3. NPC opens portal. Enter portal → loading screen → dungeon instance loads → fight boss.

---

### Phase 5 — Polish for shipping the demo

Optional but make the demo feel like a real game.

- [ ] **Audio** — hit / cast / death SFX, ambient on dungeon map
- [ ] **Death / respawn flow** in MMOClient
- [ ] **Better death/cast/attack animations** if the Phase 1+2 stand-ins were minimal
- [ ] **Quest reward inventory flow** — basic notification + item slot
- [ ] **Boss encounter pattern** — at least one telegraphed mechanic (ground AoE, charge, summon adds)

---

## Out of scope until after the demo lands

- **Social / economy:** chat, party, guild, mail, AH, trade, friends, currency, vendors, repair, item durability
- **Live-service ops:** structured logging, metrics, dashboards, GM commands, backups, crash reporting, player reports
- **Production hardening:** TLS, anti-cheat, rate limiting, brute-force protection (no adversarial concern in a closed demo)
- **Scale:** multi-realm, multi-shard, cluster coordination, hot-reload, phasing
- **Distribution:** Launcher / manifest / signing — "Run Locally" is the demo deploy
- **Multiple races/classes** — one is enough
- **Graphics polish:** bloom, HDR, ACES, atmospheric scattering, weather, day/night, decals, foliage, water shader, lightmap baking, IBL, reflection probes, CSM
- **Asset import pipeline expansion** — existing FBX path is fine
- **Editor polish:** snap-to-surface, billboard icons, minimap preview, weather-zone authoring, reflection-probe placement
- **Close-up character detail:** IK, blendshapes, cloth, character SSS, hair shader, eye shader (wasted at isometric angle)
- **Game-feature panels** beyond what Phase 3 requires: mail UI, vendor UI, AH UI, trade UI, friend list UI, chat window
