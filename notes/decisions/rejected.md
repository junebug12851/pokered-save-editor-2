# Rejected Approaches

Things that were tried, failed, and should NOT be tried again.

The first batch below comes from the 2019–2020 development history (reconstructed from the
commit log; see `version.md` and `context/origins.md`). The rest are from the 2026 revival.

---

## ❌ "Windows CI can't have Qt ≥ 6.11" — a WRONG conclusion, retracted the same hour (2026-07-17)

**Concluded:** that Qt 6.11 was unreachable for the Windows CI/release jobs, and the choice was
therefore between 6.10.3 everywhere, staying on 6.8.3, or putting project leadership's Qt account password into
GitHub secrets. It was written up in `qt-patterns.md` as a *ceiling* and put to leadership as a
settled constraint.

**It was wrong.** The reasoning was sound as far as it went — the diagnosis was exact (Qt split the
Windows repo per-arch at 6.11; aqt fetches `qt6_6110/qt6_6110/Updates.xml` and 404s; 6.9/6.10 work,
6.11.0/6.11.1/6.12.0 all fail identically) — and then it took **one bad step**: PyPI says 3.3.0 is the
newest aqtinstall, therefore no fix exists, therefore it's impossible.

**Project leadership refused it:** *"i find it hard to believe the community has no solution for this moving
forward."* Ninety seconds of actually looking: issue
[#959](https://github.com/miurahr/aqtinstall/issues/959) describes the exact folder split, and PR
[#1000](https://github.com/miurahr/aqtinstall/pull/1000) *"Support Qt 6.11+ for Windows x64"* was
**merged 2026-03-24 — four months earlier**. `install-qt-action` has an `aqtsource` input for exactly
this case. Verified working in a throwaway venv, then shipped as a pinned `aqtsource`.

**The standing lessons — three, and they generalise well past Qt:**

1. **"The latest RELEASE can't" is not "it can't."** 3.3.0 is from **June 2025**; the project had
   moved on a year without cutting a tag. A release number is a publishing decision, not a statement
   about what the code does. **Check the repo — issues, PRs, master — before declaring a wall.**
2. **A big ecosystem having no answer is a SMELL, not a finding.** Every Qt project on GitHub Actions
   installs Qt this way. If the conclusion is "they're all stuck", the conclusion is wrong. That
   incredulity is a load-bearing instinct and it should fire *before* the write-up, not after.
3. **Don't promote a constraint to a decision for someone else.** The ceiling was handed up as fact
   with options built on top of it — which would have converted one unchecked assumption into a
   permanent, wrong architecture choice (6.10.3, or credentials in CI). **Findings that shape
   leadership's options need the same standard of proof as findings that shape the save file.**

*(Sibling of the same day's other lesson — this one's cousin is "a file existing is not a capability
being available", which is why the container was lying about three tests. Both are: check the thing
itself, not a proxy for it.)*

---

## The conflicting-flags system (briefed 2026-07-15 → shelved 2026-07-16)

**Tried:** a system to flag *combinations* of event flags as conflicting — a logical predicate over a flag
set (both-on / both-off / not-both-on / at-most-one / …), each **suspected** or **confirmed**, with a
severity, surfaced in the UI in two places (a panel at the top listing active conflicts + a badge on each
conflicting flag), re-evaluated live on every toggle. Briefed as a QoL perk: let a non-advanced user do
something advanced quickly and safely.

**Shelved 2026-07-16, leadership** (*"too expensive to figure out… it honestly may be too expensive"*);
call delegated and **agreed**. Why it doesn't pay:

- **The founding case was a false positive.** Route 22's two rivals on one tile — the most obvious
  "conflict" in the game — is **REFUTED**: `Route22DefaultScript` is an ordered if/else, so with the 1st
  flag set the 2nd is **never read** (masked, not conflicting); the console engages a normal battle. If the
  flagship is wrong, the static heuristic's hit-rate is unknown and likely poor.
- **Only `confirmed` may warn** (the rule that refutation forced — suspicion must never render as a risk).
  So an unadjudicated system shows the user **nothing**: all cost, no visible value.
- **Adjudication is bespoke and doesn't amortise.** Arming *one* cutscene needed the exact script step
  (`wRoute22CurScript`), missable visibility, and the true trigger coords (`dbmapcoord 29,4`, y,x) — fresh
  research per case, across a 2,560-flag space.

**Do not rebuild it on static co-location.** "Two flags for one subject / two objects on one tile" is a
**lead, not evidence** — dispatch order can mask a flag entirely. Any revival must adjudicate on the
console *first* and show only confirmed results.

**Kept as knowledge (not shipped):** `scripts/analyze_flag_contradictions.py` + `tmp/event-flags/conflicts.json`
(research output), the refutation + its lesson in [`reference/event-flags.md`](../reference/event-flags.md),
`scripts/emu/probe_route22_conflict.py` (now a no-crash regression), and the one real lead the probe found —
`route22-rival-armed-but-hidden` (flags arm the ambush while the rival object is hidden → trigger fires,
script advances, no battle engages). **Nothing about conflicts renders in the UI.**

⚠️ The event-flag **research, sorting and grouping are NOT rejected** — leadership was explicit they
"remain very important for everything else". Only the conflict *feature* is shelved.

---

## Universal object stacking on the map (2026-07-14 → removed 2026-07-15)

**Tried:** a `MapObjectStack` "group box" that gathered every map object sharing a tile (player + NPCs +
warps + signs) into one tabbed widget — pick a member on the left, move the group from the centre, delete
the group on the right — and made each lone chip hide itself (`isStacked`) when it became part of a stack.
Added at project leadership's request 2026-07-14.

**Removed 2026-07-15, project leadership:** *"it never worked well and there's no point in fixing it because I only
added it from a misunderstanding."* Overlapping objects now simply draw over each other, each an
independent selectable/draggable chip (the ordinary behaviour, and the one that works). Do not
reintroduce the group box. Deleted `MapObjectStack.qml` + the `stacks`/`stackList`/`isStacked`/dispatch
machinery in `MapCanvas` and the `!isStacked` visibility gates in the chips.

---

## Historical dead ends (2019–2020)

These were each tried for real and abandoned. Hashes point at the commit in `version.md`.

### The whole JavaScript implementation
**Tried** (`7c59d67`, Oct 2019): rewrite the app in QML/QtQuick + JavaScript instead of C++,
to make porting easier and the databases cleaner. A full JS save model was built (iterator,
expanded data, fragments, sections, a PokemonDB, a text-search class).
**Abandoned** (`df68676` / `03c5739`, Dec 2019): deleted entirely and rebuilt in C++, because
**Qt's JS engine had degraded since being retargeted at QML/Quick** — it wasn't a good enough
foundation for the back-end. **Lesson:** the data/back-end logic belongs in C++; QML is for the
view only. Don't move model logic back into JS.

### QML `Loader` + the object-chain navigation
**Tried**: the first UI drove navigation with QML `Loader` plus `Pages.js`/`Style.js` and an
object chain. **Abandoned** (`aba290a` — "Loader is a disaster and Object chain is a disaster";
torn out in `cb36cbc`, ~2,000 lines deleted). **Replaced by** the C++ `Router` driven through
the `Bridge` (`d0b4f41`) — the navigation system still in use. Don't reintroduce a
Loader/Pages.js scheme.

### QML drag-and-drop
**Tried** (`53d69ea`, Feb 2020): drag-and-drop reordering in the item/bag lists. After 4–5
hours it produced *nothing* working — Qt's QML drag-and-drop was judged disproportionately hard
(little working documentation, much of it C++-only). **Abandoned**; reordering is done with
explicit move up/down/top/bottom buttons instead (see `ItemBoxView`/`ItemsPane`). Don't sink
time into QML drag-and-drop unless the Qt story has genuinely improved.

### A separate "Core" library/DLL
**Tried** (`e682f2e`): scaffold a standalone `core` library to split out shared bits as its own
DLL. **Abandoned a few commits later** (`01f51d1`): a plugin-style core split wasn't needed and
only added time on top of the refactor already underway. The shared helpers live in `common`
instead. Don't re-split a `core` out of `common`.

### Static libraries for the sub-projects
**Tried**: building common/db/savefile as **static** libraries. **Abandoned** (`5cbd7ff`):
something linked them out-of-order despite instructions not to, breaking the build. Switched to
**shared** libraries (which is why each carries a `*_autoport.h` export header). Don't flip them
back to static.

### `QVariant` as a `QHash` value
**Tried** (during the Era-1 C++ models): storing model data in `QHash<…, QVariant>`.
**Failed** (`476ba72`): `QVariant` can't be used as a `QHash` value type. Moved toward
`QString`-keyed/typed storage. (Filed here as a Qt limitation to remember — see
`reference/qt-patterns.md`.)

### `Q_PROPERTY` on the plain data-model classes
**Tried**: putting `Q_PROPERTY` directly on the early C++ data models. **Failed**
(`bceb15e`/`99188ed`): it couldn't work the way it was wanted — QML/C++ interop forces
Qt-ecosystem object shapes, and the ecosystem rejects modern-C++ object designs that compile
fine outside it. **Resolved** by reshaping the models into plain structs registered with the Qt
Meta Object System (`9fb2775`, `42da8d7`). This is the deep reason the data types are shaped the
way they are.

### Repeating random-name selection
**Tried**: picking random names by pure random index. **Abandoned** (`12eb978` — "it just kept
re-picking the same names"): switched to stepping through an incremental list, then to a
no-repeat pool that auto-reloads when depleted (`7eff6bb`). Don't go back to naive random index
selection for the name generators.

---

## Wrong theory: dataExpandedChanged signal parameter (session 4 theory, session 10 confirmed)

**What was tried**: Removing the parameter from `SaveFile::dataExpandedChanged` signal.

**Why it seemed plausible**: QML was showing `Cannot read property 'player' of undefined`
whenever `dataExpanded` changed. Session 4 theorized that having a parameter in the signal
was confusing Qt's property binding system.

**Why it was wrong**: The signal parameter was never the issue.
`SaveFile::dataExpandedChanged(SaveFileExpanded*)` is correct. **Session 13 strengthened this:**
`SaveFileExpanded`'s own NOTIFY signals (`playerChanged()` etc.) are *parameterless* and the
chain still read `undefined` — so a signal parameter cannot be the cause.

**Do NOT** remove the parameter from this signal or change its signature.

### Correction (session 13): the "rebuild + qRegisterMetaType" explanation was ALSO incomplete

Sessions 10–12 concluded the `dataExpanded = undefined` chain was caused by a truncated
`bootQmlLinkage.cpp` missing `qRegisterMetaType<T*>()` calls, and that a clean rebuild would
fix it. The rebuild happened (the binary was current — exe newer than all sources) and the
registrations were present, yet the **entire chain still read `undefined`**.

The actual root cause is `Q_DECLARE_OPAQUE_POINTER` on the QObject chain types (in
`savefile_autoport.h` and `area/area.h`): it forces
`IsPointerToTypeDerivedFromQObject<T*> = false`, so Qt treats those QObject pointers as opaque
values that QML cannot traverse. `qRegisterMetaType` / `qmlRegisterUncreatableType` do not
override it. Fix = remove the opaque decls for the QObject types and `#include` their full
headers down the chain. See `reference/qt-patterns.md` and `reference/fix-patterns.md`.

(Truncation repair and the registrations were still worthwhile — they just weren't the cause.)

---

## Using qt_add_qml_module() alongside app.qrc

**What was tried**: Adding `qt_add_qml_module()` to `projects/app/CMakeLists.txt` as part
of modernization.

**Why it seemed right**: Qt 6 documentation recommends `qt_add_qml_module()` for modern Qt
Quick applications.

**Why it failed**: The app already registers QML files via `app.qrc` at `qrc:/ui/app/...`.
Adding `qt_add_qml_module()` registers the same files again at `qrc:/qt/qml/App/...`. The
generated module initialization code conflicts with the existing paths and hangs
`QQuickWidget::setSource()` indefinitely.

**Decision**: Keep QML in `app.qrc` + use `qmlRegisterType()` / `qmlRegisterUncreatableType()`
in `bootQmlLinkage.cpp`. This is sufficient and avoids the conflict.

---

## QSurfaceFormat MSAA

**What was tried**: Setting `format.setSamples(8)` on the default `QSurfaceFormat` in boot.

**Why it seemed right**: Standard anti-aliasing setup for Qt apps.

**Why it failed**: `QQuickWidget` renders to an offscreen FBO, not a regular window surface.
Windows GPU drivers hang 40+ seconds trying to negotiate MSAA for FBO rendering (unsupported).

**Decision**: Remove all `QSurfaceFormat` configuration. Qt Quick handles its own antialiasing.

---

## Calling load() from DB singleton constructors

**What was tried**: The original design (kept from Qt 5) where each DB constructor called `load()`.

**Why it worked in Qt 5**: C++03 static local init had no mutex — re-entry was UB but harmless.

**Why it deadlocks in Qt 6**: C++11 static local init is mutex-guarded. Same-thread re-entry
during initialization deadlocks.

**Decision**: `DB::loadAll()` is the sole caller of each DB's `load()`, invoked after all
singleton objects exist. Do NOT put `load()` back into constructors.

---

## Wrong fix: Pokédex dexInd off-by-one (session 10, reverted)

**What was tried**: Changed `(dexInd+1)` to `dexInd` in `getMonUrl()` and `fixNum()` in
`Pokedex.qml`. Also changed `toggleOne(dexInd)` to `toggleOne(dexInd-1)` and modified
`playerpokedex.cpp` to add bounds checking and emit `dexItemChanged(val+1)`.

**Why it seemed plausible**: Assumed `dexInd` was 1-indexed (Pokédex numbers start at 1).

**Why it was wrong**: `IndRole` in `PokedexModel` returns `*mon->pokedex` which is **0-indexed**
(Bulbasaur = 0, Ivysaur = 1, ...). The original `(dexInd+1)` correctly converts to 1-indexed
filenames ("001-bulbasaur.svg"). Changing to `dexInd` produced "001-ivysaur.svg" for Ivysaur.

**Evidence of wrongness**: After the "fix", build output showed:
```
Cannot open: qrc:/assets/icons/mon-icons/001-ivysaur.svg
Cannot open: qrc:/assets/icons/mon-icons/002-venusaur.svg
```
All reverted. Original code is correct.

**Do NOT** change dexInd arithmetic. `(dexInd+1)` for filenames/display, `toggleOne(dexInd)` 
for direct array access — all correct as originally written.

---

## Bulk `sed -i` over the Cowork mount (2026-06-06 - catastrophic data loss)

**What was tried**: a project-wide rename via `sed -i` (and shell `>` redirection / heredocs) run
over the Cowork **mounted** filesystem (`/sessions/.../mnt/...`).

**What went wrong**: the mount does **partial/truncated writes under load** - a single `sed -i` sweep
across ~276 files silently truncated **55 source files + 8 notes** on the real disk (the Windows
compiler confirmed the truncation; it was NOT a stale-read artifact). git HEAD had been committed after
the damage, so it was useless for restore. Recovery took the entire session, via chat transcripts.

**Rule**: NEVER bulk-edit project files with `sed -i`, `perl -i`, or shell redirection over the
mount. Edit with the Read/Edit/Write tools or PowerShell (`[System.IO.File]::WriteAllText`,
UTF8-no-BOM), and **verify every write** (re-read + brace balance + no NUL bytes). The bash mount is
also unreliable for *reads* (false truncation) - verify with the Read tool / PowerShell, not
`cat`/`wc`. Recovery how-to: `reference/diagnostic-methods.md` -> "Recovering files from Cowork
chat transcripts".
