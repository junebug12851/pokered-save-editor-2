# Pokered Save Editor 2

![UI/UX progress](https://i.imgur.com/ULhYfEN.png)

**A polished desktop save editor for Pokémon Red & Blue (Game Boy), built in Qt 6 with C++ & QML.**

Open a Gen 1 `.sav`, edit your trainer, team, Pokédex, items, boxes and world — then save it back
*byte-for-byte intact*. The goal is software that feels like it belongs in your applications folder:
native, fluid, and fun to poke around in — not a wall of hex.

> _The reboot of [`pokered-save-editor`](https://github.com/junebug12851/pokered-save-editor) — same
> heart, rebuilt from scratch on Qt/C++ to escape the weight of the old JavaScript/Electron stack._
>
> Six years dormant, revived in 2026, and still going. ♡〜٩( ˃́▿˂̀ )۶〜♡

---

## ⚠️ Status — early alpha, work in progress

This is an **active WIP and not finished** — expect rough edges and bugs. It now edits real saves and has a large
automated test suite, but it is not yet a 1.0. Current version lives in [`VERSION`](VERSION)
(presently in the `0.x-alpha` range).

- 👉 For a **mature, complete** editor today, use the original:
  **<https://github.com/junebug12851/pokered-save-editor>**. This reboot will supersede it once it
  leaves WIP.
- This editor targets **original Red/Blue `.sav` files only**. It is **not** built for Pokémon Bank,
  Pokémon Home, or the Virtual Console releases, and makes no effort to support that pipeline.

---

## Table of contents

- [What it is](#what-it-is)
- [Features](#features)
- [Building from source](#building-from-source)
- [Running the tests](#running-the-tests)
- [Contributing](#contributing)
- [Why the reboot](#why-the-reboot)
- [Architecture](#architecture)
- [Project layout](#project-layout)
- [Screenshots](#screenshots)
- [Credits](#credits)
- [License & disclaimer](#license--disclaimer)

---

![Pokémon details screen](https://i.imgur.com/jnOZoyG.png)

## What it is

Pokered Save Editor 2 reads a 32 KB Pokémon Red/Blue battery save, **expands** it into a full tree of
editable C++ objects, lets you change things through a native desktop UI, and **flattens** your edits
back to disk.

The aim is to let you edit **every bit and byte of the save** through a clean, simple, dependable GUI,
at whatever depth you want — from quickly re-rolling a name, to editing your team and items, all the
way to advanced things like importing/exporting the the normally-unused bytes as raw `.bin`, and
working with map state. The quick stuff stays quick; the powerful stuff is
there when you want it. (Plenty of that in development — see [Features](#features).)

A few convictions shape everything:

- **Bit- and byte-exact fidelity, always.** An edit changes *only* the exact bits and bytes it needs
  to, and leaves everything else untouched — no rewriting, re-packing, or "normalizing" the file, and
  checksums are recomputed only where they genuinely must be. Even *unused* bits are treated as
  precious: it would be unacceptable for a single unintended bit to flip. Protecting your save data is
  the project's highest priority, and wherever something could go wrong, graceful degradation is always
  preferred over any risk of corruption.
- **Careful, graceful failure** The goal is to take real care with errors: even an otherwise-fatal problem should
  be caught and presented through a clean, clear UI, degrading smoothly and — where it's possible —
  offering a way to keep going (for example, reloading what failed) instead of crashing or stranding
  you. Debug builds surface problems to the developer in detail; release builds handle them gracefully
  and clearly, without ever putting your data at risk.
- **A real native desktop app.** A native `MainWindow` with native menus hosts the Qt Quick UI —
  this is not a web app.

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
- **Assisted code / state injection** into the save.

**Randomization** is a flagship part of the app, and a *growing* one. Today it covers the basics — a
quick one-click name re-roll and an early new-game randomizer. The planned system is built around
three modes:

- **Constrained Random** — randomized, but kept within sensible, playable bounds.
- **Unconstrained Random** — anything goes; chaotic by design; it may not even load.
- **Synthetic Natural** — a generated save crafted to look like one genuinely earned by playing the
  real game.

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

![Pokémart screen](https://i.imgur.com/2MMsNqk.png)

## Contributing

This is an open-source project and contributions are welcome. The workflow and standards are written
down so anyone (human or AI assistant) can pick it up cleanly. **Please read the living notes in
[`notes/`](notes/) before making changes** — start with [`notes/status.md`](notes/status.md) for
current state, and [`notes/context/`](notes/context/) for the project's goals and principles.

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

Getting here was long — the codebase was **rewritten from scratch three times** (a pure-C++ start,
a JavaScript/QML detour, and the final C++ design) while wrestling a framework that often fights
modern C++. The hard-won takeaway: keep the foundation lightweight and solid, then build the powerful
stuff on top of it. The fuller story lives in [`notes/context/origins.md`](notes/context/origins.md).

## Architecture

A four-library split (build order), with the app shell on top:

```
common  →  db  →  savefile  →  appcore  →  app
```

| Layer | Role |
|-------|------|
| `common` | Shared types and a `Random` wrapper. |
| `db` | All Gen 1 game data, loaded from JSON, cross-linked, exposed via `XxxDB::inst()` singletons. |
| `savefile` | Parse → **expand** into editable C++ objects → **flatten** back with bit- and byte-exact fidelity. |
| `appcore` | The testable app logic: the `Bridge`, ~25 MVC list models, the font/tileset engine, the `Router`, settings. |
| `app` | Thin Qt/QML shell: `main`, boot, native `MainWindow`, `app.qrc`. QML talks to C++ through one `Bridge` object exposed as `brg`. |

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

The **in-app Credits screen** (data in
[`projects/db/assets/data/credits.json`](projects/db/assets/data/credits.json)) lists everyone and
everything that helped: contributors, data sources, frameworks, tools, services, and the artists
behind the icons and artwork. Most artwork and icons are by fans and are credited there accordingly.
The 2026 revival has been done with the help of development tooling.

## License & disclaimer

Copyright © Twilight. The project's **source code** is licensed under the **Apache License,
Version 2.0** — see [`LICENSE`](LICENSE) for the full text, or
<http://www.apache.org/licenses/LICENSE-2.0>.

Pokémon, Pokémon Bank, Pokémon Home, the Virtual Consoles, and related names, sprites, and artwork are
owned by GameFreak / Nintendo / The Pokémon Company. This project claims no ownership of them. Bundled
third-party assets and dependencies retain their own licenses (e.g. Qt under LGPL v3, icon sets under
their respective terms); see the Credits screen and asset license files.
