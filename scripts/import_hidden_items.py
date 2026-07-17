#!/usr/bin/env python3
"""
import_hidden_items.py -- what is actually BURIED at each hidden pickup, imported from pret/pokered.

A hidden item in Gen 1 stores nothing about itself in the save. All the save keeps is a single
"already collected" bit, and the ONLY thing that ties that bit to a place in the world is its
POSITION IN A TABLE:

    HiddenItems -> FindHiddenItemOrCoinsIndex -> bit `n` of wObtainedHiddenItemsFlags,
    where `n` is the row's index in HiddenItemCoords.

So: save bit `i` == row `i` of HiddenItemCoords == a real (map, x, y). That is index identity,
not inference -- which is why these 66 pickups are the cleanest positional data in the game and
need no guessing at all.

`hiddenItems.json` / `hiddenCoins.json` already ship that (map, x, y) -- what they have never had
is WHAT IS BURIED THERE. A row reads "Viridian Forest (1, 18)" and cannot tell you it is a Potion.
This imports that one missing fact.

Per the standing file-format rule this EXTENDS the data we already ship: it adds one field and
NEVER touches an existing one. Run it and diff -- every `map`, `x` and `y` is byte-identical.

Source:  <pokered>/data/events/hidden_item_coords.asm  (the ORDER -- i.e. which save bit)
         <pokered>/data/events/hidden_events.asm       (what is buried -- the item / coin amount)
Output:  projects/db/assets/data/hiddenItems.json  (adds "item": "GREAT BALL")
         projects/db/assets/data/hiddenCoins.json  (adds "coins": 10)

`item` is the item's name EXACTLY as `items.json` spells it (the game's own caps, e.g.
"GREAT BALL"), so it is both a readable name and a valid join key into ItemsDB -- a deep link
(-> artwork, price, description) stays available later without a second import. The name is
never invented here: an item constant that does not resolve against items.json is a FAILURE,
not a fallback.

── The two traps, both of which silently transpose every coordinate ──────────────────────────

  1. `hidden_item` args are (map, X, Y) -- the file's own comment says `; map id, x, y` -- but
     the macro STORES `db \\1, \\3, \\2` = map, Y, X. Read the ARGS, not the storage order.
  2. `hidden_event` args are (X, Y, function, argument); it stores `db \\2` (y) then `db \\1` (x).
     Same trap, opposite file.

Both files agree on ARG order (x, y), which is what makes the join by (map, x, y) exact.

Self-checking, because a silently-wrong join would put the wrong item on the wrong tile:
  1. Counts: exactly 54 items + 12 coins, matching MAX_HIDDEN_ITEMS usage and WorldHidden's
     hiddenItemCount / hiddenCoinCount. A drift here means the ROM changed or the parse broke.
  2. Alignment: every row's (x, y) must equal the committed JSON's (x, y) at the SAME index.
     This is the load-bearing check -- it proves we are writing the item onto the bit that
     actually digs it up.
  3. Resolution: every item constant must resolve to a real items.json entry.
  4. Join: every coord row must find exactly one HiddenItems/HiddenCoins event at its (map,x,y).

Usage:
    python scripts/import_hidden_items.py [--pokered <path>] [--check]
    --check   parse + verify + diff against the committed JSON, write nothing. Exit 1 on drift.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

for _s in (sys.stdout, sys.stderr):
    try:
        _s.reconfigure(encoding="utf-8")
    except AttributeError:
        pass

REPO = Path(__file__).resolve().parent.parent
DEFAULT_POKERED = Path.home() / "Documents" / "projects" / "pokered"
DATA = REPO / "projects" / "db" / "assets" / "data"
ITEMS_JSON = DATA / "items.json"
HIDDEN_ITEMS_JSON = DATA / "hiddenItems.json"
HIDDEN_COINS_JSON = DATA / "hiddenCoins.json"

# What the ROM says there are. Pinned rather than discovered: WorldHidden's hiddenItemCount /
# hiddenCoinCount are compile-time constants sized to exactly these, so a mismatch is not a
# cosmetic drift -- it would mean the save model is now the wrong shape.
EXPECT_ITEMS = 54
EXPECT_COINS = 12

# `hidden_item VIRIDIAN_FOREST,  1,  18`   -> (map, x, y).  ARGS, not storage order (trap #1).
RE_HIDDEN_ITEM = re.compile(
    r"^\s*hidden_item\s+([A-Z0-9_]+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)", re.MULTILINE
)
# `hidden_events_for VIRIDIAN_FOREST`
RE_EVENTS_FOR = re.compile(r"^\s*hidden_events_for\s+([A-Z0-9_]+)", re.MULTILINE)
# `hidden_event  1, 18, HiddenItems, POTION`  -> (x, y, function, argument).  Trap #2.
RE_HIDDEN_EVENT = re.compile(
    r"^\s*hidden_event\s+(-?\d+)\s*,\s*(-?\d+)\s*,\s*([A-Za-z0-9_]+)\s*,\s*([^;\n]+)", re.MULTILINE
)


def read(p: Path) -> str:
    return p.read_text(encoding="utf-8")


def parse_coords(text: str) -> list[tuple[str, int, int]]:
    """HiddenItemCoords, IN ORDER. The index IS the save bit -- so order is the whole point."""
    return [(m.group(1), int(m.group(2)), int(m.group(3))) for m in RE_HIDDEN_ITEM.finditer(text)]


def parse_events(text: str) -> dict[tuple[str, int, int], tuple[str, str]]:
    """(map, x, y) -> (function, argument), for the HiddenItems/HiddenCoins events only.

    The file is a run of `hidden_events_for <MAP>` blocks, so the current map is whichever
    header we most recently passed. That block structure is what makes the join exact: (x, y)
    alone collides across maps, (map, x, y) does not.
    """
    out: dict[tuple[str, int, int], tuple[str, str]] = {}
    # Walk headers and events together in source order.
    marks = [(m.start(), "map", m.group(1)) for m in RE_EVENTS_FOR.finditer(text)]
    marks += [
        (m.start(), "event", (int(m.group(1)), int(m.group(2)), m.group(3), m.group(4).strip()))
        for m in RE_HIDDEN_EVENT.finditer(text)
    ]
    marks.sort(key=lambda t: t[0])

    cur: str | None = None
    for _pos, kind, val in marks:
        if kind == "map":
            cur = val
            continue
        x, y, fn, arg = val
        if fn not in ("HiddenItems", "HiddenCoins"):
            continue  # bench guys, PCs, trash cans -- other hidden-event kinds, not pickups
        if cur is None:
            continue
        out[(cur, x, y)] = (fn, arg)
    return out


def coin_amount(arg: str) -> int:
    """`COIN + 10` -> 10. The argument is an item id offset from COIN; the offset IS the amount."""
    m = re.match(r"^COIN\s*\+\s*(\d+)$", arg.strip())
    if m is None:
        raise ValueError(f"unparsed coin argument: {arg!r}")
    return int(m.group(1))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--pokered", type=Path, default=DEFAULT_POKERED)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    coords_asm = args.pokered / "data" / "events" / "hidden_item_coords.asm"
    events_asm = args.pokered / "data" / "events" / "hidden_events.asm"
    for p in (coords_asm, events_asm):
        if not p.is_file():
            print(f"FAIL: not found: {p}\n(pass --pokered <path to your pokered clone>)", file=sys.stderr)
            return 1

    coords = parse_coords(read(coords_asm))
    events = parse_events(read(events_asm))

    items = json.loads(read(ITEMS_JSON))
    # items.json spells the game's own caps in `name` ("GREAT BALL"); the pret constant is
    # GREAT_BALL. Underscores -> spaces is the whole mapping, and check #3 proves it for all 54
    # rather than trusting it.
    item_names = {e["name"] for e in items if "name" in e}

    hidden_items = json.loads(read(HIDDEN_ITEMS_JSON))
    hidden_coins = json.loads(read(HIDDEN_COINS_JSON))

    problems: list[str] = []

    # ── Check 1: counts ──────────────────────────────────────────────────────────────────
    if len(coords) != EXPECT_ITEMS:
        problems.append(f"HiddenItemCoords has {len(coords)} rows, expected {EXPECT_ITEMS}")
    if len(hidden_items) != EXPECT_ITEMS:
        problems.append(f"hiddenItems.json has {len(hidden_items)} rows, expected {EXPECT_ITEMS}")
    if len(hidden_coins) != EXPECT_COINS:
        problems.append(f"hiddenCoins.json has {len(hidden_coins)} rows, expected {EXPECT_COINS}")

    # ── Build the item rows ──────────────────────────────────────────────────────────────
    # coords[] is item-only and in bit order, so index i here IS save bit i.
    resolved_items: list[str] = []
    for i, (map_const, x, y) in enumerate(coords):
        hit = events.get((map_const, x, y))
        if hit is None:
            problems.append(f"item bit {i}: no hidden_event at {map_const} ({x}, {y})")
            resolved_items.append("")
            continue
        fn, arg = hit
        if fn != "HiddenItems":
            problems.append(f"item bit {i}: {map_const} ({x},{y}) is {fn}, not HiddenItems")
            resolved_items.append("")
            continue
        name = arg.replace("_", " ").strip()
        if name not in item_names:
            problems.append(f"item bit {i}: '{arg}' -> '{name}' is not in items.json")
        resolved_items.append(name)

        # ── Check 2: alignment. The load-bearing one. ────────────────────────────────────
        if i < len(hidden_items):
            cur = hidden_items[i]
            if cur.get("x") != x or cur.get("y") != y:
                problems.append(
                    f"item bit {i}: coords disagree -- ROM says ({x}, {y}), "
                    f"hiddenItems.json says ({cur.get('x')}, {cur.get('y')}) on '{cur.get('map')}'"
                )

    # ── Build the coin rows ──────────────────────────────────────────────────────────────
    # Coins have no coord table of their own: HiddenCoins events ARE the table, in source order.
    coin_events = [(k, v) for k, v in events.items() if v[0] == "HiddenCoins"]
    # events{} is insertion-ordered by source position (marks were sorted), so this order is
    # the ROM's order == bit order.
    resolved_coins: list[int] = []
    for i, ((map_const, x, y), (_fn, arg)) in enumerate(coin_events):
        try:
            amount = coin_amount(arg)
        except ValueError as e:
            problems.append(f"coin bit {i}: {e}")
            resolved_coins.append(0)
            continue
        resolved_coins.append(amount)
        if i < len(hidden_coins):
            cur = hidden_coins[i]
            if cur.get("x") != x or cur.get("y") != y:
                problems.append(
                    f"coin bit {i}: coords disagree -- ROM says ({x}, {y}), "
                    f"hiddenCoins.json says ({cur.get('x')}, {cur.get('y')}) on '{cur.get('map')}'"
                )

    if len(resolved_coins) != EXPECT_COINS:
        problems.append(f"parsed {len(resolved_coins)} HiddenCoins events, expected {EXPECT_COINS}")

    print(f"hidden items parsed : {len(coords)}")
    print(f"hidden coins parsed : {len(resolved_coins)}")
    print(f"distinct items      : {len(set(n for n in resolved_items if n))}")

    if problems:
        print(f"\n!! {len(problems)} problem(s):")
        for p in problems[:40]:
            print(f"  {p}")
        print("\nFAIL: every shipped pickup must resolve and align. Fix the parser.", file=sys.stderr)
        return 1

    # ── Merge additively. Existing fields are never touched. ─────────────────────────────
    for i, entry in enumerate(hidden_items):
        entry["item"] = resolved_items[i]
    for i, entry in enumerate(hidden_coins):
        entry["coins"] = resolved_coins[i]

    if args.check:
        drift = 0
        for path, built in ((HIDDEN_ITEMS_JSON, hidden_items), (HIDDEN_COINS_JSON, hidden_coins)):
            if json.loads(read(path)) != built:
                drift += 1
                print(f"--check: {path.name} would change.")
        if drift:
            print("\n--check: run without --check to write.")
            return 1
        print("\n--check: hidden item/coin data is up to date.")
        return 0

    # indent=4 matches the committed formatting -- so the diff is the new field and nothing else.
    for path, built in ((HIDDEN_ITEMS_JSON, hidden_items), (HIDDEN_COINS_JSON, hidden_coins)):
        path.write_text(json.dumps(built, indent=4, ensure_ascii=False) + "\n", encoding="utf-8")
        print(f"wrote {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
