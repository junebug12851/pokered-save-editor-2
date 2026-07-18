r"""ADD map + coordinates to the ten in-game trades in projects/db/assets/data/trades.json.

⚠️ **ADDITIVE ONLY (leadership, 2026-07-17).** trades.json already exists and already ships five
fields per row -- `give`, `get`, `textId` (an INT: the dialog set, 0=casual/1=evolution/2=happy),
`nickname`, and `unused` (only where true). This importer **keeps every one of them byte-for-byte**
and only APPENDS the location data the map screen needs:

    ind     -- the save bit in wCompletedInGameTradeFlags (== store index)
    const   -- pret's TRADE_FOR_* constant
    mapId   -- the map the trader stands on (omitted for the unused trade -- it has none)
    x, y    -- walk-grid coords (spawn tile if the trader walks)
    trader  -- the trader's CLASS name (they have no personal name; see the note)
    walks   -- whether the trader wanders (so x/y is a spawn tile only)

It refuses to write unless it can prove **0 existing fields changed** across all 10 rows,
semantically -- the import_hidden_items.py standard.

⚠️ It does NOT touch `textId`. That field is the dialog set and its name is arguably wrong (it is
not a text id), but renaming it is a data+code change trades.cpp depends on, and that is leadership's
call, not this importer's.

Source of the location data (the standing file-format rule -- read pret's, join it):
  1. data/events/trades.asm          -- the 10 rows, in bit order (give/get/dialogset/nickname)
  2. constants/script_constants.asm  -- TRADE_FOR_* : the bit index, and which is `; unused`
  3. scripts/*.asm                   -- `ld a, TRADE_FOR_X` : which map's text names it
  4. data/maps/objects/<Map>.asm     -- the object_event with that text id : its x/y + sprite class

The join is (map id, text id) -> trade, mechanical. Nine resolve; TRADE_FOR_CHIKUCHIKU is `; unused`,
no script names it, so it gets NO map and NO coords -> the General page.

Self-validating; --check verifies the shipped file matches without writing.

    python scripts/import_trades.py [--check]
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
PRET = Path.home() / "Documents" / "projects" / "pokered"
OUT = REPO / "projects" / "db" / "assets" / "data" / "trades.json"
MAPS_JSON = REPO / "projects" / "db" / "assets" / "data" / "maps.json"

NUM_NPC_TRADES = 10
UNUSED_TRADE_NICK = "CHIKUCHIKU"

# The trader's CLASS name (the sprite constant). Traders have no personal name; leadership's rule is
# "class name = the species, nickname if they had one" -- and they haven't. So the class is the whole
# identity, the same way a wild Pokemon has only a species.
SPRITE_CLASS = {
    "SPRITE_YOUNGSTER": "Youngster",
    "SPRITE_GAMEBOY_KID": "Gameboy Kid",
    "SPRITE_SCIENTIST": "Scientist",
    "SPRITE_LITTLE_GIRL": "Little Girl",
    "SPRITE_GAMBLER": "Gambler",
    "SPRITE_GRAMPS": "Gramps",
    "SPRITE_BEAUTY": "Beauty",
}


def die(msg: str) -> "NoReturn":  # type: ignore[valid-type]
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def parse_trade_constants() -> list[str]:
    """constants/script_constants.asm -> the TRADE_FOR_* names, in bit order."""
    src = (PRET / "constants" / "script_constants.asm").read_text(encoding="utf-8")
    block = re.search(r"; TradeMons indexes.*?DEF NUM_NPC_TRADES", src, re.S)
    if not block:
        die("could not find the TRADE_FOR_* const block in script_constants.asm")
    names = re.findall(r"^\s*const\s+(TRADE_FOR_\w+)", block.group(0), re.M)
    if len(names) != NUM_NPC_TRADES:
        die(f"parsed {len(names)} TRADE_FOR_* constants, expected {NUM_NPC_TRADES}")
    return names


def find_trade_sites() -> dict[str, tuple[str, str]]:
    r"""scripts/*.asm -> {TRADE_FOR_X: (ScriptFileStem, TEXT_<MAP>_<WHO>)}.

    Two silent-failure traps, both live-caught:

    1. Route11Gate2F writes `xor a ; TRADE_FOR_TERRY`, not `ld a, TRADE_FOR_TERRY` -- trade 0 is
       zero, so the constant hides in a COMMENT. Both forms are matched.

    2. The gap between a label and its trade must NOT cross another label. `(?:.*\n)*?` will happily
       scan past the next label; in Cinnabar Lab Fossil Room (two Scientists, only the 2nd trades)
       that filed SAILOR at (5,2) instead of (7,6), and nine trades STILL resolved with every total
       looking right. The `(?!^\w+:)` guard makes a block unable to borrow the next block's body.
    """
    sites: dict[str, tuple[str, str]] = {}
    for f in sorted((PRET / "scripts").glob("*.asm")):
        src = f.read_text(encoding="utf-8")
        for m in re.finditer(
                r"^(\w+):[ \t]*\n[ \t]*text_asm[ \t]*\n(?:(?!^\w+:)[^\n]*\n)*?"
                r"[ \t]*(?:ld[ \t]+a[ \t]*,[ \t]*(TRADE_FOR_\w+)"
                r"|xor[ \t]+a[ \t]*;[ \t]*(TRADE_FOR_\w+))", src, re.M):
            label, viaLd, viaXor = m.groups()
            trade = viaLd or viaXor
            tm = re.search(rf"dw_const\s+{re.escape(label)}\s*,\s*(TEXT_\w+)", src)
            if not tm:
                die(f"{f.name}: {label} names {trade} but has no dw_const row")
            if trade in sites:
                die(f"{trade} is named by two scripts: {sites[trade][0]} and {f.stem}")
            sites[trade] = (f.stem, tm.group(1))
    return sites


def parse_objects(stem: str) -> tuple[list[dict], str]:
    """data/maps/objects/<stem>.asm -> its object_events + the map's const name."""
    f = PRET / "data" / "maps" / "objects" / f"{stem}.asm"
    src = f.read_text(encoding="utf-8")
    mapConst = re.search(r"def_warps_to\s+(\w+)", src)
    if not mapConst:
        die(f"{f.name}: no def_warps_to -- cannot identify the map")
    objs = []
    for m in re.finditer(
            r"^\s*object_event\s+(\d+)\s*,\s*(\d+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,\s*(TEXT_\w+)",
            src, re.M):
        x, y, sprite, move, face, text = m.groups()
        objs.append({"x": int(x), "y": int(y), "sprite": sprite, "move": move,
                     "face": face, "text": text})
    return objs, mapConst.group(1)


def map_ids() -> dict[str, int]:
    """constants/map_constants.asm -> {MAP_CONST: id}, positional in const_def order."""
    src = (PRET / "constants" / "map_constants.asm").read_text(encoding="utf-8")
    block = re.search(r"^\tconst_def\n(.*?)^\tconst_def", src, re.S | re.M)
    body = block.group(1) if block else src
    ids: dict[str, int] = {}
    n = 0
    for m in re.finditer(r"^\tmap_const\s+(\w+)\s*,", body, re.M):
        ids[m.group(1)] = n
        n += 1
    if not ids:
        die("parsed no map_const rows")
    return ids


def location_for(bit: int, const: str) -> dict:
    """The ADDED fields for one trade -- ind/const always; map/x/y/trader/walks when located."""
    add = {"ind": bit, "const": const}
    site = find_trade_sites().get(const)
    if site is None:
        return add  # unused: no map, no coords -> General page

    stem, textConst = site
    objs, mapConst = parse_objects(stem)
    match = [o for o in objs if o["text"] == textConst]
    if len(match) != 1:
        die(f"{const}: {len(match)} objects carry {textConst} in {stem} -- expected exactly 1")
    obj = match[0]

    ids = map_ids()
    if mapConst not in ids:
        die(f"{const}: map const {mapConst} is not in map_constants.asm")
    if obj["sprite"] not in SPRITE_CLASS:
        die(f"{const}: sprite {obj['sprite']} has no class name -- add it to SPRITE_CLASS")

    add.update(mapId=ids[mapConst], x=obj["x"], y=obj["y"],
               trader=SPRITE_CLASS[obj["sprite"]], walks=obj["move"] == "WALK")
    return add


def build() -> list[dict]:
    """The existing rows, each with the location fields appended and NOTHING existing touched."""
    existing = json.loads(OUT.read_text(encoding="utf-8"))
    if len(existing) != NUM_NPC_TRADES:
        die(f"trades.json has {len(existing)} rows, expected {NUM_NPC_TRADES}")

    consts = parse_trade_constants()

    out = []
    for bit, row in enumerate(existing):
        # pret names the constant after the nickname (TRADE_FOR_TERRY <-> "TERRY"). If the shipped
        # file's row order ever disagreed with pret's bit order, the join would be silently wrong --
        # so it is asserted, not assumed.
        nick = row.get("nickname", "")
        suffix = consts[bit][len("TRADE_FOR_"):]
        if suffix != nick:
            die(f"bit {bit}: pret constant {consts[bit]} != trades.json nickname {nick!r} -- "
                f"the shipped row order does not match the save's bit order")

        # Start from the EXISTING dict (verbatim), then append. dict insertion order is preserved,
        # so every original key keeps its place and value; new keys land after them.
        merged = dict(row)
        add = location_for(bit, consts[bit])

        # The unused-trade check, from the data itself.
        if row.get("unused", False):
            if nick != UNUSED_TRADE_NICK:
                die(f"row marked unused is {nick!r}, expected {UNUSED_TRADE_NICK}")
            if "mapId" in add:
                die(f"{nick} is marked unused but resolved a map -- the script scan is confused")
        else:
            if "mapId" not in add:
                die(f"{nick} is a USED trade but resolved no map -- the script scan is incomplete")

        merged.update(add)
        out.append(merged)
    return out, existing


def validate(rows: list[dict], existing: list[dict]) -> None:
    # THE additive guarantee: every original key/value is byte-identical in the output.
    for i, (old, new) in enumerate(zip(existing, rows)):
        for k, v in old.items():
            if k not in new:
                die(f"row {i}: existing field {k!r} was DROPPED")
            if new[k] != v:
                die(f"row {i}: existing field {k!r} CHANGED {v!r} -> {new[k]!r}")

    located = [r for r in rows if "mapId" in r]
    unused = [r for r in rows if r.get("unused", False)]
    if len(located) != 9:
        die(f"{len(located)} located, expected 9")
    if len(unused) != 1:
        die(f"{len(unused)} unused, expected 1")

    maps = {m["ind"] for m in json.loads(MAPS_JSON.read_text(encoding="utf-8"))}
    for r in located:
        if r["mapId"] not in maps:
            die(f"{r['const']}: map id {r['mapId']} is not in maps.json")

    if [r["ind"] for r in rows] != list(range(NUM_NPC_TRADES)):
        die("trade inds are not a dense 0..9 -- the save bit is the index")

    print(f"  OK  {len(rows)} trades: {len(located)} located, {len(unused)} unused; "
          f"0 existing fields changed")
    for r in rows:
        where = (f"{r['mapId']} at ({r['x']},{r['y']}) -- {r['trader']}"
                 + ("  [WALKS]" if r.get("walks") else "")) if "mapId" in r else "unused -> General"
        print(f"      bit {r['ind']}  {r['nickname']:<11} textId={r['textId']}  {where}")


def emit(rows: list[dict]) -> str:
    # Match the shipped file's 2-space indent so the diff is purely additive lines.
    return json.dumps(rows, indent=2, ensure_ascii=False) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    if not PRET.exists():
        die(f"pret/pokered clone not found at {PRET}")
    if not OUT.exists():
        die(f"{OUT.name} does not exist -- this importer EXTENDS it, it does not create it")

    rows, existing = build()
    validate(rows, existing)
    text = emit(rows)

    if args.check:
        if OUT.read_text(encoding="utf-8") != text:
            die(f"{OUT.name} is STALE -- re-run without --check")
        print(f"  OK  {OUT.name} is up to date")
        return 0

    OUT.write_text(text, encoding="utf-8")
    print(f"  wrote {OUT.relative_to(REPO)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
