# Qt / QML Gotchas — Project-Lifetime Catalog

A single index of the Qt and QML landmines this project has hit across its whole life (2019 →
2026). It exists because the same kinds of surprises recur, and because a lot of the project's
hardest days went into these. Many cost hours or whole days; a few caused rewrites.

This is the **map**. The detailed fixes live elsewhere and are linked per row:
- `reference/qt6-patterns.md` — the Qt 5 → Qt 6 migration gotchas, with code.
- `reference/fix-patterns.md` — compiler/runtime error → fix lookup.
- `decisions/rejected.md` — approaches abandoned because of a gotcha.
- `decisions/architecture.md` — workarounds adopted as deliberate design.
- `version-info.md` — the commit (hash) where each historical one was hit.

---

## Historical (2019–2020)

| Gotcha | What happens | Resolution | Where |
|--------|-------------|------------|-------|
| **`QVariant` can't be a `QHash` value** | Won't compile / won't store as expected | Use `QString`-typed storage instead | `476ba72` |
| **`Q_PROPERTY` won't sit on plain data models** | QML/C++ interop rejects non-Qt-ecosystem object shapes; modern C++ that compiles outside Qt is unusable inside it | Reshape models into plain structs registered with the Qt Meta Object System | `bceb15e`, `99188ed`, `9fb2775`, `42da8d7` |
| **Qt Quick URL-encodes but can't decode its own encoding** | Image-provider source strings arrive corrupted | Hex-encode in QML, pass through the URL, decode from hex in C++ (`encodeBeforeUrl`) | `8fe8447` · `decisions/architecture.md` |
| **Image provider must report the "whole" size or Quick rescales it** | Tiles/text render blurry | Provider returns the exact requested size so Quick doesn't scale | `0fb0106` |
| **QML `Loader` + object-chain navigation is unworkable** | Fragile, hard to reason about | Replaced with a C++ `Router` via the `Bridge` | `aba290a`/`cb36cbc`/`d0b4f41` · `decisions/rejected.md` |
| **QML drag-and-drop is disproportionately hard** | Hours lost, nothing working; docs mostly C++-only | Use explicit move buttons instead | `53d69ea` · `decisions/rejected.md` |
| **Static libraries link out-of-order** | Build breaks unpredictably | Build sub-projects as shared libraries (+ export headers) | `5cbd7ff` · `decisions/rejected.md` |
| **QML `ListView` + C++ MVC is fragile** | Recurring, hard-to-track view/model glitches | Treated as a known hazard; some rare glitches left deliberately unfixed | `1799397`, `3e9e367` |
| **Naive random index repeats picks** | The same names keep coming up | No-repeat pool that reloads when depleted | `12eb978`/`7eff6bb` |

> Note: some Era-1/2 "QML only accepts a strict `int`" type pain is the historical cousin of the
> Qt-6 `unsigned int` Q_PROPERTY issue below — both come from QML's narrow numeric handling.

---

## Revival-era (2026) — see qt6-patterns.md / fix-patterns.md for the full fix

These are the big ones from the Qt 6 modernization. Summarized here for the catalog; the
detailed mechanism and code are in the linked files.

| Gotcha | One-line | Detail |
|--------|----------|--------|
| **`Q_DECLARE_OPAQUE_POINTER` blocks QML traversal** | Opaque-declaring a real QObject type makes QML read `obj.prop.sub` as `undefined`; `#include` + de-opaque the traversed types (but only the traversed branches, for build speed) | `qt6-patterns.md`, `decisions/architecture.md` → "QML Property-Chain Traversal" |
| **DB static-init deadlock** | C++11 mutex-guarded static local init deadlocks if a DB constructor re-enters its own `inst()`; `loadAll()` is the sole `load()` caller | `decisions/architecture.md`, `decisions/rejected.md` |
| **`qt_add_qml_module()` vs `app.qrc`** | Double-registering QML paths hangs `QQuickWidget::setSource()`; keep `app.qrc` + `qmlRegister*` only | `decisions/rejected.md` |
| **`QSurfaceFormat` MSAA on `QQuickWidget`** | MSAA on the offscreen FBO hangs Windows GPU drivers 40s+; remove the surface-format setup | `decisions/rejected.md` |
| **`unsigned int` Q_PROPERTY blank in `TextField`** | QML needs an explicit `.toString()` | `history.md` Phase 4 |
| **Strict QML ID scoping** | IDs no longer leak across file boundaries; thread shared refs as explicit `property var` | `history.md` Phase 4 |
| **`Q_PROPERTY` methods aren't callable from QML** | Use `Q_INVOKABLE` for functions QML calls (vs `Q_PROPERTY` for values) | `history.md` Phase 4 |
| **`Q_INVOKABLE` returning a parentless QObject gets GC'd** | Wrap returns in `qmlCppOwned()` to stop QML garbage-collecting them mid-session | `qt6-patterns.md`, `CLAUDE.md` |
| **Qt 6 Material 3 control heights** | `TextField`/`ComboBox` are taller than Qt-5-era hardcoded layouts assumed; pin heights / anchor below, don't fixed-offset | `reference/ui-patterns.md`, `history.md` Phase 8 |
| **`parent` briefly null in delegates during model reset** | Guard `parent ? parent.width : 0` in ListView/GridView delegates | `history.md` Phase 4 |
| **`=== NaN` / `isNaN` misuse** | `x === NaN` is always false in JS; use `isNaN(x)` | `history.md` Phase 4 |

---

## Deliberately accepted quirks (don't "fix" these)

A few Qt/QML behaviors had no clean fix and were consciously left alone. They're documented at
the call sites; listed here so nobody burns time re-discovering them.

### Disabling a `Button` while it's hovered/pressed leaves it visually stuck
`Button.hovered` / `Button.pressed` are **read-only**, so if you flip `enabled = false` on a
control while the cursor is over it (or mid-press), it gets stuck rendering in the
hovered/down state until the next interaction — looking like a frozen, greyed pill.

- **Pokémon details "Heal" button**: would disable itself the moment you clicked it (once the
  mon is healed there's nothing to heal), and stuck in the mouse-down look. Resolution: the
  `btn2.enabled` binding is **commented out** — the button just stays enabled. (`PokemonDetails.qml`)
- **Pokémart "Checkout" button**: `btn2.enabled: brg.marketModel.canAnyCheckout` flips off after
  a checkout while hovered, and sticks. The maintainer tried programmatically forcing `down` off before
  disabling and via a `Connections` hook — same result, because `pressed`/`hovered` are
  read-only — and accepted the cosmetic glitch ("a bit of an eyesore"). (`Pokemart.qml`)

If you must disable-on-click, expect this; the cleaner path is to leave the control enabled and
make the *action* a no-op when nothing applies.

### A QML call to a 5-bool C++ method only passes 4 args
In `PokemonDetails.qml`'s "Correct Data" action, `boxData.update(...)` was expanded to take 5
bools, but calling it from QML with 5 only ever delivered 4. The maintainer tried everything and gave
up; the workaround is to call the follow-on work explicitly — `update(true,true,true,true)` then
`correctMoves()` + `cleanupMoves()` directly. If you extend a `Q_INVOKABLE`'s argument list and
QML seems to drop the last argument, this is why — pass fewer args and call the remainder
explicitly, or bundle them.

### One rare ListView/MVC glitch left unfixed
A minor, rare item-list glitch was left in place deliberately (`3e9e367`) — most users won't
notice it and the fix risked introducing worse ListView bugs. The QML `ListView` + C++ MVC combo
is fragile (see the historical table); weigh any "fix" against the chance of new breakage.

---

## Meta-lesson

The throughline: **Qt's QML/C++ boundary is where the surprises live** — numeric types, property
exposure, object ownership, URL handling, and the meta-object requirements. When something
"impossible" happens at that boundary, assume a framework gotcha before assuming your own bug,
check this catalog and `qt6-patterns.md` first, and when you find a new one, **add a row here**
(and document the workaround at the call site).
