# UI Screenshots & Animation Capture

How the project's UI screenshots and animated GIFs are generated — automatically, headlessly,
and without ever touching save data. Built 2026-06-15.

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
- **Hover states** — the keyboard's font-tile preview tooltip and a Pokédex tile →
  `editor/text_keyboard_hover_tile.png`, `screens/pokedex_hover_tile.png`.
- **Animation frame sequences** under `frames/<name>/frame_NNN.png`, assembled into GIFs:
  - `tileset_anim` — the full keyboard's animated tileset tiles (water/flower) cycling. The capture
    sets the preview tileset to **Overworld + outdoor** (only outdoor tilesets actually move tiles),
    stops `TilesetDisplay`'s Timer, and drives its `curFrame` 0–7 so each frame is distinct.
  - `typing` — the quick-edit name editor building a name letter-by-letter: the capture sets the field
    text **and** `NameDisplay.str` per step so the live GB-font preview + byte counter update (a plain
    `name` preview does NOT animate on its own — only tileset tiles do; that's why this drives `str`).
  - `tab_cycle` — cycling the editor's General/DV-EV/Moves tabs.

  Frame playback speeds live in `make_gifs.py` `DURATION_MS` (per sequence).

GIFs are written next to the PNGs: `tmp/screenshots/<name>.gif`.

## How it works

Two pieces, both repo-friendly and CI-friendly:

1. **`projects/tests/tools/screenshooter.cpp`** — a small C++ tool (NOT a CTest test) built on the
   shared GUI harness `tests/helpers/guiapp.h`. It boots the **real** application UI headless on the
   `offscreen` platform — the exact engine/provider wiring the app uses (`brg`, the tileset + font
   image providers, `DB::qmlProtect`, the exe's `bootQmlLinkage`) — then navigates / instantiates /
   drives the live UI and grabs each frame with `QQuickWindow::grabWindow()`. CMake target
   `screenshooter` (in `projects/tests/CMakeLists.txt`, modeled on `pse_add_gui_test` +
   `gen_synthetic_fixtures`).

2. **`scripts/make_gifs.py`** — a Pillow script that assembles each `frames/<name>/` folder into a
   GIF. Pure Python + one common dependency; prints a clear message (and exits non-zero) if Pillow
   isn't installed, without affecting the PNG captures.

Driven by **`scripts/capture_screenshots.ps1`** (Windows dev kit) / **`scripts/capture_screenshots.sh`**
(Linux/Docker/CI): build the `screenshooter` target, run it, then run `make_gifs.py`.

## Byte-fidelity (sacred)

The tool **only ever reads** the save in memory to render the UI. It never calls
`saveFile()`/`flattenData()` and never writes a save byte. (See
`../context/principles.md` → "Save File Integrity Is Sacred".)

## Key technical notes (don't relearn these the hard way)

- **Render through a REAL GPU window — NOT offscreen+software.** This is the big lesson. The app draws
  its QML through a GPU-backed `QQuickWidget`; the `offscreen` platform with `QT_QUICK_BACKEND=software`
  **silently drops every `MultiEffect`/layered item** — so the Credits cards vanished, Home's disabled
  (greyed) tiles vanished, and shadows/whole screens rendered washed-out. The fix: the `.ps1` does NOT
  set `QT_QPA_PLATFORM`, so the tool runs on the native platform and `main()` shows the `QQuickView` as a
  **frameless, `Qt::Tool`, off-screen-positioned** window (`setPosition(-4000,-4000)`) — it renders on
  the GPU exactly like the app but never flashes on the user's desktop. `grabWindow()` renders the scene
  graph to an image regardless of the window being off-screen/occluded. (Offscreen+software is still the
  CI fallback — it runs, with the known missing-effects caveat. xvfb on Linux would render effects.)
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

- Capture: the normal Qt 6.11 kit (no extra deps) — `screenshooter` builds in the `build/` test dir.
- GIFs: **Python + Pillow** (`pip install Pillow`). Optional locally — if Python isn't present the
  script step is skipped and the PNGs are still produced (the `.ps1` warns and continues). Pillow is
  credited in `credits.json` → Tools Used.
