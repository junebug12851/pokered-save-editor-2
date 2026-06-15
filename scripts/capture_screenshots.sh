#!/usr/bin/env bash
# Copyright 2026 Twilight  --  Apache-2.0 (see LICENSE).
#
# Portable (Linux/CI/Docker) driver for the screenshot + animation capture tool.
# Mirror of capture_screenshots.ps1 for the containerized Linux toolchain (docker/).
# Builds ONLY the screenshooter target, runs it offscreen (software backend), and
# assembles GIFs with Pillow. Output -> <repo>/tmp/screenshots (git-ignored).
#
# Nothing here writes a save byte; the tool only renders the UI.
#
# Usage: scripts/capture_screenshots.sh [outdir] [build-dir]
set -euo pipefail

repo="$(cd "$(dirname "$0")/.." && pwd)"
out="${1:-$repo/tmp/screenshots}"
build="${2:-$repo/build}"

if [ ! -f "$build/CMakeCache.txt" ]; then
  echo "Configuring test build dir ($build)..."
  cmake -S "$repo/projects" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Debug
fi

echo "Building screenshooter..."
cmake --build "$build" --target screenshooter

# The screenshooter exe lands at the build root on Windows (CMAKE_RUNTIME_OUTPUT_
# DIRECTORY) but under build/tests/ on Linux -- search for it rather than assume.
exe="$build/screenshooter"
[ -x "$exe" ] || exe="$build/screenshooter.exe"
[ -x "$exe" ] || exe="$(find "$build" -type f -name screenshooter 2>/dev/null | head -n1)"
[ -x "$exe" ] || exe="$(find "$build" -type f -name 'screenshooter.exe' 2>/dev/null | head -n1)"
[ -n "$exe" ] && [ -x "$exe" ] || { echo "screenshooter binary not found under $build" >&2; exit 1; }

mkdir -p "$out"
echo "Capturing screenshots -> $out"
# Prefer a REAL window under a virtual X display (xvfb): the GPU/llvmpipe path renders
# MultiEffect/layered content (Credits cards, Home disabled tiles, shadows) that the
# offscreen+software backend silently drops -- and it's still headless/CI-friendly.
if command -v xvfb-run >/dev/null 2>&1; then
  xvfb-run -a -s "-screen 0 1280x800x24" "$exe" "$out"
else
  echo "xvfb-run not found; falling back to offscreen+software (MultiEffect content will be MISSING)"
  QT_QPA_PLATFORM=offscreen PSE_FORCE_SOFTWARE=1 "$exe" "$out"
fi

if command -v python3 >/dev/null 2>&1; then
  echo "Assembling GIFs..."
  python3 "$repo/scripts/make_gifs.py" --dir "$out" || \
    echo "GIF assembly skipped (Pillow missing?); PNGs are in $out"
else
  echo "python3 not found; skipping GIF assembly (PNGs are in $out)"
fi

echo "Screenshot capture complete: $out"
