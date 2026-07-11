# UI Screenshot Capture

How the project's UI **still screenshots** are generated — automatically, headlessly, and without
ever touching save data. Built 2026-06-15.

> **No automated GIFs.** This pipeline captures still PNGs only. Animated GIFs are added **manually,
> one at a time** (Twilight's call, 2026-06-15): the headless tool's frame-sequence capture routines
> were removed, and the scripts/CI no longer auto-assemble GIFs. `scripts/make_gifs.py` (the Pillow
> assembler) is **kept for manual use** — run it by hand to turn your own captured frames into a GIF.

> **Capturing is not reviewing — you MUST look (2026-07-10, standing default).** On ANY UI / QML /
> screen / layout change, after capturing you must **open the PNG and scrutinise it yourself** (crop/zoom
> the changed area): hunt for overlaps (a field over the trainer art), misalignment, uneven spacing,
> clipping past a card/panel border, and overall polish — then fix what's wrong before calling the work
> done or telling Twilight it's ready. A glance is not a review; the trainer-card clock overlapped the
> artwork and shipped precisely because this step was skipped. Enforced in `CLAUDE.md` → Default Workflow
> step 1; see also `ui-patterns.md`.

The output is **not tracked** (it's regenerated on demand); it lands in `tmp/screenshots/`
(ignored by `tmp/.gitignore`, which ignores everything in `tmp/` except itself). Regenerate it
by default after a fast-forward of `main` (see `CLAUDE.md` → Default Workflow), or any time by
running the capture script.

## What gets captured

Over the populated fixture `assets/saves/natural-clean/BaseSAV.sav`:

- **Every top-level screen** — Home, Trainer Card, Bag, Pokémon, Rival, Pokédex, Pokémart, Maps,
  plus the About / File Tools / New File modals → `tmp/screenshots/screens/<name>.png`.
- **The "View All" overview drawers** (Pokémon + Bag) slid open → `screens/<name>_view_all.png`.
- **The Pokémon deep editor tabs** — General / DV-EV / Moves (with the Glance pane) →
  `editor/pokemon_editor_{general,dvev,moves}.png`.
- **Both text-editor modes** — the quick-edit popup and the modal full keyboard, plus the keyboard's
  tileset "tileviewer" → `editor/text_{quick_popup,full_keyboard,keyboard_tileset}.png`.
- **Hover state** — the full keyboard's font-tile preview tooltip (hover a real character pill to
  raise its `TilePreview`) → `editor/text_keyboard_hover_tile.png`. (No Pokédex hover shot — the
  Pokédex doesn't need one.)

All automated output is flat PNGs under `screens/` and `editor/` — the tool no longer writes any
`frames/` folders or `.gif` files. To add an animation, capture/record the frames by hand and assemble
them yourself (`scripts/make_gifs.py` is kept for exactly this — point it at a `frames/<name>/` folder
you populated), then drop the finished `.gif` wherever it's needed.

## How it works

One piece, repo-friendly and CI-friendly:

- **`projects/tests/tools/screenshooter.cpp`** — a small C++ tool (NOT a CTest test) built on the
  shared GUI harness `tests/helpers/guiapp.h`. It boots the **real** application UI headless on the
  `offscreen` platform — the exact engine/provider wiring the app uses (`brg`, the tileset + font
  image providers, `DB::qmlProtect`, the exe's `bootQmlLinkage`) — then navigates / instantiates /
  drives the live UI and grabs each shot with `QQuickWindow::grabWindow()`. CMake target
  `screenshooter` (in `projects/tests/CMakeLists.txt`, modeled on `pse_add_gui_test` +
  `gen_synthetic_fixtures`).

Driven by **`scripts/capture_screenshots.ps1`** (Windows dev kit) / **`scripts/capture_screenshots.sh`**
(Linux/Docker/CI): build the `screenshooter` target and run it. No GIF/assembly step.

## Byte-fidelity (sacred)

The tool **only ever reads** the save in memory to render the UI. It never calls
`saveFile()`/`flattenData()` and never writes a save byte. (See
`../context/principles.md` → "Save File Integrity Is Sacred".)

## Key technical notes (don't relearn these the hard way)

- **Capture at a fixed 750×480** — the small rectangle the UI is designed for (the app's own saved
  window size is ~751×480; 750 is just the round number). The `1130×740` in `guiapp.h` is only the
  fresh-profile default and made every shot look "too big". Override with `PSE_SHOT_SIZE="WxH"`.
  **Resize BEFORE `start()`** so the QML lays out at the final size exactly once — resizing *after* the
  initial layout left some `MultiEffect`-layered tiles (Home's greyed Maps/Options) unrendered.

- **Render through a REAL GPU window — NOT offscreen.** This is the big lesson. The app draws its QML
  through a GPU-backed `QQuickWidget`; the `offscreen` platform **silently drops every `MultiEffect`/
  layered item** (Credits cards, Home's greyed disabled tiles, shadows → washed-out screens). And it's
  not just the software backend: **offscreen with the default RHI was tested on Windows and STILL didn't
  render the effects** (the offscreen platform has no real swapchain). So the only way to get the
  effects is a real GPU window. The `.ps1` does NOT set `QT_QPA_PLATFORM`; the tool runs on the native
  platform and `main()` shows the `QQuickView` **frameless + `Qt::Tool`**, positioned **off the visible
  desktop** on Windows/macOS (`setPosition(-4000,-4000)`) so it renders on the GPU without flashing, and
  at `(0,0)` under xvfb/X so it stays on the virtual screen. `grabWindow()` renders the scene graph to
  an image regardless of the window being off-screen/occluded.
  - **CI:** `capture_screenshots.sh` runs under **`xvfb-run`** (a real X window + llvmpipe renders the
    effects, still headless). Only if `xvfb-run` is absent does it fall back to `offscreen` +
    `PSE_FORCE_SOFTWARE=1` — which runs but with **missing effects** (documented, last resort).
- **`QQuickView` doesn't size the Controls overlay → centered popups land bottom-right.** A plain
  `QQuickView` (unlike an `ApplicationWindow`/`QQuickWidget`) leaves `QQuickOverlay` at **size 0×0**,
  parked at the window centre. So `anchors.centerIn: Overlay.overlay` popups (the quick-edit name
  editor) had nothing to center against and rendered at the bottom-right. `fixOverlay()` finds the
  `QQuickOverlay` and sets its geometry to fill the window (called after show + before each popup grab),
  which makes the `centerIn` bindings re-evaluate and center correctly. (Diagnosed by logging the
  overlay/popup geometry — it read `QQuickOverlay … size 0×0` at the window centre.)
- **HiDPI → downscale to 1130×740.** On a scaled display `grabWindow()` returns PHYSICAL pixels (e.g.
  1695×1110 on a 150% screen). `grab()` downsamples every image to the window's LOGICAL size (1130×740)
  with a smooth transform, so output is a stable 1130×740 regardless of the dev machine's scaling (and
  the supersampled downscale is, if anything, crisper than a native DPR-1 render). `QT_SCALE_FACTOR`
  does NOT override the screen DPR, so don't rely on it — downscale the grab instead.
- **Fonts (offscreen only).** Under the offscreen fallback, Qt's FreeType font DB finds **no fonts**
  unless pointed at a font dir — else all UI text renders as tofu boxes (the in-game *bitmap* name
  preview still renders via the font image provider, which masked it at first). The tool sets
  `QT_QPA_FONTDIR` to the OS font dir (Windows `%WINDIR%\Fonts`, macOS `/System/Library/Fonts`) when
  empty; on the native platform the OS font DB is used, so this isn't needed. The harness allowlists the
  benign "Cannot find font directory" warning.
- **Driving the UI.** Top-level screens are reached via the real Router (`app.navigate(name)`); the
  detail editor is instantiated with a real party mon as `boxData` (`app.instantiate(url, props)`); the
  View All drawers are opened by **clicking the real footer "View All" button** (found by its `text` —
  robust across Bag + Pokémon; guessing a `shown` property hit the wrong item on Bag); the name editor's
  popup field is found by its **`placeholderText` "Enter a name"** (NOT the first TextField — that's the
  tileset combo) and its `text` is driven so the field owns the value and the preview follows; the
  name editor itself is found by its `editorVisible` property; hovers are `QTest::mouseMove` to centre.
- **Detached builds.** Per the usual kit gotchas, run the build/run detached + polled (the PowerShell
  transport caps ~60s) and set the crash-fast error mode (the `.ps1` does both).

## Dependencies

- Capture (automated): the normal Qt 6.11 kit (no extra deps) — `screenshooter` builds in the `build/`
  test dir and produces PNGs only. The scripts/CI need nothing beyond that.
- Manual GIFs: **Python + Pillow** (`pip install Pillow`), used only when you run `scripts/make_gifs.py`
  by hand to assemble frames you captured yourself. Not part of the automated pipeline. Pillow is
  credited in `credits.json` → Tools Used.
