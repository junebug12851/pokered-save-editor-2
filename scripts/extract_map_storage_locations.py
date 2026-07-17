#!/usr/bin/env python3
"""Extract every piece of PERSISTENT SAVE STORAGE that has a place on a map.

The deep dive behind map-screen Phase 16f (Fairy Fox, 2026-07-17: "figure out the
coordinates and boxes and stuff for the scripts, event flags, filter flags, and more
stuff ill add later").

WHAT HAS A LOCATION, AND WHAT DOES NOT -- the whole point of the dive:

  * FILTER FLAGS (wMissableObjectFlags, file 0x2852) -- an object on a map. Its tile
    is in maps.json and its bit is that object's `missable`. Exact, already shipped,
    already drawn (Phase 16c/d). Not re-extracted here.

  * HIDDEN ITEMS  (wObtainedHiddenItemsFlags, file 0x299C, 54 used of 112 allocated)
    HIDDEN COINS  (wObtainedHiddenCoinsFlags, file 0x29AA, 12 used of  16 allocated)
    -- EXACT coordinates, and the cleanest data in the whole dive: the flag's bit
    index IS its row index in the coord table, so row i <-> bit i, no inference at
    all. 66 boxes we can draw today.

  * EVENT FLAGS (wEventFlags, file 0x29F3) -- NO location. They belong to the code.
    They reach the map only THROUGH a script, so they are extracted per-routine here
    and attached to whatever that routine has a location for.

  * SCRIPTS -- a location when they test the player's coords, in two shapes:
      - `dbmapcoord` tables read by ArePlayerCoordsInArray  -> real TILES
      - raw `ld a, [wYCoord]` / `[wXCoord]` + `cp`          -> a ROW/COLUMN/RANGE
    Leadership: "if its a coord range test then put a box around the whole range" --
    so a script box carries an EXTENT (w/h in tiles), not a fixed 16x16.

THE RULE, and it is not negotiable: extract only what the source PROVES. A spot whose
location cannot be established gets NO box -- never a guessed one. Static
co-location is a lead, never evidence (the Route 22 false positive; see
notes/decisions/rejected.md).

Output (git-ignored): tmp/event-flags/storage_locations.json
"""

import json
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PRET = os.path.join(os.path.expanduser("~"), "Documents", "projects", "pokered")
OUT_DIR = os.path.join(REPO, "tmp", "event-flags")
OUT = os.path.join(OUT_DIR, "storage_locations.json")

# WRAM -> save-file offset. Verified live on the console (reference/event-flags.md).
WRAM_TO_FILE = 0xAD54
HIDDEN_ITEMS_FILE = 0x299C
HIDDEN_COINS_FILE = 0x29AA


def _map_key(const: str) -> str:
    """MAP_CONSTANT -> the CamelCase name pret uses for the map's script file."""
    return "".join(p.capitalize() for p in const.split("_"))


# ── Hidden items + coins: the exact ones ─────────────────────────────────────────────

def parse_hidden(path: str, macro: str):
    """`hidden_item VIRIDIAN_FOREST, 1, 18` -> {map, x, y}.

    ⚠️ The ARGS are (map, x, y) -- the comment in the file says so -- but the macro
    stores `db \\1, \\3, \\2`, i.e. map, y, x. Read the ARGS, not the storage order,
    or every coordinate comes out transposed.

    The row's INDEX is the save bit's index. That is the whole link, and it is exact.
    """
    out = []
    if not os.path.exists(path):
        return out
    with open(path, encoding="utf-8") as fh:
        for line in fh:
            m = re.match(r"\s*%s\s+([A-Z0-9_]+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)" % macro, line)
            if m:
                out.append({
                    "ind": len(out),          # == the bit index in the save's flag array
                    "mapConst": m.group(1),
                    "map": _map_key(m.group(1)),
                    "x": int(m.group(2)),
                    "y": int(m.group(3)),
                })
    return out


# ── Scripts: the located ones ────────────────────────────────────────────────────────

LABEL_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_.]*):")
# ⚠️ Tables are usually LOCAL labels -- `.PlayerCoordsArray:` -- and a leading dot is
# not a letter. Missing this dropped 31 of 53 located script files on the first run
# (Route 24, every Silph Co floor, the Seafoam/Victory Road set): the routine was
# found, the `ld hl, .Foo` was found, and the table it pointed at was invisible.
# ⚠️ The colon is OPTIONAL in RGBDS. `.Route22RivalBattleCoords` (no colon) is a label
# just as much as `.SwitchCoords:` is, and requiring the colon silently lost Route 22's
# rival-battle coords -- on the very map whose flags we already had a false positive on.
# Anchored at column 0, because instructions are always indented.
ANY_LABEL_RE = re.compile(r"^(\.?[A-Za-z_][A-Za-z0-9_.]*):?\s*(?:;.*)?$")
COORD_RE = re.compile(r"^\s*dbmapcoord\s+(\d+)\s*,\s*(\d+)")
ARRAY_RE = re.compile(r"ld\s+hl\s*,\s*(\.?[A-Za-z_][A-Za-z0-9_.]*)\s*\n\s*call\s+ArePlayerCoordsInArray")
# The card-key doors: a per-map `.GateCoordinates` table read by the map's own
# SetCardKeyDoor*Script. ⚠️ NOT the same thing as data/events/card_key_coords.asm,
# which pret's own comment says is UNUSED ("probably supposed to be door locations in
# Silph Co., but they are unused"). These per-map ones are live.
CARDKEY_RE = re.compile(r"ld\s+hl\s*,\s*(\.?[A-Za-z_][A-Za-z0-9_.]*)\s*\n\s*call\s+\w*SetCardKeyDoor\w*")
SETEV_RE = re.compile(r"\b(SetEvent|ResetEvent)\s+(EVENT_[A-Z0-9_]+)")
CHKEV_RE = re.compile(r"\bCheckEvent\s+(EVENT_[A-Z0-9_]+)")
TOGGLE_RE = re.compile(r"ld\s+a\s*,\s*(TOGGLE_[A-Z0-9_]+)")
SHOWHIDE_RE = re.compile(r"predef\s+(Show|Hide)Object")
# `ld a, [wYCoord]` ... `cp N` -- the raw form. The compare may be a few lines later.
RAWY_RE = re.compile(r"ld\s+a\s*,\s*\[wYCoord\][\s\S]{0,120}?cp\s+(\d+)")
RAWX_RE = re.compile(r"ld\s+a\s*,\s*\[wXCoord\][\s\S]{0,120}?cp\s+(\d+)")


def split_routines(text: str):
    """Split a script file into (label, body) routines. The routine is the unit of
    truth here: a flag written in the same ROUTINE as a coord test is that trigger's
    flag. Proximity across a label boundary proves nothing."""
    lines = text.split("\n")
    routines = []
    cur, body = None, []
    for ln in lines:
        m = LABEL_RE.match(ln)
        if m and not ln.startswith((" ", "\t")):
            if cur is not None:
                routines.append((cur, "\n".join(body)))
            cur, body = m.group(1), []
        else:
            body.append(ln)
    if cur is not None:
        routines.append((cur, "\n".join(body)))
    return routines


def coord_tables(text: str):
    """label -> [(x, y), ...] for every dbmapcoord table in the file.

    Scans EVERY label, global and local, because the tables are nearly always local
    (`.PlayerCoordsArray:`). Keyed by the label exactly as written, so `ld hl, .Foo`
    resolves against `.Foo:`. Local labels are file-unique enough in practice for
    coord tables -- and where they are not, the worst case is a table found twice with
    identical contents, not a wrong coordinate.
    """
    tables = {}
    cur = None
    for ln in text.split("\n"):
        m = ANY_LABEL_RE.match(ln)
        if m:
            cur = m.group(1)
            continue
        c = COORD_RE.match(ln)
        if c and cur is not None:
            # `dbmapcoord y, x` -- the macro's arg order. Store (x, y).
            tables.setdefault(cur, []).append((int(c.group(2)), int(c.group(1))))
    return tables


def parse_script_file(path: str):
    with open(path, encoding="utf-8") as fh:
        text = fh.read()

    tables = coord_tables(text)
    spots = []

    for label, body in split_routines(text):
        writes = sorted({m.group(2) for m in SETEV_RE.finditer(body)})
        checks = sorted({m.group(1) for m in CHKEV_RE.finditer(body)})
        toggles = sorted({m.group(1) for m in TOGGLE_RE.finditer(body)}) if SHOWHIDE_RE.search(body) else []

        # 1. TILES: this routine reads a dbmapcoord table.
        for m in ARRAY_RE.finditer(body):
            tbl = m.group(1)
            if tbl in tables:
                for (x, y) in tables[tbl]:
                    spots.append({
                        "kind": "scriptTile", "routine": label, "table": tbl,
                        "x": x, "y": y, "w": 1, "h": 1,
                        "setsEvents": writes, "checksEvents": checks, "togglesFilters": toggles,
                    })

        # 1b. CARD-KEY DOORS: the same shape, a different reader, and its own kind --
        #     a door you open with the Card Key is not a script trigger you walk onto.
        for m in CARDKEY_RE.finditer(body):
            tbl = m.group(1)
            if tbl in tables:
                for (x, y) in tables[tbl]:
                    spots.append({
                        "kind": "cardKeyDoor", "routine": label, "table": tbl,
                        "x": x, "y": y, "w": 1, "h": 1,
                        "setsEvents": writes, "checksEvents": checks, "togglesFilters": toggles,
                    })

        # 2. RANGES: raw coord compare -> a whole row/column. Leadership: box the range.
        #    A `cp N` on wYCoord means "row N, all of it" -- the width is the map's,
        #    which this extractor does not know, so it emits h=1 + `span: "row"` and
        #    lets the UI stretch it. Same for a column.
        for m in RAWY_RE.finditer(body):
            spots.append({
                "kind": "scriptRow", "routine": label, "y": int(m.group(1)),
                "span": "row", "setsEvents": writes, "checksEvents": checks,
                "togglesFilters": toggles,
            })
        for m in RAWX_RE.finditer(body):
            spots.append({
                "kind": "scriptCol", "routine": label, "x": int(m.group(1)),
                "span": "col", "setsEvents": writes, "checksEvents": checks,
                "togglesFilters": toggles,
            })

    return spots


def main() -> int:
    if not os.path.isdir(PRET):
        print("pret/pokered clone not found at %s" % PRET, file=sys.stderr)
        return 1

    os.makedirs(OUT_DIR, exist_ok=True)

    items = parse_hidden(os.path.join(PRET, "data", "events", "hidden_item_coords.asm"), "hidden_item")
    coins = parse_hidden(os.path.join(PRET, "data", "events", "hidden_coins.asm"), "hidden_coin")

    scripts = {}
    sdir = os.path.join(PRET, "scripts")
    for name in sorted(os.listdir(sdir)):
        if not name.endswith(".asm"):
            continue
        spots = parse_script_file(os.path.join(sdir, name))
        if spots:
            scripts[name[:-4]] = spots

    data = {
        "hiddenItems": {"fileOffset": HIDDEN_ITEMS_FILE, "entries": items},
        "hiddenCoins": {"fileOffset": HIDDEN_COINS_FILE, "entries": coins},
        "scripts": scripts,
    }
    with open(OUT, "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=1)

    tiles = sum(1 for v in scripts.values() for s in v if s["kind"] == "scriptTile")
    rows = sum(1 for v in scripts.values() for s in v if s["kind"] == "scriptRow")
    cols = sum(1 for v in scripts.values() for s in v if s["kind"] == "scriptCol")
    withev = sum(1 for v in scripts.values() for s in v if s["setsEvents"])

    print("hidden items (exact, bit i == row i): %d" % len(items))
    print("hidden coins (exact, bit i == row i): %d" % len(coins))
    print("script files with a location:         %d" % len(scripts))
    print("  script TILES  (dbmapcoord):         %d" % tiles)
    print("  script ROWS   (raw wYCoord cp):     %d" % rows)
    print("  script COLS   (raw wXCoord cp):     %d" % cols)
    print("  located spots that WRITE events:    %d" % withev)
    print("out -> %s" % OUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
