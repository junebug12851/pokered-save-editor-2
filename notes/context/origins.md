# Origins — The First Life (2019–2020)

The story *before* the 2026 revival. `context/history.md` picks up at "the repo had 592
commits, last pushed March 2020" — this file is the prologue it assumes. It's reconstructed
from the full commit history (see the root `version-notes.md` changelog; commit hashes below tie
each claim back to a real diff).

The short version: the project was started in June 2019, rewritten from scratch **three
times** as Twilight fought the Qt framework, eventually settled into the architecture the
codebase still uses today, and was abandoned mid-refactor in March 2020 — not from lack of
care, but because the complexity outgrew what one solo developer could sustain. Everything
below is *why the code looks the way it does*.

---

## Era 1 — The first C++ attempt (June 2019)

The repository opens with `6aa6445` (Initial Commit: `.gitignore`, LICENSE, README) and
`1ff31c1` (first real code: file management, raw save-data handling, and a sample QML UX).

A formative decision came almost immediately: `49594d7` **moved off a pure Qt Quick Window
into a native `MainWindow` with a native menu bar**, explicitly to escape the "web page"-like
feel of a QML menu and get something that feels native on Windows. That conviction — *this is
desktop software, not a web app* — is still visible in the architecture (a `QQuickWidget`
hosted inside a real `MainWindow`).

The data layer was built as pure-C++ models (`item`, `move`, `pokemon`, `type`) backed by a
`store/pokemondatabase`. This era is mostly a running battle with Qt:

- `QVariant` can't be used as a `QHash` value (`476ba72`) — an unexpected, undocumented limit.
- `Q_PROPERTY` couldn't live on the plain data models the way it was wanted (`bceb15e`).
- Qt's ecosystem won't accept modern C++ that compiles fine *outside* it, and QML/C++ interop
  requires Qt-ecosystem objects (`99188ed`) — a "walled garden" the message openly resents.
- The models were converted from `enum class` + QObject-derived classes into plain structs
  registered with the Qt Meta Object System (`9fb2775`, `42da8d7`).
- C++17 was adopted for `std::optional` (`3593e07`).

There was a revert-of-a-revert chain here (`14d58f0` → `1cddf37` → `a6e94b7`) as the data
integration was tried, backed out, and reinstated. By the end of the era the models built and
ran (`00ecbd5`), but the friction had been constant.

---

## Era 2 — The JavaScript pivot (October 2019)

`7c59d67` is the **single largest commit in the project's history** and a hard reset: the
approach shifts from pure C++ to **QML/QtQuick with JavaScript**, on the theory that it would
make porting easier and the databases cleaner. The old C++ `model/`/`store/` classes are
removed; the data and assets are reorganized under `assets/`; lodash is pulled in.

Over October a full JavaScript implementation took shape: the save-file system and iterator
(`d59bc5d`), the expanded-data fragments — Pokemon box/party, sign/sprite/warp/map-connection
data (`b609700`, `42537c1`), the sections — daycare, Hall of Fame, rival, storage (`fa86fcc`),
a working PokemonDB (`8985f8f`), and a text class with search (`c747839`).

This is the ancestor of everything that came after: the *shape* of the save model (expanded
objects, fragments, sections) was worked out here in JS first.

---

## Era 3 — Back to C++, for good (December 2019 – January 2020)

`df68676` ("Starting another try lol") deletes the entire `src.js/` JavaScript codebase and
pivots back to C++. The reason is stated plainly in `03c5739`: **Qt's JS engine isn't as good
as it used to be since it became tailored to QML/Quick** — so the back-end had to be
"painstakingly recreated in C++." This is the era that produced the architecture still in use.

**The databases.** Each game-data set was rebuilt as a C++ class loaded from JSON — the long
run of "Completed X Data" commits (items, moves, pokemon, maps, events, trainers, types,
fonts, …). Then came an **in-memory index** over each set (the "Index X data" commits) and a
**deep-linking** system that cross-references entries — e.g. a Pokemon links to the moves it
learns and the items/TMs involved (`98660a1` proved the first link; the rest followed). The
massive `maps.json` was built out with connections, warps, signs, sprites and wild encounters
across many commits.

**The expanded-data object model.** Each section of a save file became an editable C++ object:
area (audio/general/map/npc/player/pokemon/sign/sprites/tileset/warps), world
(general/events/completed/hidden/missables/scripts/towns/trades), player
(basics/pokedex/pokemon/items), storage, daycare, Hall of Fame, rival, and the shared
fragments (Pokemon box/party, item/sprite/sign/warp/map-connection data). The famously painful
one was `PokemonBox` (`6c950b7` — "may I never have to do that again"). A `Random` helper
(`70f9207`) was introduced to wrap `QRandomGenerator` behind a cleaner interface.

**The font/tileset rendering.** Two C++ image providers were built to render text and tiles in
the real Game Boy font for QML: the `TilesetEngine`/`TilesetProvider` (`22baf52`) and the
`FontPreviewProvider` (`49e47c3`). Getting these right meant fighting Qt Quick's image
scaling/blur (`0fb0106` — the provider had to report the correct "whole" size so Quick wouldn't
rescale) and, notably, **Qt Quick's broken URL encoding**: it URL-encodes the preview string
but can't decode its own encoding, so the workaround (`8fe8447`) is to hex-encode in QML, pass
that to C++, and decode from hex there — the origin of the `encodeBeforeUrl` helper still used
today.

**The UI and the Loader→Router pivot.** The first UI was a NavBar/Footer scaffold using QML
`Loader` plus `Pages.js`/`Style.js`. That approach was judged a disaster (`aba290a` — "Loader
is a disaster and Object chain is a disaster"); `cb36cbc` tears out ~2,000 lines of it and
`d0b4f41` introduces the **C++ `Router`** that drives navigation through the `Bridge` — the
pattern the app still uses. On top of that, each screen was built: Trainer Card, Pokedex,
Bag/Items, Pokemon storage + details, Pokemart, Maps, Rival — plus the elaborate **3-stage
name editor** and full on-screen keyboard, which Twilight put enormous care into (the in-game
font preview, the searchable character picker, the simulated tilesets).

---

## Era 4 — The great library refactor (March 2020)

The final push before abandonment. `7d5199b` ("Begun large-scale refactor") breaks the
monolithic app apart into separate libraries — **common / db / savefile / app** — the layout
the project still has. Along the way:

- The libraries were made **shared** rather than static after a stubborn link-order bug
  (`5cbd7ff`), which is why each library carries an export/autoport header.
- A separate **Core DLL** was scaffolded (`e682f2e`) then abandoned as unnecessary (`01f51d1`).
- The savefile code moved into its own `pse-savefile` library (`3a15b03` — the commit with the
  long, raw Qt-frustration rant).
- Every flat database was converted into a **`XxxDB` + per-row `XxxDBEntry`** layout under
  `entries/`, registered in a central `DB` aggregate. Credits was the pilot (`198effb`); Maps,
  Items, Events, Game Corner, Fonts, Hidden Items/Coins, Missables and the rest followed.

The last commit is `af883fd` (March 18, 2020). Then six years of silence.

---

## What carried over to 2026

The architecture the revival inherited — and the notes describe — was all set here:

- The **four-library split** (`systems/overview.md`, `decisions/architecture.md`).
- The **DB-plus-entry + deep-linking** database design (`systems/db.md`).
- The **expanded-data object model** with byte-exact flattening (`systems/savefile.md`).
- The **Bridge + Router** QML↔C++ wiring (`systems/app.md`).
- The **font/tileset image providers** and the `encodeBeforeUrl` hex trick (`systems/app.md`,
  `reference/qt-gotchas.md`).

And the *temperament* carried over too: a builder who will rewrite the whole thing three times
to get it right, who insists on native-feeling, non-janky software, and who fights a difficult
framework rather than ship something sloppy. See `context/principles.md`.

For the dead ends this era produced (so they aren't re-tried), see
`decisions/rejected.md` → "Historical dead ends (2019–2020)". For the catalog of Qt landmines
hit along the way, see `reference/qt-gotchas.md`.
