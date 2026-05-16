# UI Library — Pending Human Validation

Items that were root-caused, fixed, built, and pushed but still need a **human
on-machine check** (I can't eyeball pixels or drive the interactive repro
headlessly). Branch: `feature/spawn-pack-trigger-and-doc-fixes`
(commits `79af616`, `90d66be`, `9bfdeeb`, `e46e121` — on origin).

Created 2026-05-16. Delete each box when confirmed; delete this file once all pass.

## 1. M3 text actually renders (commit `79af616`)
- [ ] Run `build\windows-debug\bin\MMOEditor3D.exe` from repo root. First run
      rebakes the atlas (~1–2 s, prints `[UI] baked N glyphs ...`).
- [ ] Confirm `Resources/Fonts/Roboto-Medium.atlas.bin` regenerated with a
      **v2** header (size ~3.16 MB).
- [ ] Open the **UI Editor** panel (or any screen with a `Label`). Glyphs must
      be **visible, upright, and crisp** at small *and* large `pxSize`.
- [ ] Spot-check alignment (left/center/right) and a descender glyph (g, y, p).

## 2. NavMesh debug view + UI no longer crashes (commits `90d66be`, `e46e121`)
- [ ] Load a map, **File ▸ Bake NavMesh**, then enable **View ▸ NavMesh**.
- [ ] With the overlay on, open/drive the UI Editor (forces `UIRenderer::Flush`
      while the navmesh VAO would previously have leaked).
- [ ] Confirm **no `nvoglv64.dll` access violation** over a sustained run
      (toggle NavMesh on/off repeatedly while UI renders).

## 3. M12 UIEditor WIP is functionally stable (uncommitted)
- [ ] It builds. Click-through the stage simulator chrome + the "Login stage"
      (Button + TextInput) — confirm it's stable enough to commit.
- [ ] Decide commit scope: it's entangled with non-UI changes
      (`StatusOverlay`/`NavMeshBakeService`/`ModelExporter`) in
      `Editor3DLayer` + `Editor3D/CMakeLists.txt`, so it cannot be a clean
      UI-only commit. Choose: untangle hunks, or land non-UI bits as their own
      commit.

## 4. Doc-correction sanity (commit `9bfdeeb`)
- [ ] Confirm the corrected CLAUDE.md is accurate to how you actually work:
      Windows Desktop tree + origin is canonical; WSL `/home/god/git/Onyx`
      (`master`, ~2026-05-04) is abandoned for this stream; the old
      `rsync --delete` must not be run.

## 5. No regression from the 4 commits
- [ ] Full build is green (verified). Smoke-run `MMOEditor3D` and `MMOClient`
      to confirm nothing else broke.

## 6. New M8/M9/M11 widgets (built + pushed, NOT visually validated)
Commits `79af616`..`4a54849` add the demo-critical widgets. Each compiles +
links (verified); none has been seen on screen. Note: text-bearing widgets
inherit the M3 caveat — confirm M3 text first (item 1) or they read blank.
- [ ] **ProgressBar** — HP/cast: fill tracks value, border/label correct.
- [ ] **Image** — Stretch/Fit/Fill (Fill clipped); loading-screen art.
- [ ] **Icon** — atlas region; disabled-dim.
- [ ] **ScrollView** — wheel scrolls, scissor-clips, thumb tracks; quest tracker.
- [ ] **Modal** — dims screen, centers panel, grabs input, Escape/backdrop close.
- [ ] **FloatingText** — rises+fades at a world point, despawns (damage numbers).
- [ ] **Nameplate** — name + HP bar tracks a head, hides off-screen/culled.
- [ ] **WorldMarker** — quest !/? badge tracks a world point (bob).
- [ ] **CooldownOverlay / DrawPie** — clockwise radial sweep + countdown,
      clipped to the slot.
- [ ] **M8 DragSlot/DragContext/DragGhostLayer** — drag an icon, ghost
      follows, drop on a compatible slot fires handler with modifier flags,
      drop-outside snaps back.
- [ ] Wire one demo screen (e.g. a stage in UIEditor / MMOClient) that
      exercises these so the validation is real, not synthetic.
