# Versioning -- the scheme, the source of truth, and how to maintain it

The project uses **Semantic Versioning 2.0.0** (SemVer) as the people-facing number,
with the **git commit appended as build metadata** so dev/CI builds stay traceable to an
exact commit. This reconciles the two systems: SemVer is the simple, linear, human-readable
number for people-to-people communication; the commit hash rides along (after a `+`, which is
literally what SemVer reserves "build metadata" for) so nothing is lost for debugging.

## The number

`MAJOR.MINOR.PATCH[-prerelease][+build]`

- **MAJOR** -- `0` while pre-release; `1` at the first real, stability-promising release.
- **MINOR** -- new features / notable changes.
- **PATCH** -- bug fixes and small tweaks.
- **-prerelease** -- `-alpha` (early), `-beta` (feature-stable), `-rc.1` (release candidate);
  dropped entirely for a final release. While we're at `0.x`, the `0` already signals
  "in development, things still move," so `-alpha`/`-beta` is an extra honesty layer, not a
  requirement.
- **+build** -- *computed, never authored*. The build appends `+g<shorthash>` and `.dirty`
  when the tree has uncommitted tracked changes, e.g. `0.7.0-alpha+gbd88c0c2.dirty`.

### Why we started at `0.7.0-alpha`

Derived from the actual project, not picked arbitrarily. The codebase is large and mature
(760 commits across 2019-2020 and the 2026 revival) with the hardest thing -- **byte-exact
save read/write/roundtrip** -- done and tested, plus most editor screens built and polished.
But it is **not released**, is in an active UI-polish phase, still has an end-to-end
save/reopen verification pass and a documentation pass outstanding, and no packaging/installer
yet. That profile is a feature-rich **late alpha**: clearly pre-1.0, but well past the early
`0.1`-`0.3` range. `0.7.0-alpha` says exactly that and leaves a clean runway -- `0.8` as things
stabilize, `0.9`/`-rc` as a release nears, `1.0.0` at the first real release. (There were never
real version numbers before now -- CMake just said `1.0.0` and `boot.cpp` said `v1.0.0`, never
synced -- so this is the first *formal* baseline rather than a renumbering of history.)

The number is one line in `VERSION`; if the maintainer judges the maturity differently, it's a
one-line change.

## Single source of truth: `VERSION`

The repo-root **`VERSION`** holds the number on one line (comments with `#` allowed). It is
the *only* place a human edits the version. (It's the bare `VERSION` -- the changelog folder
that used to be `version/` was renamed to `version-notes/` (since moved to `notes/version-info/`), so on case-insensitive Windows there
is no longer a `VERSION`/`version/` collision to dodge.)

Everything else is derived from it at build time. **Do not** hardcode a version anywhere else.

## How it propagates (the machine)

```
VERSION  (you edit this)
   |
   v
projects/CMakeLists.txt
   - include(cmake/PSEVersion.cmake); pse_read_version(../VERSION)
   - project(PokeredSaveEditor VERSION <core>)          # numeric M.m.p
   - generates  <build>/generated/pse_version.h         # via cmake/pse_version.h.in
        * configure-time pass (so the first compile has it)
        * build-time refresh target `pse_version_gen` re-reads git each build
          (configure_file only rewrites on change -> no needless recompiles)
   |
   +--> projects/app/src/boot/boot.cpp
   |       #include "pse_version.h"
   |       QApplication::setApplicationVersion(PSE_VERSION_FULL);   # full, with +g<hash>
   |
   +--> About screen  (ui/app/screens/modal/About.qml)
   |       shows  "v" + Qt.application.version.split("+")[0]        # clean, hash stripped
   |
   +--> Windows .exe resource  (app/pse_app.rc.in -> pse_app.rc)    # FILEVERSION / strings
           (toggle with -DPSE_GEN_WINRC=OFF if a kit's RC compiler chokes)
```

`pse_version.h` exposes: `PSE_VERSION_MAJOR/MINOR/PATCH`, `PSE_VERSION_STRING` (human, e.g.
`"0.7.0-alpha"`), `PSE_GIT_HASH`, `PSE_GIT_DIRTY`, and `PSE_VERSION_FULL` (human + build
metadata). The header is generated into the build tree and is **not** committed.

### Two display forms, on purpose

- **`Qt.application.version` / logs / crash reports / `.exe` details** = the **full** string
  (`0.7.0-alpha+gbd88c0c2`) -- precise, ties a build to a commit.
- **About screen footer** = the **clean** string (`v0.7.0-alpha`) -- the `+g...` metadata is
  stripped for a tidy human-facing display. (Want the hash visible there too? Remove the
  `.split("+")[0]` in `About.qml`.)

A tagged, clean release build has no `+g...` at all, so both forms read simply `0.7.0`.

## Files involved

| File | Role |
|------|------|
| `VERSION` | The number (the only thing you edit) |
| `projects/cmake/PSEVersion.cmake` | Parse VERSION; git metadata; compose full string |
| `projects/cmake/PSEVersionGen.cmake` | Build-time script: refresh git metadata, regenerate header |
| `projects/cmake/pse_version.h.in` | Template for the generated C++ header |
| `projects/CMakeLists.txt` | Reads VERSION before `project()`; wires generation + RC option |
| `projects/app/CMakeLists.txt` | Adds the generated include dir + `pse_version_gen` dep; compiles the .rc |
| `projects/app/pse_app.rc.in` | Windows VERSIONINFO template |
| `projects/app/src/boot/boot.cpp` | Sets the runtime version from `PSE_VERSION_FULL` |
| `projects/app/ui/app/screens/modal/About.qml` | Displays the (cleaned) version |

## How to bump the version

1. Edit the version line in **`VERSION`** (e.g. `0.7.0-alpha` -> `0.8.0-alpha`, or
   `-> 0.7.1`). That's the whole change.
2. Re-run CMake configure (the kit build, or `cmake -S projects -B build`) so `project(VERSION)`
   and the generated header pick up the new core number. (Just rebuilding refreshes the *git
   metadata* but not a changed *number* -- a number change needs a reconfigure.)
3. Build + run; the About screen and `.exe` details reflect the new number.

## Releases and git tags

When cutting a real release:

1. Bump `VERSION` to the release number (drop any `-alpha`/`-beta` for a final release).
2. Commit on `dev`, run the full suite green, fast-forward `main` (the standing workflow).
3. **Tag the release commit** `vX.Y.Z` and push the tag:
   `git tag -a v0.8.0 -m "v0.8.0" && git push origin v0.8.0`.
   The tag is the accurate anchor that maps the SemVer number to one commit and lets
   `git describe` derive versions. (Tags are created on request -- not automatically.)

A build from a clean, tagged checkout shows the plain number with no `+g...` suffix.

## Relationship to the changelog (`notes/version-info.md` / `notes/version-info/`)

These are **complementary, not the same thing**:

- **`VERSION` + this scheme** = the *identity* of a build / release (one number).
- **`notes/version-info.md` + `notes/version-info/`** = the plain-English *narrative history*, one entry per commit
  (see `reference/version-history.md`). It explains what changed; the version number labels
  where you are.

When you tag a release, it's worth noting the version in the changelog month file too, but the
changelog stays per-commit and is maintained separately (by hand, on request).

## Maintenance (keep this current by default)

- Bump `VERSION` at the commit where a feature/fix warrants it (the git hash updates itself;
  you never touch metadata by hand).
- If you add a new place that needs the version, derive it from `pse_version.h` (C++) or
  `Qt.application.version` (QML) -- never add a new hardcoded literal.
- If the RC step ever fails on a toolchain, configure with `-DPSE_GEN_WINRC=OFF` and note it.
