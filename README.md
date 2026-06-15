# Pokered Save Editor 2

**A Pokémon Red & Blue save editor written in C++/Qt/QML.**

A desktop app for comprehensively editing every aspect of a Generation 1 save file through a clean,
simple, intuitive interface — heavily tested for stability and built to protect every byte in your
save, even the unused ones, from unintended changes during editing, to the best of what testing can
ensure.

> _The reboot of [`pokered-save-editor`](https://github.com/junebug12851/pokered-save-editor) —
> rebuilt from scratch in Qt/C++._

---

![Pokered Save Editor — Home screen](https://junebug12851.github.io/pokered-save-editor-2/screenshots/home.png)

## ⚠️ Status — early alpha, work in progress

This is an **active WIP and not finished** — expect rough edges and bugs. It carries a large
automated test suite that does its best to ensure stability and byte fidelity, protecting your save
file as completely as it can throughout editing — but it is not considered released or finished yet.
Current version lives in [`VERSION`](VERSION) (presently in the `0.x-alpha` range).

- 👉 For a **mature, complete** editor today, use the original:
  **<https://github.com/junebug12851/pokered-save-editor>**. This reboot will supersede it once it
  leaves WIP.
- This editor targets **original Red/Blue `.sav` files only**. It is **not** built for Pokémon Bank,
  Pokémon Home, or the Virtual Console releases, and makes no effort to support that pipeline.

---

## Table of contents

- <a href="#what-it-is">What it is</a>
- <a href="#screenshots">Screenshots</a>
- <a href="#features">Features</a>
- <a href="#building-from-source">Building from source</a>
- <a href="#running-the-tests">Running the tests</a>
- <a href="#contributing">Contributing</a>
- <a href="#why-the-reboot">Why the reboot</a>
- <a href="#architecture">Architecture</a>
- <a href="#project-layout">Project layout</a>
- <a href="#credits">Credits</a>
- <a href="#license--disclaimer">License & disclaimer</a>

---

## Screenshots

> Live UI images, **auto-updated on every commit** (rendered headlessly in CI and hosted on GitHub
> Pages — so they always reflect the current `main`). Full gallery + docs linked below.

| | |
|---|---|
| ![Home](https://junebug12851.github.io/pokered-save-editor-2/screenshots/home.png) | ![Trainer Card](https://junebug12851.github.io/pokered-save-editor-2/screenshots/trainerCard.png) |
| ![Pokémon storage](https://junebug12851.github.io/pokered-save-editor-2/screenshots/pokemon.png) | ![Pokémon editor](https://junebug12851.github.io/pokered-save-editor-2/screenshots/pokemon_editor_general.png) |
| ![Bag](https://junebug12851.github.io/pokered-save-editor-2/screenshots/bag.png) | ![Pokédex](https://junebug12851.github.io/pokered-save-editor-2/screenshots/pokedex.png) |

The in-game name editor, building a name live:

![Typing a name in the in-game font](https://junebug12851.github.io/pokered-save-editor-2/screenshots/typing.gif)

📸 **[Full screenshot gallery](https://junebug12851.github.io/pokered-save-editor-2/screenshots/)** &nbsp;·&nbsp; 📖 **[Documentation](https://junebug12851.github.io/pokered-save-editor-2/docs/)**

## What it is

Pokered Save Editor 2 reads a 32 KB Pokémon Red/Blue battery save, **expands** it into a full tree of
editable C++ objects, lets you change things through a native desktop UI, and **flattens** your edits
back to disk.

The aim is to let you edit **every bit and byte of the save** through a clean, simple, dependable GUI,
at whatever depth you want — from quickly re-rolling a name, to editing your team and items, all the
way to advanced things like importing/exporting the normally-unused bytes as raw `.bin`, and
working with map state. The quick stuff stays quick; the deeper stuff is there when you want it.
(Plenty of it is in development — see <a href="#features">Features</a>.)

A few convictions shape everything:

- **Bit- and byte-exact fidelity, always.** An edit changes *only* the exact bits and bytes it needs
  to, and leaves everything else untouched — no rewriting, re-packing, or "normalizing" the file, and
  checksums are recomputed only where they genuinely must be. Even *unused* bits are treated as
  immutable without intent: it would be unacceptable for a single unintended bit to flip. Protecting
  your save data is the project's highest priority, and wherever something could go wrong, graceful
  degradation is always preferred over any risk of corruption.
- **Beautiful UI/UX.** The interface must be clean, polished, creative, and artistic; the experience
  must be clean, structured, organized, intuitive, and comprehensive. Anything that would put the UI,
  the UX, or their stability at risk doesn't ship as-is — it waits for an implementation route that
  doesn't.
- **Graceful failure.** Even an otherwise-fatal error or crash should be caught and presented through
  a clean, clear UI, degrading smoothly and — where it's possible — offering a way to keep going (for
  example, reloading what failed) instead of crashing or stranding you. Data integrity, byte fidelity,
  and preventing data loss stay the highest priority through error handling. Debug builds surface
  problems to the developer in detail; release builds handle them gracefully and clearly.
- **No longer a web app.** A native `MainWindow` with native menus hosts the Qt Quick UI — this is a
  real desktop application, not a packaged web page.

![New File screen](https://i.imgur.com/Woakr9P.png)

## Features

**What works today** — open a save and edit:

- **Trainer Card** — name, money, badges, play time, starter, IDs.
- **Pokémon** — team & PC box editing: species, level, stats, moves, DVs/EVs, nickname, OT.
- **Pokédex** — seen/caught state for all 151.
- **Items** — bag, PC storage, and Pokémart.
- **Rival** — name and related data.
- **Quick name randomization** — one-click re-roll in the name editor and trainer screen.
- **In-game font keyboard** — a name editor that renders the real Game Boy font and character picker,
  not a table of hex codes.

**In development / planned** — the bigger picture:

- A deeper **randomization** system (described below).
- **Map / location** editing and map randomization.
- **Hall of Fame** and broader world-state editing.
- **Raw `.bin` import/export** of the whole save — including the normally-unused bytes.
- **Assisted custom-code execution** — help injecting custom code into a save so it runs in-game, and
  help scrubbing custom code back out of a save.

**Randomization** is a flagship part of the app, and a *growing* one. Today it covers the basics —
sensible randomizations on request, applicable to many parts of the save — and is functional but
still early. The planned system is built around three modes:

- **Constrained Random** — randomized, but kept within sensible, playable bounds.
- **Unconstrained Random** — anything goes; chaotic by design.
- **Synthetic Natural** — a generated save crafted to look like one genuinely *earned* by playing the
  real game. Imagine pressing a button for a random save and being handed an old, worn file with real
  history and depth — one you might get caught up in, want to finish, or just dig through. The goal is
  to make that its own button, sitting right next to the constrained and unconstrained options.

**Under the hood:**

- Bit- and byte-exact save round-tripping with a comprehensive automated test suite (savefile, model,
  GUI, and QML-screen smoke tests) run on Windows and in a Linux container (incl. ASan/UBSan and
  coverage).
- A C++ game database of all Gen 1 data (Pokémon, moves, items, maps, …) with cross-linked entries.

![Pokédex screen](https://i.imgur.com/Ks6KzaT.png)

## Building from source

**Prerequisites**

- **Qt 6.11** (the project is developed against the `llvm-mingw` 64-bit kit on Windows; a recent
  Clang-based Qt 6 kit on Linux/macOS should also work).
- **CMake ≥ 3.21** and **Ninja**.
- A **C++17** compiler (Clang from the llvm-mingw toolchain is the reference).

**Option A — Qt Creator (easiest):** open `projects/CMakeLists.txt` as a CMake project, select your
Qt 6.11 kit, and build/run the `PokeredSaveEditor` target.

**Option B — command line:**

```sh
# Configure (the CMake root is the projects/ folder, not the repo root)
cmake -S projects -B build -G Ninja

# Build the app
cmake --build build --target PokeredSaveEditor

# Run it (path varies by generator/kit; e.g.)
./build/app/PokeredSaveEditor
```

The version number is single-sourced from [`VERSION`](VERSION); CMake propagates it to the runtime,
the About screen, and the Windows executable resource — never hardcode a version.

**API documentation** (optional): the C++ is Doxygen-commented. Run `doxygen Doxyfile` from the repo
root to generate HTML docs (QML is intentionally excluded).

![File Tools screen](https://i.imgur.com/4oPvOE9.png)

## Running the tests

The test tree configures from the same CMake root:

```sh
cmake -S projects -B build -G Ninja
cmake --build build
cd build && ctest --output-on-failure
```

For Linux builds, sanitizers (ASan/UBSan — which don't run on the Windows kit), and coverage, use the
containerized harness:

```powershell
./docker/dtest.ps1 [standard|asan|xvfb|coverage|all]
```

A change is considered green only when the **full `ctest` suite passes**, including the QML screen
smoke test (`tst_qml_screens`), which loads every screen through the real engine.

## Contributing

This is an open-source project and contributions are welcome. The workflow and standards are written
down so anyone (human or AI assistant) can pick it up cleanly. **Please read the living notes in
[`notes/`](notes/README.md) before making changes** — start with [`notes/status.md`](notes/status.md) for
current state, and [`notes/context/`](notes/context/project.md) for the project's goals and principles.

A few things to keep in mind:

- **Save-data fidelity is a big deal here.** The editor only ever changes the exact bits and bytes an
  edit needs, leaving everything else untouched — please hold contributions to that same high
  standard. Background in the save-data integrity section of
  [`notes/context/principles.md`](notes/context/principles.md).
- **Quality bar is high:** prefer the correct, clean solution over a quick hack. Well-documented
  workarounds for genuine framework defects are fine; hacks of convenience are not.
- **Branches:** `main` is stable (builds + tests green) and only ever fast-forwards from `dev`; do
  day-to-day work on `dev`. Never commit directly to `main`.
- **Commits:** one focused change each, imperative `type: summary` messages (`fix:`, `feat:`,
  `refactor:`, `docs:`, `tests:`, …). Stage specific files; never `git add -A`. Don't rewrite pushed
  history.
- **Keep the notes and the in-app Credits living** — update them as part of the change, not after.

Full details: [`notes/reference/git-workflow.md`](notes/reference/git-workflow.md) and
[`notes/plans/testing.md`](notes/plans/testing.md).

## Why the reboot

The first version was JavaScript: Angular + Electron, back-end and front-end split by sandboxing, a
pile of libraries, sync/async juggling, and a build that took **~45 minutes** to produce a gigantic
binary. JavaScript itself is lovely — but on that stack a large *desktop* app grew unwieldy fast.
Angular is built for the web; Electron packaging is slow and error-prone; the complexity outgrew what
the project could sustain, and it was shelved.

Qt/C++ buys back simplicity without giving up the look and feel: a far smaller toolset built for
exactly this job, room for the features that were always wanted, and builds that finish in *minutes,
not 45 minutes*. The trade-off (large output, fiddlier deployment) is well worth it.

## Architecture

A four-library split (build order), with the app shell on top:

```
common  →  db  →  savefile  →  appcore  →  app
```

| Layer | Role |
|-------|------|
| [`common`](notes/systems/common.md) | Shared types and a `Random` wrapper. |
| [`db`](notes/systems/db.md) | All Gen 1 game data, loaded from JSON, cross-linked, exposed via `XxxDB::inst()` singletons. |
| [`savefile`](notes/systems/savefile.md) | Parse → **expand** into editable C++ objects → **flatten** back with bit- and byte-exact fidelity. |
| [`appcore`](notes/systems/app.md) | The testable app logic: the `Bridge`, ~25 MVC list models, the font/tileset engine, the `Router`, settings. |
| [`app`](notes/systems/app.md) | Thin Qt/QML shell: `main`, boot, native `MainWindow`, `app.qrc`. QML talks to C++ through one `Bridge` object exposed as `brg`. |

For the in-depth, code-grounded version, see [`notes/systems/overview.md`](notes/systems/overview.md)
and [`notes/context/architecture.md`](notes/context/architecture.md). The save format itself is
documented in [`assets/saves/structure.md`](assets/saves/structure.md) (with the byte-exact 010 Editor
template `structure.bt` beside it).

## Project layout

```
assets/        Non-code assets: save fixtures (assets/saves/), tilesets, wallpaper, staging
docker/        Containerized Linux build/test harness (standard, ASan, xvfb, coverage)
notes/         The living project knowledge base — read notes/status.md first
projects/      All source: common / db / savefile / appcore / app, plus tests
tmp/           Tracked-but-ignored scratch area for build/run artifacts
Doxyfile       C++ API doc generation
VERSION        Single source of truth for the version number
```

## Credits

The **in-app Credits screen** lists everyone and everything that helped: contributors, data sources,
frameworks, tools, services, and the artists behind the icons and artwork. See the full list in
[`credits.md`](projects/db/assets/data/credits.md) (the human-readable rendering of the underlying
[`credits.json`](projects/db/assets/data/credits.json) the app reads). Most artwork and icons are by fans and are credited there accordingly.
The 2026 revival has been done with the help of development tooling.

## License & disclaimer

Copyright © Twilight. The project's **source code** is licensed under the **Apache License,
Version 2.0** — see [`LICENSE`](LICENSE) for the full text, or
<http://www.apache.org/licenses/LICENSE-2.0>.

Pokémon, Pokémon Bank, Pokémon Home, the Virtual Consoles, and related names, sprites, and artwork are
owned by GameFreak / Nintendo / The Pokémon Company. This project claims no ownership of them. Bundled
third-party assets and dependencies retain their own licenses (e.g. Qt under LGPL v3, icon sets under
their respective terms); see the Credits screen and asset license files.
