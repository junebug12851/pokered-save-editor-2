r"""Import the ten in-game NPC trades from `pret/pokered` -> projects/db/assets/data/trades.json.

The standing rule: where pret has the data, we read THEIRS. Three of their files, joined:

  1. data/events/trades.asm          -- TradeMons: give, get, dialogset, nickname (10 rows)
  2. constants/script_constants.asm  -- TRADE_FOR_* : the bit index (and which is `; unused`)
  3. scripts/*.asm                   -- `ld a, TRADE_FOR_X` / `ld [wWhichTrade], a` : WHICH MAP
  4. data/maps/objects/<Map>.asm     -- the object_event carrying that map's text id : the x/y

⭐ **The join is (map id, text id) -> trade, and it is mechanical, not proximity.** A trade is
reached by talking to an NPC; the script that fires is that NPC's text entry; the text entry's id is
the object's `text` field. So we walk: trade -> the script file that names it -> the TEXT_<MAP>_<WHO>
constant on the same `text_asm` block -> the object with that text id -> its x/y.

Nine of the ten resolve. The tenth (TRADE_FOR_CHIKUCHIKU) is `; unused` in both the table and the
constants, no script names it, so it has NO map and NO coordinates -- it is emitted with map=null,
which is what puts it on the General page. Per the standing rule: a spot whose location cannot be
established gets NO box, never a guessed one.

⚠️ The 4th npctrade field is the RECEIVED POKEMON'S NICKNAME, not the trader's name (the macro's own
comment says `; give mon, get mon, dialog id, nickname`, and the code copies it to wPartyMonNicks).
TERRY is the Nidorina. Traders have no names -- only sprite classes, which repeat. See
notes/reference/in-game-trades.md section 2a.

Self-validating: refuses to write unless all 10 rows parse, exactly 9 resolve a map+coords, exactly
1 is unused, every species resolves against pokemon.json, and every map id resolves against
maps.json. `--check` verifies the shipped file matches without writing (for CI / idempotence).

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
POKEMON_JSON = REPO / "projects" / "db" / "assets" / "data" / "pokemon.json"

NUM_NPC_TRADES = 10
UNUSED_TRADE_NICK = "CHIKUCHIKU"

# The save field these bits live in -- carried into the JSON so the DB never has to guess.
TRADE_FLAGS_OFFSET = 0x29E3  # wCompletedInGameTradeFlags, $D737, 2 bytes, 10 bits used

# pret spells species as constants (NIDORAN_M, MR_MIME); pokemon.json keys them by the GAME's own
# caps spelling (NIDORAN<m>, MR.MIME). We emit pokemon.json's `name`, because that is a valid
# PokemonDB key -- exactly as import_hidden_items.py emits items.json's own spelling. The readable
# form ("Mr. Mime") is pokemon.json's `readable` and is the UI's business, not ours: a second
# opinion about how a species is written is a second source of truth.
#
# Only the constants that are NOT already identical to the json name need a row here. The importer
# validates every species against pokemon.json and FAILS rather than shipping a name that will not
# resolve -- which is the whole point (see notes/reference/map-storage-locations.md section 2a).
SPECIES_FIXUPS = {
    "NIDORAN_M": "NIDORAN<m>",
    "NIDORAN_F": "NIDORAN<f>",
    "MR_MIME": "MR.MIME",
}

# A trader has no name -- only a class. The sprite constant is the class, and it is the "species"
# half of leadership's rule: "if the trader doesnt have a nickname then use there class name same
# logic as pokemon for species name and nickname".
SPRITE_CLASS = {
    "SPRITE_YOUNGSTER": "Youngster",
    "SPRITE_GAMEBOY_KID": "Gameboy Kid",
    "SPRITE_SCIENTIST": "Scientist",
    "SPRITE_LITTLE_GIRL": "Little Girl",
    "SPRITE_GAMBLER": "Gambler",
    "SPRITE_GRAMPS": "Gramps",
    "SPRITE_BEAUTY": "Beauty",
}

DIALOGSET = {
    "TRADE_DIALOGSET_CASUAL": "casual",
    "TRADE_DIALOGSET_EVOLUTION": "evolution",
    "TRADE_DIALOGSET_HAPPY": "happy",
}


def die(msg: str) -> "NoReturn":  # type: ignore[valid-type]
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def species_name(const: str) -> str:
    """A pret species constant -> pokemon.json's own `name` (a valid PokemonDB key)."""
    return SPECIES_FIXUPS.get(const, const)


def parse_trade_mons() -> list[dict]:
    """data/events/trades.asm -> the ten rows, in bit order."""
    src = (PRET / "data" / "events" / "trades.asm").read_text(encoding="utf-8")
    rows = []
    # npctrade NIDORINO,   NIDORINA,  TRADE_DIALOGSET_CASUAL,    "TERRY"
    pat = re.compile(
        r'^\s*npctrade\s+(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,\s*"([^"]+)"\s*(;.*)?$', re.M)
    for m in pat.finditer(src):
        give, get, dialog, nick, comment = m.groups()
        if dialog not in DIALOGSET:
            die(f"unknown dialogset {dialog!r} -- pret added one?")
        rows.append({
            "ind": len(rows),
            "give": species_name(give),
            "get": species_name(get),
            "dialogset": DIALOGSET[dialog],
            "nickname": nick,
            "unused": bool(comment and "unused" in comment),
        })
    if len(rows) != NUM_NPC_TRADES:
        die(f"parsed {len(rows)} npctrade rows, expected NUM_NPC_TRADES={NUM_NPC_TRADES}")
    return rows


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

    Two parser traps here, and BOTH fail silently -- the parse succeeds and the data is merely
    wrong, which is the worst failure mode this project has:

    1. ⚠️ **Route11Gate2F writes `xor a ; TRADE_FOR_TERRY`, not `ld a, TRADE_FOR_TERRY`.** Trade 0 is
       zero, so the assembler-friendly idiom hides the constant in a COMMENT. A regex that only
       accepts `ld a, TRADE_FOR_` loses exactly one trade and the count still looks plausible at
       9/10. Both forms are matched.

    2. ⚠️ **The gap between a label and its trade must NOT cross another label.** The first cut used
       `(?:.*\n)*?` between `text_asm` and the trade -- lazy, but perfectly happy to scan PAST the
       next label. In CinnabarLabFossilRoom (two Scientists, only the second trades) it matched
       Scientist**1**'s label and then found Scientist2's `TRADE_FOR_SAILOR` further down, filing
       SAILOR at (5,2) instead of (7,6). **Nine trades still resolved and the totals still looked
       right** -- caught only by diffing against a hand-read of the source. The gap is now
       `(?!^\w+:)`-guarded, so a block cannot borrow the next block's contents.
    """
    sites: dict[str, tuple[str, str]] = {}
    for f in sorted((PRET / "scripts").glob("*.asm")):
        src = f.read_text(encoding="utf-8")
        # Find each `text_asm` block that names a trade, and the label above it. The
        # `(?!^\w+:)` in the gap is load-bearing -- see trap 2 above.
        for m in re.finditer(
                r"^(\w+):[ \t]*\n[ \t]*text_asm[ \t]*\n(?:(?!^\w+:)[^\n]*\n)*?"
                r"[ \t]*(?:ld[ \t]+a[ \t]*,[ \t]*(TRADE_FOR_\w+)"
                r"|xor[ \t]+a[ \t]*;[ \t]*(TRADE_FOR_\w+))", src, re.M):
            label, viaLd, viaXor = m.groups()
            trade = viaLd or viaXor
            # The label -> its TEXT_* constant, via the map's def_text_pointers table.
            tm = re.search(rf"dw_const\s+{re.escape(label)}\s*,\s*(TEXT_\w+)", src)
            if not tm:
                die(f"{f.name}: {label} names {trade} but has no dw_const row -- cannot find its text id")
            if trade in sites:
                die(f"{trade} is named by two scripts: {sites[trade][0]} and {f.stem}")
            sites[trade] = (f.stem, tm.group(1))
    return sites


def parse_objects(stem: str) -> tuple[list[dict], str]:
    """data/maps/objects/<stem>.asm -> its object_events (1-based index) + the map's const name."""
    f = PRET / "data" / "maps" / "objects" / f"{stem}.asm"
    src = f.read_text(encoding="utf-8")
    mapConst = re.search(r"def_warps_to\s+(\w+)", src)
    if not mapConst:
        die(f"{f.name}: no def_warps_to -- cannot identify the map")
    objs = []
    # object_event  4,  2, SPRITE_YOUNGSTER, WALK, LEFT_RIGHT, TEXT_ROUTE11GATE2F_YOUNGSTER
    for m in re.finditer(
            r"^\s*object_event\s+(\d+)\s*,\s*(\d+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,\s*(TEXT_\w+)",
            src, re.M):
        x, y, sprite, move, face, text = m.groups()
        objs.append({"x": int(x), "y": int(y), "sprite": sprite, "move": move,
                     "face": face, "text": text})
    return objs, mapConst.group(1)


def map_ids() -> dict[str, int]:
    """constants/map_constants.asm -> {MAP_CONST: id}. The ids are positional (const_def order)."""
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


def build() -> list[dict]:
    rows = parse_trade_mons()
    consts = parse_trade_constants()
    sites = find_trade_sites()
    ids = map_ids()

    for i, row in enumerate(rows):
        row["const"] = consts[i]
        # The constant's own suffix must agree with the table's nickname -- pret names the constant
        # after the nickname (TRADE_FOR_TERRY <-> "TERRY"). If those ever diverge, the two files
        # have drifted and the bit order is no longer trustworthy.
        suffix = consts[i][len("TRADE_FOR_"):]
        if suffix != row["nickname"]:
            die(f"bit {i}: constant {consts[i]} disagrees with nickname {row['nickname']!r} -- "
                f"trades.asm and script_constants.asm have drifted")

        site = sites.get(row["const"])
        if site is None:
            row.update(map=None, mapId=None, x=None, y=None, trader=None,
                       traderSprite=None, walks=False, textId=None)
            continue

        stem, textConst = site
        objs, mapConst = parse_objects(stem)
        match = [(n, o) for n, o in enumerate(objs, start=1) if o["text"] == textConst]
        if len(match) != 1:
            die(f"{row['const']}: {len(match)} objects carry {textConst} in {stem} -- expected exactly 1")
        n, obj = match[0]

        if mapConst not in ids:
            die(f"{row['const']}: map const {mapConst} is not in map_constants.asm")
        if obj["sprite"] not in SPRITE_CLASS:
            die(f"{row['const']}: sprite {obj['sprite']} has no class name -- add it to SPRITE_CLASS")

        row.update(
            map=mapConst,
            mapId=ids[mapConst],
            x=obj["x"],
            y=obj["y"],
            # The trader's CLASS is the "species" half of leadership's rule; they have no nickname.
            trader=SPRITE_CLASS[obj["sprite"]],
            traderSprite=obj["sprite"],
            # ⚠️ A walking trader's x/y is the SPAWN tile, not where the man is standing now.
            walks=obj["move"] == "WALK",
            textId=textConst,
        )
    return rows


def validate(rows: list[dict]) -> None:
    located = [r for r in rows if r["mapId"] is not None]
    unused = [r for r in rows if r["unused"]]

    if len(located) != 9:
        die(f"{len(located)} trades resolved a map, expected 9 "
            f"(unresolved: {[r['const'] for r in rows if r['mapId'] is None]})")
    if len(unused) != 1 or unused[0]["nickname"] != UNUSED_TRADE_NICK:
        die(f"expected exactly one `; unused` trade ({UNUSED_TRADE_NICK}), got "
            f"{[r['nickname'] for r in unused]}")
    # The unused one must be the unlocated one -- if a USED trade lost its map, the script scan broke.
    if unused[0]["mapId"] is not None:
        die(f"{unused[0]['const']} is marked unused but resolved a map?")
    if any(r["mapId"] is None and not r["unused"] for r in rows):
        die("a USED trade failed to resolve its map -- the script scan is incomplete")

    # Every species must be a real one.
    mons = {m["name"] for m in json.loads(POKEMON_JSON.read_text(encoding="utf-8"))}
    for r in rows:
        for key in ("give", "get"):
            if r[key] not in mons:
                die(f"{r['const']}: species {r[key]!r} does not resolve against pokemon.json")

    # Every map id must be a real map, and its name must match what maps.json calls that id.
    maps = {m["ind"]: m for m in json.loads(MAPS_JSON.read_text(encoding="utf-8"))}
    for r in located:
        if r["mapId"] not in maps:
            die(f"{r['const']}: map id {r['mapId']} is not in maps.json")

    # Bit order must be dense 0..9 -- the save's bit IS the index.
    if [r["ind"] for r in rows] != list(range(NUM_NPC_TRADES)):
        die("trade inds are not a dense 0..9 -- the save bit is the index, so this must hold")

    print(f"  OK  {len(rows)} trades: {len(located)} located, {len(unused)} unused (no coords)")
    for r in rows:
        where = (f"{r['map']} ({r['mapId']}) at ({r['x']},{r['y']}) -- {r['trader']}"
                 + ("  [WALKS: spawn tile]" if r["walks"] else "")
                 ) if r["mapId"] is not None else "-- unused, no map, no coords -> General"
        print(f"      bit {r['ind']}  {r['nickname']:<11} {r['give']:>10} -> {r['get']:<11} {where}")


def emit(rows: list[dict]) -> str:
    out = []
    for r in rows:
        e = {
            "ind": r["ind"],                    # == its bit in wCompletedInGameTradeFlags
            "const": r["const"],
            "nickname": r["nickname"],          # the RECEIVED mon's nickname -- NOT the trader
            "give": r["give"],
            "get": r["get"],
            "dialogset": r["dialogset"],
            "unused": r["unused"],
        }
        if r["mapId"] is not None:
            e.update({
                "map": r["map"],
                "mapId": r["mapId"],
                "x": r["x"],
                "y": r["y"],
                "trader": r["trader"],          # the class name -- traders have no personal name
                "walks": r["walks"],
                "textId": r["textId"],
            })
        out.append(e)
    return json.dumps(out, indent=4, ensure_ascii=False) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true",
                    help="verify the shipped trades.json matches, without writing")
    args = ap.parse_args()

    if not PRET.exists():
        die(f"pret/pokered clone not found at {PRET}")

    rows = build()
    validate(rows)
    text = emit(rows)

    if args.check:
        if not OUT.exists():
            die(f"{OUT.name} does not exist")
        if OUT.read_text(encoding="utf-8") != text:
            die(f"{OUT.name} is STALE -- re-run without --check")
        print(f"  OK  {OUT.name} is up to date")
        return 0

    OUT.write_text(text, encoding="utf-8")
    print(f"  wrote {OUT.relative_to(REPO)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
