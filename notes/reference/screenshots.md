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
(Linux/Docker/CI): build the `screenshooter` target, run it offscreen, then run `make_gifs.py`.

## Byte-fidelity (sacred)

The tool **only ever reads** the save in memory to render the UI. It never calls
`saveFile()`/`flattenData()` and never writes a save byte. (See
`../context/principles.md` → "Save File Integrity Is Sacred".)

## Key technical notes (don't relearn these the hard way)

- **Software scene-graph backend.** The tool forces `QT_QUICK_BACKEND=software` under offscreen so
  `grabWindow()` returns real pixels with no GPU (works in CI/Docker). Trade-off: the software backend
  skips `MultiEffect` (drop shadows, the disabled-tile desaturation) — a minor cosmetic loss. Unset
  `QT_QUICK_BACKEND` to try the GPU path for effect-accurate shots.
- **Fonts on offscreen.** The offscreen platform uses Qt's FreeType font DB, which finds **no fonts**
  unless pointed at a font dir — without this, all UI text renders as tofu boxes (the in-game *bitmap*
  name preview still renders, since it comes from the font image provider, which masked the problem at
  first). The tool sets `QT_QPA_FONTDIR` to the OS font dir (Windows `%WINDIR%\Fonts`, macOS
  `/System/Library/Fonts`) when it's empty; Linux/CI is served by fontconfig. The harness already
  allowlists the benign "Cannot find font directory" warning.
- **Driving the UI.** Top-level screens are reached via the real Router (`app.navigate(name)`); the
  detail editor is instantiated with a real party mon as `boxData` (`app.instantiate(url, props)`);
  the View All panels are opened by setting their custom `shown` property; the name editor is found by
  its distinctive `editorVisible` property (not a fragile metatype-name match); hovers are synthesized
  with `QTest::mouseMove` to the item centre.
- **Detached builds.** Per the usual kit gotchas, run the build/run detached + polled (the PowerShell
  transport caps ~60s) and set the crash-fast error mode (the `.ps1` does both).

## Dependencies

- Capture: the normal Qt 6.11 kit (no extra deps) — `screenshooter` builds in the `build/` test dir.
- GIFs: **Python + Pillow** (`pip install Pillow`). Optional locally — if Python isn't present the
  script step is skipped and the PNGs are still produced (the `.ps1` warns and continues). Pillow is
  credited in `credits.json` → Tools Used.
