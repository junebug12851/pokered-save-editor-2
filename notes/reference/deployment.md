# Deployment / Releases (GitHub Actions)

How release binaries are built and published. CI (test-on-every-push) lives in
`.github/workflows/tests.yml`; **software deployment** lives in `.github/workflows/release.yml`;
the **GitHub Pages site** (docs + screenshots) is deployed by `.github/workflows/pages.yml`.
Built 2026-06-15.

## Policy (standing rules)

- **Releases are SOFTWARE releases only** — never an images-only or otherwise non-software release.
  The versioned `release.yml` release is the only kind; don't create side releases to host files.
- **Every Release has a well-written, user-facing description by default.** `release.yml`'s `Compose
  release notes` step builds a structured body, top→bottom: a plain-English "what the app is", a
  prerelease/unsigned note, **"What's new in this release"** (the *actual* changes — see below), **"How to
  get it"** (downloads table), and **Links** (Pages docs, full changelog, issues). softprops then appends
  the auto **"What's Changed"** (raw commit/PR list — the deepest technical layer).
  - **"What's new" is pulled from our living changelog automatically.** The step finds the previous
    release tag (`git describe --tags --abbrev=0 HEAD`, by *ancestry* — robust to the version-number
    reset), then extracts the entries **added to `notes/version/` since then** (`git diff <prev>..HEAD`,
    added lines, newest-first). Those entries are already plain-English ("what + why"), so the release
    note reflects exactly this deployment — no "go look elsewhere". (The release job checks out with
    `fetch-depth: 0` + `fetch-tags: true` so the diff/describe work.)
  - **Override for a big release:** drop a hand-written `notes/releases/<version>.md` and the step uses
    that verbatim for the "What's new" prose instead of the auto changelog.
- **README screenshots go on GitHub Pages**, not git or a release (zero repo-size growth, no third
  party). See "README screenshots (GitHub Pages)" below.

## What gets published

Each release attaches:

- **Windows portable `.zip`** — the `windeployqt`'d app (run-in-place; the "portable"
  and "zip" are the same artifact).
- **Windows installer `.exe`** — Inno Setup (`packaging/windows/installer.iss`).
- **Linux `.AppImage`** — portable, double-click to run (the cross-distro equivalent
  of the Windows installer/portable).
- **Linux portable `.tar.gz`** — the fully-populated AppDir tree (the `.zip` equivalent).
- **Docs `.zip`** — the generated Doxygen HTML site (`doxygen Doxyfile` → `docs/html`).
- **Screenshots `.zip`** — the UI **still PNGs** from the `screenshooter` tool (captured
  headless under `xvfb`). No automated GIFs — animated GIFs are added manually.

## GitHub Pages site — docs + screenshots (`.github/workflows/pages.yml`)

`pages.yml` runs on every push to `main` and deploys ONE Pages site holding both the docs and the
screenshots (a repo gets a single Pages site, so they share one deploy):

```
https://junebug12851.github.io/pokered-save-editor-2/                   # Doxygen docs home (the ROOT)
https://junebug12851.github.io/pokered-save-editor-2/screenshots/<name> # UI images + a gallery
```

The **Doxygen home is the Pages root** (the docs HTML is copied to the site root). The docs main page
is the README (`USE_MDFILE_AS_MAINPAGE`, left untouched); **Screenshots** + **GitHub** appear as custom
top-nav tabs on every docs page — added without editing the README by generating the default
`DoxygenLayout.xml` (`doxygen -l`), injecting two `<tab type="user">` entries, and building with that
layout. It captures the UI headless (`screenshooter` under xvfb → `screens/*.png` + `editor/*.png`,
still PNGs only — no GIFs), runs Doxygen, assembles `site/` (docs at root + `screenshots/`), and
deploys via `actions/configure-pages` → `upload-pages-artifact` → `deploy-pages`.

**Referencing screenshots from the docs:** use the absolute Pages URL
(`https://junebug12851.github.io/pokered-save-editor-2/screenshots/<name>`) — it resolves the same
whether the page is viewed as Doxygen-on-Pages, on GitHub (the notes `.md` are also rendered there), or
in the README. (From a docs page you can also use the relative `screenshots/<name>`.)

Why Pages: images + docs live on the Pages CDN — **not** in the git repo (no size growth, no LFS) and
**not** in a release (releases stay software-only) — and CI keeps them current. Pages was enabled with
the **GitHub Actions** build source (`gh api -X POST repos/:owner/:repo/pages -f build_type=workflow`);
the workflow needs `permissions: pages: write` + `id-token: write` and the `github-pages` environment.
(Considered + rejected for images: a rolling images-only release — violates the software-only rule; an
orphan branch — repo object-store bloat; Imgur — image purges / deprecated API / needs a secret.)
The versioned release still attaches its own `*-docs.zip` + `*-screenshots.zip` as archival snapshots.

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

## First-run shakeout (DONE — `v0.7.2-alpha` shipped 2026-06-15)

The pipeline ran green and published its first release. The shakeout fixes (now baked into
the workflow) were all Windows + one script path:

- **`windeployqt --compiler-runtime` is MSVC/g++ only** (errors on llvm-mingw) → dropped it;
  the llvm-mingw runtime DLLs (libc++ / libunwind / libwinpthread) are copied from the
  compiler `bin` by hand.
- **Invoke `windeployqt` by FULL PATH** (`$QT_ROOT_DIR/bin/windeployqt.exe`). A stray
  windeployqt on PATH (a different Qt) printed "Unable to find the platform plugin" and
  deployed nothing. There's also a manual Qt-DLL / plugin-set / `qml` fallback and a hard
  assertion that `Qt6Core.dll` + `platforms/qwindows.dll` ended up deployed.
- **Inno Setup:** `ISCC.exe` is at `${Env:ProgramFiles(x86)}\Inno Setup 6\` and is
  **pre-installed** on windows-latest. The braces matter — `$Env:ProgramFiles(x86)`
  mis-parses in PowerShell.
- **`capture_screenshots.sh`** searches for the `screenshooter` binary (it's at `build/tests/`
  on Linux, the build root on Windows). Screenshots render under xvfb + llvmpipe (MultiEffect
  included).
- **AppImage on CI** runs the `linuxdeploy` AppImages with `APPIMAGE_EXTRACT_AND_RUN=1`
  (no FUSE on runners) — worked first try.

To iterate further: edit the workflow, push, and watch with `gh run watch` / `gh run view
--log-failed` (the GitHub CLI is installed). For a no-publish trial, run **release** from the
Actions tab (or `gh workflow run release.yml -f dry_run=true`).

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
