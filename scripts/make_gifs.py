#!/usr/bin/env python3
# Copyright 2026 Twilight
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""
Assemble the frame sequences captured by the `screenshooter` tool into animated GIFs.

The C++ capture tool writes numbered PNG frames into

    <screenshots>/frames/<name>/frame_000.png, frame_001.png, ...

This script turns each such <name> folder into <screenshots>/<name>.gif.

Repo-friendly + CI-friendly: pure Python with a single common dependency (Pillow),
argparse-driven, and a clear, non-crashing message if Pillow is missing. It only
READS the PNGs and writes GIFs -- it never touches the app or any save data.

Usage:
    python scripts/make_gifs.py [--dir <screenshots>] [--fps 5] [--loop 0]
                                [--keep-frames]

Defaults: --dir = <repo>/tmp/screenshots, --fps 5, --loop 0 (loop forever).
With no Pillow installed it prints how to install it and exits non-zero (so a CI
step fails loudly) -- but the PNG captures themselves are unaffected.
"""

import argparse
import os
import re
import sys

# Per-sequence frame duration overrides (ms). Anything not listed uses --fps.
# These match the cadences the capture tool grabs at, so the GIFs feel right.
DURATION_MS = {
    "name_anim": 220,   # in-game name preview animation
    "typing":    180,   # live typing
    "tab_cycle": 260,   # editor tab-cycle interaction
}

_NUM = re.compile(r"(\d+)")


def _frame_key(name):
    m = _NUM.search(name)
    return int(m.group(1)) if m else 0


def find_sequences(frames_root):
    """Return {seq_name: [ordered png paths]} for each frames/<name>/ folder."""
    seqs = {}
    if not os.path.isdir(frames_root):
        return seqs
    for name in sorted(os.listdir(frames_root)):
        folder = os.path.join(frames_root, name)
        if not os.path.isdir(folder):
            continue
        pngs = [f for f in os.listdir(folder) if f.lower().endswith(".png")]
        if not pngs:
            continue
        pngs.sort(key=_frame_key)
        seqs[name] = [os.path.join(folder, f) for f in pngs]
    return seqs


def build_gifs(shots_dir, fps, loop):
    try:
        from PIL import Image
    except ImportError:
        sys.stderr.write(
            "make_gifs.py: Pillow is not installed.\n"
            "  Install it with:  python -m pip install --user Pillow\n"
            "  (PNG screenshots were still captured; only GIF assembly is skipped.)\n"
        )
        return 1

    frames_root = os.path.join(shots_dir, "frames")
    seqs = find_sequences(frames_root)
    if not seqs:
        sys.stderr.write(
            "make_gifs.py: no frame sequences found under %s\n"
            "  (run the screenshooter tool first.)\n" % frames_root
        )
        return 1

    default_ms = max(1, int(round(1000.0 / max(1, fps))))
    made = 0
    for name, paths in seqs.items():
        try:
            frames = [Image.open(p).convert("RGBA") for p in paths]
        except Exception as e:  # noqa: BLE001 - report + continue to next seq
            sys.stderr.write("  [skip] %s: %s\n" % (name, e))
            continue
        if not frames:
            continue
        # GIF has no alpha; flatten onto white so anti-aliased edges read cleanly.
        flat = []
        for fr in frames:
            bg = Image.new("RGBA", fr.size, (255, 255, 255, 255))
            bg.alpha_composite(fr)
            flat.append(bg.convert("P", palette=Image.ADAPTIVE))
        out = os.path.join(shots_dir, name + ".gif")
        flat[0].save(
            out,
            save_all=True,
            append_images=flat[1:],
            duration=DURATION_MS.get(name, default_ms),
            loop=loop,
            optimize=True,
            disposal=2,
        )
        print("  [gif] %s  (%d frames)" % (os.path.relpath(out, shots_dir), len(flat)))
        made += 1

    print("make_gifs.py: wrote %d GIF(s) to %s" % (made, shots_dir))
    return 0 if made else 1


def main(argv=None):
    repo_default = os.path.normpath(
        os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tmp", "screenshots")
    )
    ap = argparse.ArgumentParser(description="Assemble screenshooter frame folders into GIFs.")
    ap.add_argument("--dir", default=repo_default,
                    help="screenshots dir (contains frames/<name>/). Default: <repo>/tmp/screenshots")
    ap.add_argument("--fps", type=int, default=5, help="default frames/sec for sequences (default 5)")
    ap.add_argument("--loop", type=int, default=0, help="GIF loop count, 0 = forever (default 0)")
    ap.add_argument("--keep-frames", action="store_true",
                    help="(no-op; frames are always kept -- flag reserved for future cleanup)")
    args = ap.parse_args(argv)
    return build_gifs(args.dir, args.fps, args.loop)


if __name__ == "__main__":
    raise SystemExit(main())
