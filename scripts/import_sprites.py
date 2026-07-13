"""Import every overworld sprite's ARTWORK from pret/pokered -- the other 71.

Until now the repo carried exactly one sprite sheet: `red.png`, the player. That was enough
to draw the player and nothing else. The map screen needs all 72.

WHAT IT PRODUCES

  projects/app/assets/sprites/overworld.png   one atlas, 72 cells of 16x96 px (1152 x 96),
                                              cell N-1 = sprite picture id N, greyscale, in
                                              exactly the format MapEngine already reads
  projects/app/src/engine/spriteart.h         a generated table: frames per sprite id

WHERE THE ORDER COMES FROM

`data/sprites/sprites.asm` -> `SpriteSheetPointerTable`. Its rows are in picture-id order
(row 1 = SPRITE_RED = picture id 1 = "Player"), and each names a graphics label. The labels
resolve to files through `gfx/sprites.asm` (`RedSprite:: INCBIN "gfx/sprites/red.2bpp"`), and
the .2bpp sits next to a .png of the same name.

Five ids share a sheet with another (the two Old Ambers, the three sleeping Gamblers) -- the
ROM points two or three rows at the same label. That is not a bug and we reproduce it.

THE THREE SHAPES -- and the middle one is the interesting one

  16 x 96 px   a WALKING person: SIX 16x16 frames -- stand down, stand up, stand LEFT, then
               the three walking ones. There is no right-facing frame; the game draws right
               as left, X-flipped. (sprites.md, Part 2.)
  16 x 48 px   a STILL person: THREE frames -- stand down, stand up, stand left, and that is
               all. These people never take a step, so the art for it was never drawn. This
               is the other half of the Game Boy's "9 walking + 2 still" sprite budget
               (sprite-sets.md) -- Nurses, Guards, Mom, the Gameboy Kid.
  16 x 16 px   a ball / boulder / fossil: ONE frame. The game's facing table sends every
               facing and every animation frame to the same quad.

SELF-VALIDATING. It refuses to write anything unless all 72 ids resolve, every file exists,
and every image is one of the two legal shapes. Run it and it either tells you it is right,
or it tells you exactly what isn't.

    python scripts/import_sprites.py            (add --check to verify without writing)
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
POKERED = Path(r"C:\Users\juneh\Documents\projects\pokered")

ATLAS = REPO / "projects" / "app" / "assets" / "sprites" / "overworld.png"
HEADER = REPO / "projects" / "app" / "src" / "engine" / "spriteart.h"
SPRITES_JSON = REPO / "projects" / "db" / "assets" / "data" / "sprites.json"

CELL_W, CELL_H = 16, 96
SPRITE_COUNT = 72


def parse_sheet_table() -> list[str]:
    """SpriteSheetPointerTable, in picture-id order -> the graphics label of each."""
    text = (POKERED / "data" / "sprites" / "sprites.asm").read_text(encoding="utf-8")

    labels: list[str] = []
    for line in text.splitlines():
        line = line.strip()
        if not line.startswith("overworld_sprite "):
            continue   # skips the MACRO definition line, which starts with "MACRO"
        m = re.match(r"overworld_sprite\s+(\w+)\s*,", line)
        if not m:
            raise SystemExit(f"could not parse a table row: {line!r}")
        labels.append(m.group(1))

    return labels


def parse_label_files() -> dict[str, str]:
    """gfx/sprites.asm -> {label: basename}, e.g. RedSprite -> red."""
    text = (POKERED / "gfx" / "sprites.asm").read_text(encoding="utf-8")

    out: dict[str, str] = {}
    for m in re.finditer(r'(\w+)::\s*INCBIN\s+"gfx/sprites/([\w]+)\.2bpp"', text):
        out[m.group(1)] = m.group(2)
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true", help="verify, write nothing")
    args = ap.parse_args()

    if not POKERED.exists():
        print(f"SKIP: no pret/pokered clone at {POKERED}")
        return 2

    from PIL import Image

    labels = parse_sheet_table()
    files = parse_label_files()

    # ── validate before writing a single byte ───────────────────────────────────────────
    problems: list[str] = []

    if len(labels) != SPRITE_COUNT:
        problems.append(f"SpriteSheetPointerTable has {len(labels)} rows, expected {SPRITE_COUNT}")

    resolved: list[tuple[int, str, Path, int]] = []   # (id, label, png, frames)

    for i, label in enumerate(labels[:SPRITE_COUNT], start=1):
        base = files.get(label)
        if base is None:
            problems.append(f"id {i}: label {label} has no INCBIN in gfx/sprites.asm")
            continue

        png = POKERED / "gfx" / "sprites" / f"{base}.png"
        if not png.exists():
            problems.append(f"id {i}: {png} does not exist")
            continue

        with Image.open(png) as im:
            w, h = im.size

        if (w, h) == (16, 96):
            frames = 6      # a walking person
        elif (w, h) == (16, 48):
            frames = 3      # a still person -- no walk cycle was ever drawn
        elif (w, h) == (16, 16):
            frames = 1      # a ball / boulder / fossil
        else:
            problems.append(f"id {i}: {png.name} is {w}x{h} -- not a legal sprite shape")
            continue

        resolved.append((i, label, png, frames))

    if problems:
        print("REFUSING to import -- the data does not check out:\n")
        for p in problems:
            print(f"  * {p}")
        return 1

    # Cross-check against our own DB: same count, and the ids line up 1..72.
    import json
    db = json.loads(SPRITES_JSON.read_text(encoding="utf-8"))
    if len(db) != SPRITE_COUNT or [s["ind"] for s in db] != list(range(1, SPRITE_COUNT + 1)):
        print("REFUSING to import -- sprites.json is not 72 entries with ind 1..72")
        return 1

    print(f"all {len(resolved)} sprite sheets resolved and are a legal shape")
    dupes: dict[str, list[int]] = {}
    for i, label, _, _ in resolved:
        dupes.setdefault(label, []).append(i)
    shared = {k: v for k, v in dupes.items() if len(v) > 1}
    for label, ids in shared.items():
        names = ", ".join(f"{i} ({db[i - 1]['name']})" for i in ids)
        print(f"  shared sheet {label}: ids {names}")

    shapes = {6: 0, 3: 0, 1: 0}
    for _, _, _, f in resolved:
        shapes[f] += 1
    print(f"  {shapes[6]} walking people (6 frames), {shapes[3]} still people (3 frames), "
          f"{shapes[1]} one-frame objects")

    if args.check:
        print("\n--check: nothing written")
        return 0

    # ── build the atlas ─────────────────────────────────────────────────────────────────
    # Greyscale, cell N-1 = picture id N. Colour index 0 (black, value 0) is the sprite's
    # cut-out and is what the renderer turns transparent -- so the empty part of a
    # one-frame cell is 0 too, and reads as "nothing there".
    atlas = Image.new("L", (CELL_W * SPRITE_COUNT, CELL_H), 0)

    for i, _label, png, _frames in resolved:
        with Image.open(png) as im:
            atlas.paste(im.convert("L"), ((i - 1) * CELL_W, 0))

    ATLAS.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(ATLAS, optimize=True)
    print(f"\nwrote {ATLAS.relative_to(REPO)}  ({atlas.width}x{atlas.height})")

    # ── build the frame table ───────────────────────────────────────────────────────────
    rows = []
    for i, label, png, frames in resolved:
        rows.append(f"  {frames},   // {i:>2}  {db[i - 1]['name']}")

    HEADER.write_text(
        "/*\n"
        "  * Copyright 2026 Twilight\n"
        "  *\n"
        "  * Licensed under the Apache License, Version 2.0 (the \"License\");\n"
        "  * you may not use this file except in compliance with the License.\n"
        "  * You may obtain a copy of the License at\n"
        "  *\n"
        "  *   http://www.apache.org/licenses/LICENSE-2.0\n"
        "  *\n"
        "  * Unless required by applicable law or agreed to in writing, software\n"
        "  * distributed under the License is distributed on an \"AS IS\" BASIS,\n"
        "  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
        "  * See the License for the specific language governing permissions and\n"
        "  * limitations under the License.\n"
        "*/\n"
        "#pragma once\n"
        "\n"
        "/**\n"
        " * @file spriteart.h\n"
        " * @brief GENERATED by scripts/import_sprites.py -- do not edit by hand.\n"
        " *\n"
        " * How many 16x16 frames each overworld sprite's artwork holds.\n"
        " *\n"
        " *   6  a WALKING person: stand down, stand up, stand LEFT, then the three walking\n"
        " *      frames. There is no right-facing frame -- the game draws right as left,\n"
        " *      X-flipped.\n"
        " *   3  a STILL person: stand down, stand up, stand left. No walk cycle was ever\n"
        " *      drawn for them (the Game Boy's sprite budget is 9 walking + 2 still).\n"
        " *   1  a ball / boulder / fossil: the game's facing table sends every facing and\n"
        " *      every animation frame to the same quad.\n"
        " *\n"
        " * Indexed by picture id (1-based); index 0 is a placeholder so the ids read straight.\n"
        " * The artwork itself is one atlas: :/assets/sprites/overworld.png, 72 cells of\n"
        " * 16x96 px, cell (id - 1). See notes/reference/sprites.md.\n"
        " */\n"
        "\n"
        f"constexpr int spriteArtCount = {SPRITE_COUNT};\n"
        "\n"
        f"constexpr int spriteArtFrames[{SPRITE_COUNT} + 1] = {{\n"
        "  0,   //  0  (not a sprite -- picture id 0 means the slot is unused)\n"
        + "\n".join(rows) + "\n"
        "};\n",
        encoding="utf-8",
    )
    print(f"wrote {HEADER.relative_to(REPO)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
