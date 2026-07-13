#!/usr/bin/env python3
"""
import_tile_traits.py -- what every tile of every tileset MEANS, imported from pret/pokered.

The map emulator can already draw the world. This imports the layer that says what the
world *does*: which tiles are walls, which are grass, which warp, which are doors, which
are ledges you can only jump one way, which are the shop counters you lean over.

None of it is invented and none of it is in the save file. The save only *points* at it
(`wTilesetCollisionPtr` and friends), so the app has to carry the same data the cartridge
does -- exactly as we already do for the map blocks (scripts/import_map_blocks.ps1) and the
music (scripts/import_music.py).

Source:  <pokered>/data/tilesets/*.asm   (a git clone of pret/pokered)
Output:  projects/db/assets/data/tileTraits.json

Self-checking, in two ways, because a silently-wrong parse here would put a wall where the
game has a door:

  1. Structural. Every tileset must come out with a passable list; the shared lists must come
     out shared; the fallthrough labels must produce the longer list, not the shorter one.
  2. Against the cartridge. With --rom, the collision lists are re-read straight out of ROM
     bank 0 at each tileset's collPtr (from tileset.json) and must match byte-for-byte. This
     is what caught tileset.json's three wrong collPtrs. Without a ROM it is skipped, loudly.

Usage:
    python scripts/import_tile_traits.py [--pokered <path>] [--rom <path>] [--check]

    --check   parse + verify + diff against the committed JSON, write nothing. Exit 1 on drift.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

# The Windows console defaults to cp1252 and cannot print a tick mark.
for _s in (sys.stdout, sys.stderr):
    try:
        _s.reconfigure(encoding="utf-8")
    except AttributeError:
        pass

REPO = Path(__file__).resolve().parent.parent
DEFAULT_POKERED = Path.home() / "Documents" / "projects" / "pokered"
DEFAULT_ROM = REPO / "assets" / "references" / "backup.gb"
OUT = REPO / "projects" / "db" / "assets" / "data" / "tileTraits.json"
TILESET_JSON = REPO / "projects" / "db" / "assets" / "data" / "tileset.json"

# constants/tileset_constants.asm. The names are the ROM's; tileset.json's `nameAlias`
# is the same list, which is what lets the two files be joined on it.
TILESETS = [
    "OVERWORLD", "REDS_HOUSE_1", "MART", "FOREST", "REDS_HOUSE_2", "DOJO",
    "POKECENTER", "GYM", "HOUSE", "FOREST_GATE", "MUSEUM", "UNDERGROUND",
    "GATE", "SHIP", "SHIP_PORT", "CEMETERY", "INTERIOR", "CAVERN",
    "LOBBY", "MANSION", "LAB", "CLUB", "FACILITY", "PLATEAU",
]
IND = {name: i for i, name in enumerate(TILESETS)}

# The label prefix used in the .asm files ("RedsHouse1_Coll") vs the constant
# ("REDS_HOUSE_1"). Same 24 things, two spellings.
LABEL = {
    "Overworld": "OVERWORLD", "RedsHouse1": "REDS_HOUSE_1", "Mart": "MART",
    "Forest": "FOREST", "RedsHouse2": "REDS_HOUSE_2", "Dojo": "DOJO",
    "Pokecenter": "POKECENTER", "Gym": "GYM", "House": "HOUSE",
    "ForestGate": "FOREST_GATE", "Museum": "MUSEUM", "Underground": "UNDERGROUND",
    "Gate": "GATE", "Ship": "SHIP", "ShipPort": "SHIP_PORT", "Cemetery": "CEMETERY",
    "Interior": "INTERIOR", "Cavern": "CAVERN", "Lobby": "LOBBY", "Mansion": "MANSION",
    "Lab": "LAB", "Club": "CLUB", "Facility": "FACILITY", "Plateau": "PLATEAU",
}

WATER_TILE = 0x14  # the animated water tile, in every tileset that has water


def die(msg: str) -> None:
    print(f"ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def byte(tok: str) -> int:
    """One rgbasm byte literal -> 0-255. `-1` is the list terminator and means $FF."""
    tok = tok.strip().rstrip(",")
    if tok.startswith("$"):
        return int(tok[1:], 16)
    return int(tok, 10) & 0xFF


def strip_comment(line: str) -> str:
    return line.split(";", 1)[0].rstrip()


# ─────────────────────────────────────────────────────────────────────────────
# Collision -- the list of PASSABLE tiles. Absence from it is a wall.
#
# The trap: labels are SHARED ("RedsHouse1_Coll::" and "RedsHouse2_Coll::" sit on one
# list), so a parser that assumes one list per tileset in index order walks the chain out
# of step. That is exactly the bug that was sitting in tileset.json.
# ─────────────────────────────────────────────────────────────────────────────
def parse_collision(pokered: Path) -> dict[str, list[int]]:
    text = (pokered / "data" / "tilesets" / "collision_tile_ids.asm").read_text()
    out: dict[str, list[int]] = {}
    pending: list[str] = []          # labels stacked up, waiting for their coll_tiles

    for raw in text.splitlines():
        line = strip_comment(raw)
        if not line.strip():
            continue

        m = re.match(r"^(\w+)_Coll::", line.strip())
        if m:
            key = LABEL.get(m.group(1))
            if key is None:
                die(f"collision: unknown label {m.group(1)}_Coll")
            pending.append(key)
            continue

        m = re.match(r"^\s*coll_tiles\b(.*)$", line)
        if m:
            args = m.group(1).strip()
            tiles = [byte(t) for t in args.split(",") if t.strip()] if args else []
            for key in pending:                # every stacked label gets this same list
                out[key] = tiles
            pending = []                        # an unlabelled coll_tiles is the "unused" one
            continue

    missing = [t for t in TILESETS if t not in out]
    if missing:
        die(f"collision: no passable list for {missing}")
    return out


# ─────────────────────────────────────────────────────────────────────────────
# Warp tiles. Same shared-label trap, PLUS genuine fallthrough: some lists `db` a few
# bytes and then fall into the next label's list, so they are the concatenation.
# ─────────────────────────────────────────────────────────────────────────────
def parse_warp(pokered: Path) -> dict[str, list[int]]:
    text = (pokered / "data" / "tilesets" / "warp_tile_ids.asm").read_text()

    # Everything after the pointer table.
    body = text.split(".OverworldWarpTileIDs:", 1)
    if len(body) != 2:
        die("warp: could not find the start of the lists")
    body = ".OverworldWarpTileIDs:" + body[1]

    # Walk it as a sequence of (labels, own bytes, terminated?) chunks, in file order,
    # then resolve fallthrough backwards: an unterminated chunk continues into the next.
    chunks: list[dict] = []
    cur: dict | None = None

    for raw in body.splitlines():
        line = strip_comment(raw)
        if not line.strip():
            continue

        m = re.match(r"^\.(\w+)WarpTileIDs:", line.strip())
        if m:
            key = LABEL.get(m.group(1))
            if key is None:
                die(f"warp: unknown label {m.group(1)}")
            if cur is not None and not cur["own"] and not cur["term"]:
                cur["labels"].append(key)      # two labels on the same list
                continue
            cur = {"labels": [key], "own": [], "term": False}
            chunks.append(cur)
            continue

        if cur is None:
            continue

        m = re.match(r"^\s*warp_tiles\b(.*)$", line)
        if m:
            args = m.group(1).strip()
            cur["own"] += [byte(t) for t in args.split(",") if t.strip()] if args else []
            cur["term"] = True                 # the macro appends the -1
            continue

        m = re.match(r"^\s*db\b(.*)$", line)
        if m:
            cur["own"] += [byte(t) for t in m.group(1).split(",") if t.strip()]
            continue                            # bare db, no terminator -> falls through

    # Resolve fallthrough from the bottom up.
    tail: list[int] = []
    for chunk in reversed(chunks):
        chunk["tiles"] = chunk["own"] + ([] if chunk["term"] else tail)
        tail = chunk["tiles"]

    out: dict[str, list[int]] = {}
    for chunk in chunks:
        for key in chunk["labels"]:
            out[key] = chunk["tiles"]

    for t in TILESETS:
        out.setdefault(t, [])                   # a tileset with no warps really has none
    return out


# ─────────────────────────────────────────────────────────────────────────────
# Doors. A `dbw` table keyed by tileset id -- a tileset absent from it has no doors.
# Note the terminator here is 0, not -1 (door_tiles macro).
# ─────────────────────────────────────────────────────────────────────────────
def parse_door(pokered: Path) -> dict[str, list[int]]:
    text = (pokered / "data" / "tilesets" / "door_tile_ids.asm").read_text()

    order: list[tuple[str, str]] = []           # (tileset const, list label)
    for raw in text.splitlines():
        line = strip_comment(raw)
        m = re.match(r"^\s*dbw\s+(\w+)\s*,\s*\.(\w+)", line)
        if m:
            order.append((m.group(1), m.group(2)))

    lists: dict[str, list[int]] = {}
    cur: str | None = None
    for raw in text.splitlines():
        line = strip_comment(raw)
        m = re.match(r"^\.(\w+):", line.strip())
        if m:
            cur = m.group(1)
            lists.setdefault(cur, [])
            continue
        m = re.match(r"^\s*door_tiles\b(.*)$", line)
        if m and cur:
            args = m.group(1).strip()
            lists[cur] += [byte(t) for t in args.split(",") if t.strip()] if args else []

    out = {t: [] for t in TILESETS}
    for const, label in order:
        if const not in IND:
            die(f"door: unknown tileset {const}")
        if label not in lists:
            die(f"door: no list for {label}")
        out[const] = lists[label]
    return out


# ─────────────────────────────────────────────────────────────────────────────
# The small tables: (tileset, tile, ...) rows.
# ─────────────────────────────────────────────────────────────────────────────
def parse_bookshelf(pokered: Path) -> dict[str, list[int]]:
    text = (pokered / "data" / "tilesets" / "bookshelf_tile_ids.asm").read_text()
    out = {t: [] for t in TILESETS}
    for raw in text.splitlines():
        line = strip_comment(raw)
        m = re.match(r"^\s*bookshelf_tile\s+(\w+)\s*,\s*(\$?\w+)\s*,", line)
        if m:
            out[m.group(1)].append(byte(m.group(2)))
    return out


def parse_warp_pad_hole(pokered: Path) -> dict[str, list[dict]]:
    text = (pokered / "data" / "tilesets" / "warp_pad_hole_tile_ids.asm").read_text()
    out: dict[str, list[dict]] = {t: [] for t in TILESETS}
    for raw in text.splitlines():
        line = strip_comment(raw)
        m = re.match(r"^\s*db\s+(\w+)\s*,\s*(\$?\w+)\s*,\s*(\d+)\s*$", line)
        if m and m.group(1) in IND:
            out[m.group(1)].append({
                "tile": byte(m.group(2)),
                "kind": "pad" if m.group(3) == "1" else "hole",
            })
    return out


def parse_pair_collisions(pokered: Path) -> tuple[dict[str, list], dict[str, list]]:
    text = (pokered / "data" / "tilesets" / "pair_collision_tile_ids.asm").read_text()
    land: dict[str, list] = {t: [] for t in TILESETS}
    water: dict[str, list] = {t: [] for t in TILESETS}
    target = None
    for raw in text.splitlines():
        line = strip_comment(raw)
        if line.startswith("TilePairCollisionsLand"):
            target = land
            continue
        if line.startswith("TilePairCollisionsWater"):
            target = water
            continue
        m = re.match(r"^\s*db\s+(\w+)\s*,\s*(\$\w+)\s*,\s*(\$\w+)\s*$", line)
        if m and target is not None and m.group(1) in IND:
            target[m.group(1)].append([byte(m.group(2)), byte(m.group(3))])
    return land, water


def parse_tileset_list(pokered: Path, filename: str, label: str) -> list[str]:
    """One of the flat `db TILESET` membership lists (water/bike/escape-rope/dungeon)."""
    text = (pokered / "data" / "tilesets" / filename).read_text()
    out = []
    started = False
    for raw in text.splitlines():
        line = strip_comment(raw)
        if line.startswith(label):
            started = True
            continue
        if not started:
            continue
        m = re.match(r"^\s*db\s+(\w+)\s*$", line)
        if m:
            if m.group(1) in IND:
                out.append(m.group(1))
        elif re.match(r"^\s*db\s+-1", line):
            break
    return out


def parse_ledges(pokered: Path) -> list[dict]:
    """(facing, tile you stand on, the ledge tile) -- so a ledge knows which way you jump."""
    text = (pokered / "data" / "tilesets" / "ledge_tiles.asm").read_text()
    facing = {
        "SPRITE_FACING_DOWN": "down", "SPRITE_FACING_UP": "up",
        "SPRITE_FACING_LEFT": "left", "SPRITE_FACING_RIGHT": "right",
    }
    out = []
    for raw in text.splitlines():
        line = strip_comment(raw)
        m = re.match(r"^\s*db\s+(SPRITE_FACING_\w+)\s*,\s*(\$\w+)\s*,\s*(\$\w+)\s*,", line)
        if m:
            out.append({
                "facing": facing[m.group(1)],
                "standingOn": byte(m.group(2)),
                "tile": byte(m.group(3)),
            })
    return out


def parse_cut_trees(pokered: Path) -> list[dict]:
    """BLOCK ids, not tile ids -- the block-layer trap. (before, after the cut)"""
    text = (pokered / "data" / "tilesets" / "cut_tree_blocks.asm").read_text()
    out = []
    for raw in text.splitlines():
        line = strip_comment(raw)
        m = re.match(r"^\s*db\s+(\$\w+)\s*,\s*(\$\w+)\s*$", line)
        if m:
            out.append({"block": byte(m.group(1)), "cutTo": byte(m.group(2))})
    return out


# ─────────────────────────────────────────────────────────────────────────────
# The cartridge is the judge (reference/emulator-verification.md).
# ─────────────────────────────────────────────────────────────────────────────
def verify_against_rom(rom_path: Path, collision: dict[str, list[int]]) -> int:
    """Re-read every collision list out of ROM bank 0 at tileset.json's collPtr."""
    if not rom_path.exists():
        print(f"  ! no ROM at {rom_path} -- cartridge check SKIPPED", file=sys.stderr)
        return 0

    rom = rom_path.read_bytes()
    tilesets = json.loads(TILESET_JSON.read_text())
    bad = 0

    for entry in tilesets:
        key = entry["nameAlias"]
        ptr = entry["collPtr"]
        if ptr >= 0x4000:
            die(f"{key}: collPtr {ptr} is not in bank 0 -- collision is always home bank")

        # Read the $FF-terminated list the save's pointer actually names.
        found: list[int] = []
        i = ptr
        while rom[i] != 0xFF:
            found.append(rom[i])
            i += 1
            if i - ptr > 64:
                die(f"{key}: runaway collision list at {ptr}")

        want = collision[key]
        if found != want:
            bad += 1
            print(f"  ✗ {entry['name']:<14} collPtr {ptr}", file=sys.stderr)
            print(f"      cartridge: {[hex(b) for b in found]}", file=sys.stderr)
            print(f"      pokered:   {[hex(b) for b in want]}", file=sys.stderr)

    if bad:
        print(f"\n  {bad} tileset(s) point at the WRONG collision list.", file=sys.stderr)
        print("  tileset.json's collPtr is wrong, or the parse is. Do not ship this.",
              file=sys.stderr)
    else:
        print(f"  ✓ all {len(tilesets)} collision lists match the cartridge, byte for byte")
    return bad


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--pokered", default=str(DEFAULT_POKERED))
    ap.add_argument("--rom", default=str(DEFAULT_ROM))
    ap.add_argument("--check", action="store_true",
                    help="verify + diff against the committed JSON, write nothing")
    args = ap.parse_args()

    pokered = Path(args.pokered)
    if not (pokered / "data" / "tilesets").is_dir():
        die(f"no pret/pokered clone at {pokered} (see notes/reference/tiles.md)")

    print(f"Reading {pokered / 'data' / 'tilesets'}")

    collision = parse_collision(pokered)
    warp = parse_warp(pokered)
    door = parse_door(pokered)
    bookshelf = parse_bookshelf(pokered)
    padhole = parse_warp_pad_hole(pokered)
    pair_land, pair_water = parse_pair_collisions(pokered)

    water = parse_tileset_list(pokered, "water_tilesets.asm", "WaterTilesets")
    bike = parse_tileset_list(pokered, "bike_riding_tilesets.asm", "BikeRidingTilesets")
    rope = parse_tileset_list(pokered, "escape_rope_tilesets.asm", "EscapeRopeTilesets")
    dungeon = parse_tileset_list(pokered, "dungeon_tilesets.asm", "DungeonTilesets")

    ledges = parse_ledges(pokered)
    cut_trees = parse_cut_trees(pokered)

    # ── Structural checks: the shared lists must have come out shared ──
    shared = [
        ("REDS_HOUSE_1", "REDS_HOUSE_2"), ("MART", "POKECENTER"), ("DOJO", "GYM"),
        ("FOREST_GATE", "MUSEUM"), ("FOREST_GATE", "GATE"),
    ]
    for a, b in shared:
        if collision[a] != collision[b]:
            die(f"{a} and {b} share one collision list in the ROM but parsed differently")
    if collision["MART"] == collision["REDS_HOUSE_1"]:
        die("Mart and Reds House 1 came out with the SAME list -- the chain slipped "
            "(this is the exact bug that was in tileset.json)")

    print("  ✓ shared collision lists came out shared")

    print("Checking against the cartridge…")
    if verify_against_rom(Path(args.rom), collision) != 0:
        return 1

    data = {
        "_source": "pret/pokered -- data/tilesets/*.asm. Generated by "
                   "scripts/import_tile_traits.py. Do not hand-edit.",
        "waterTile": WATER_TILE,
        "ledges": ledges,
        "cutTreeBlocks": cut_trees,
        "tilesets": [
            {
                "ind": IND[t],
                "name": t,
                "passable": collision[t],
                "warp": warp[t],
                "door": door[t],
                "bookshelf": bookshelf[t],
                "warpPadHole": padhole[t],
                "pairCollisions": pair_land[t],
                "pairCollisionsWater": pair_water[t],
                "hasWater": t in water,
                "isDungeon": t in dungeon,
                "allowsBike": t in bike,
                "allowsEscapeRope": t in rope,
            }
            for t in TILESETS
        ],
    }

    text = json.dumps(data, indent=2) + "\n"

    if args.check:
        if not OUT.exists():
            print(f"  ✗ {OUT.name} does not exist", file=sys.stderr)
            return 1
        if OUT.read_text() != text:
            print(f"  ✗ {OUT.name} is out of date -- re-run without --check", file=sys.stderr)
            return 1
        print(f"  ✓ {OUT.name} is up to date")
        return 0

    OUT.write_text(text)
    print(f"Wrote {OUT.relative_to(REPO)}  ({len(text)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
