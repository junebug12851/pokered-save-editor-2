# System Map: `common` layer

The foundation library (`projects/common/src/pse-common`). Tiny -- six files -- but
everything else links against it. Fully documented in code; this is the conceptual
companion. See also the macro picture in [overview.md](overview.md).

## What's in it

| File | Provides |
|------|----------|
| `types.h` | The `var8`/`var16`/`var32`/`var64` fixed-width integer aliases (and the full `[us]var[#][sfe]` family they shorthand). |
| `random.h` / `.cpp` | `Random` -- the shared randomness source. |
| `utility.h` / `.cpp` | `Utility` -- shared helpers + the QML entry point + the QML-ownership guard. |
| `common_autoport.h` | The `COMMON_AUTOPORT` dllimport/export macro for the shared library. |

## `types.h` -- why it exists

A save editor is byte arithmetic end to end, so the project never trusts `int`/
`short`. Every integer is spelled as an explicit width via these aliases. The
everyday forms (`var8`, `var16`, `var32`, `var64`) resolve to the **exact**-width
types on purpose: a note in the file records that "fastest" types once silently
widened an 8-bit pointer to 32-bit in another project, so exact width is the safe
default here.

## `Random` -- domain-flavoured randomness

A thin QObject wrapper over `QRandomGenerator::global()`. It exists so randomization
code reads in intent ("flip a coin", "30% chance", "range a..b") instead of repeating
bounds math. Notable shape:

- Singleton (`Random::inst()`), private constructor.
- Range helpers guard degenerate/inverted inputs by returning `start` rather than
  feeding a non-positive span to `bounded()`.
- Every "chance" idea comes in an int (0-100) and a float (0.00-1.00) variant,
  because the int path can bias on small ranges -- the float path is the escape hatch.
- It is the engine behind `SaveFile::randomizeExpansion()` and the map/full
  randomization features.

## `Utility` -- helpers + the QML doorway for this layer

A QObject singleton exposed to QML as the context property `pseCommon`. Through it
QML reaches `pseCommon.random` and two string helpers (`encodeBeforeUrl` /
`decodeAfterUrl`, a hex round-trip used to pass name text through route fragments).

Its most load-bearing member is **`qmlProtectUtil()`**: the one-liner that pins a
C++ QObject to C++ ownership so the QML engine's garbage collector can't delete it.
Every database and save object in the project calls into this. Centralizing the
GC-ownership incantation here means exactly one place knows it. This is one half of
the project's answer to the "QML GC'd my parentless QObject" class of crashes; the
other half (`qmlCppOwned()` for `Q_INVOKABLE` returns) lives in the savefile layer.
Full mechanism: [../reference/qt-patterns.md](../reference/qt-patterns.md).

## How it connects outward

- `db`, `savefile`, and `app` all use `var*` types and `Random`/`Utility`.
- `Utility::qmlHook()` installs `pseCommon` into the QML context during app wiring.
- `Bridge` exposes `Utility::inst()` to QML as `brg.util` as well (see
  [overview.md](overview.md)).
