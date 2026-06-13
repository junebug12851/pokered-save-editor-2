# Linux build/test container

Compile and test the project on Linux in a Docker container — including the
things the Windows kit can't do: **ASan/UBSan** (broken on llvm-mingw) and a
**real-X (Xvfb)** GUI run. Mirrors the kit toolchain (**Qt 6.11 + clang**).

## Quick start (PowerShell, from the repo root)

```powershell
.\docker\dtest.ps1            # standard: build + full ctest (offscreen)
.\docker\dtest.ps1 asan       # AddressSanitizer + UBSan ctest run
.\docker\dtest.ps1 xvfb       # run under a real virtual X server (xcb)
.\docker\dtest.ps1 coverage   # llvm-cov per-module coverage report
.\docker\dtest.ps1 all        # every variant in sequence
```

Flags: `-Rebuild` (rebuild the image after editing the Dockerfile),
`-Clean` (wipe the persistent build volume for a from-scratch source build).

## How it works

- **`Dockerfile`** bakes the toolchain once: clang/lld/llvm, CMake + Ninja,
  Qt 6.11 (via `aqtinstall`, `linux_gcc_64` + qtcharts), the headless-GUI libs,
  Xvfb, and ccache. The repo is **not** in the image.
- At runtime the repo is bind-mounted **read-only** at `/host`, and
  **`run-tests.sh`** `rsync`s it into a persistent named volume `pse-build`
  mounted at `/build` — an ext4 filesystem inside the Docker VM. Building there
  instead of over the Windows/WSL bind mount is dramatically faster, and the
  build tree + ccache persist across runs, so repeat runs are incremental.
- Each variant configures its own build dir under `/build/out/` and runs
  `cmake --build … --target tests_all` then `ctest`. `tests_all` is the CMake
  aggregate of every registered suite, so new tests are picked up automatically.

## Variants

| Variant    | Build dir              | What it does |
|------------|------------------------|--------------|
| `standard` | `/build/out/standard`  | Debug build, full `ctest` via `QT_QPA_PLATFORM=offscreen`. |
| `asan`     | `/build/out/asan`      | `-fsanitize=address,undefined` build + `ctest`. The memory/UB net. |
| `xvfb`     | `/build/out/standard`  | Reuses the standard build; runs `ctest` under `xvfb-run` requesting `xcb`. |
| `coverage` | `/build/out/coverage`  | `-fprofile-instr-generate -fcoverage-mapping` (+ `PSE_SHARED_APPCORE=ON`); prints an `llvm-cov` per-module summary and writes HTML to `/build/out/coverage/html`. |

## Extracting the coverage HTML to the host

```powershell
docker run --rm -v pse-build:/build -v "${PWD}:/out" alpine `
    sh -c "cp -r /build/out/coverage/html /out/coverage-html"
```

## Notes / caveats

- **Qt version vs CI.** This container is **6.11** (the kit). The GitHub
  `linux-asan` CI job uses **6.8.3 + gcc**; this is the closer-to-runtime local
  reproduction, and the place ASan/UBSan actually run.
- **Per-test offscreen override.** Several GUI tests pin
  `QT_QPA_PLATFORM=offscreen` in CMake; that per-test env wins over the `xvfb`
  variant's `xcb` request. The `xvfb` path still gives a live display to any
  test that needs one.
- First run builds the image (downloads Qt) — a few minutes. Afterwards the
  image is cached and only the (incremental) compile runs.
