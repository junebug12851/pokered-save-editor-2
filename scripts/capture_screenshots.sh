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

exe="$build/screenshooter"
[ -x "$exe" ] || exe="$build/screenshooter.exe"
[ -x "$exe" ] || { echo "screenshooter binary not found in $build" >&2; exit 1; }

mkdir -p "$out"
echo "Capturing screenshots -> $out"
QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software "$exe" "$out"

if command -v python3 >/dev/null 2>&1; then
  echo "Assembling GIFs..."
  python3 "$repo/scripts/make_gifs.py" --dir "$out" || \
    echo "GIF assembly skipped (Pillow missing?); PNGs are in $out"
else
  echo "python3 not found; skipping GIF assembly (PNGs are in $out)"
fi

echo "Screenshot capture complete: $out"
