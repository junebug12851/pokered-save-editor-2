# Deployment / Releases (GitHub Actions)

How release binaries are built and published. CI (test-on-every-push) lives in
`.github/workflows/tests.yml`; **deployment** lives in `.github/workflows/release.yml`.
Built 2026-06-15.

## What gets published

Each release attaches:

- **Windows portable `.zip`** — the `windeployqt`'d app (run-in-place; the "portable"
  and "zip" are the same artifact).
- **Windows installer `.exe`** — Inno Setup (`packaging/windows/installer.iss`).
- **Linux `.AppImage`** — portable, double-click to run (the cross-distro equivalent
  of the Windows installer/portable).
- **Linux portable `.tar.gz`** — the fully-populated AppDir tree (the `.zip` equivalent).
- **Docs `.zip`** — the generated Doxygen HTML site (`doxygen Doxyfile` → `docs/html`).
- **Screenshots `.zip`** — the UI PNGs + GIFs from the `screenshooter` tool (captured
  headless under `xvfb`).

## When it runs (the version gate)

Trigger: **push to `main`** (which is FF-only from green `dev`, so it's always
all-green). Gate: the tag **`v<VERSION>` must not already exist** — i.e. `VERSION`
has been bumped since the last release. So:

- Bump `VERSION` (PATCH/MINOR) in a commit → the next time `main` advances, that commit
  cuts the release and creates the tag `vX.Y.Z`.
- A `main` commit that did **not** bump `VERSION` → the tag already exists → the run is
  a no-op (no duplicate release). This is exactly why tags key off `VERSION`.
- A `-alpha`/`-beta`/`-rc` label marks the GitHub Release as a **prerelease**.

`workflow_dispatch` allows a **dry run**: it builds + uploads everything as workflow
artifacts but (with `dry_run = true`, the default) does **not** create the Release —
use it to shake out the build without publishing.

## Toolchain (mirrors `tests.yml`)

Qt **6.8.3** via `jurplel/install-qt-action@v4`. Windows = `win64_llvm_mingw` +
`tools_llvm_mingw1706`, generator `MinGW Makefiles`. Linux = `linux_gcc_64`, Ninja.
Built **Release** (`-DCMAKE_BUILD_TYPE=Release`); the app target is `PokeredSaveEditor`.
Linux is built with tests **ON** (default) so the `screenshooter` target exists for the
screenshot step; Windows builds with `-DPSE_BUILD_TESTS=OFF` (app only).

## First-run shakeout (expected)

GitHub Actions can't be fully reproduced locally, so the first real run may need small
tweaks (same caveat as `tests.yml`). Known watch points, flagged inline in the workflow:

- **`windeployqt` runtime DLLs.** `--compiler-runtime` should pull the llvm-mingw runtime
  (libc++ / libunwind / libwinpthread); if the portable app fails to start with a
  missing-DLL error, copy the offending DLL from the llvm-mingw `bin` in that step.
- **`linuxdeploy-plugin-qt` QML discovery.** Driven by `QML_SOURCES_PATHS=projects/app/ui`
  and `QMAKE=$QT_ROOT_DIR/bin/qmake`; if a QML import is missing at runtime, add its path.
- **Inno Setup path.** `ISCC.exe` is invoked from `%ProgramFiles(x86)%\Inno Setup 6\`
  after `choco install innosetup`.
- **AppImage on CI** runs the `linuxdeploy` AppImages with `APPIMAGE_EXTRACT_AND_RUN=1`
  (no FUSE on runners).

To iterate: push the workflow, open the repo's **Actions** tab, run **release** via
"Run workflow" with `dry_run` checked, and read the logs. (Or install the GitHub CLI
`gh` to `gh workflow run release.yml -f dry_run=true` and `gh run watch`.)

## Code signing (not done — intentional)

Binaries ship **unsigned**: Windows SmartScreen will warn on the installer, and an
unsigned AppImage is normal. Signing needs a paid Windows code-signing cert (and, for a
future macOS build, the Apple Developer program + notarization). Fine for an alpha;
revisit before a 1.0.

## Supporting files

- `.github/workflows/release.yml` — the pipeline.
- `packaging/windows/installer.iss` — Inno Setup script (version/source/output/icon via
  `/D` defines).
- `packaging/linux/PokeredSaveEditor.desktop` — AppImage desktop entry (`Icon=` matches
  the `--icon-filename` the workflow passes).
- Icon: `projects/app/assets/icons/app/icon.ico` (Windows) / `256x256.png` (AppImage).
- Release notes: `generate_release_notes: true` (auto from commits); the hand-written
  per-version changelog lives in `notes/version/`.
