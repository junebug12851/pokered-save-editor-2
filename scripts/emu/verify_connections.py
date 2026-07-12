"""Prove (or disprove) our connection-strip maths against the real ROM -- every connection.

Connection strips are the most error-prone corner of the Gen 1 map engine: layered pointer
arithmetic with a clamp, signed offsets, and a length either map can limit. Getting them
"nearly" right is how you ship a renderer that looks fine until it doesn't. So we don't
reason about it -- we read the answer out of the cartridge.

The `connection` macro bakes an 11-byte struct into each map's header in ROM. This script:

  1. finds each map's header IN THE ROM by its 10-byte signature -- tileset, height, width
     and three pointers, straight out of maps.json (three 16-bit pointers make it unique),
  2. reads the connection structs that follow it (in the macro's own order: N, S, W, E),
  3. recomputes those same 11 bytes from maps.json using the macro's arithmetic,
  4. compares every field, for every connection in the game.

Any disagreement is printed. Silence means our model of the macro IS the game's.

> Why not drive the emulator for this? Because a Gen 1 save is a full WRAM snapshot -- it
> stores the live map header AND the connection structs -- so the game never re-reads them
> on CONTINUE. Building a save to "place" the player on another map would mean writing the
> very structs we are trying to verify. The ROM has no such problem: it is the source the
> game itself reads from. (The emulator still verifies the buffers those structs produce --
> see tst_emu_parity.)

Local-only; needs the gitignored ROM. Run: python scripts/emu/verify_connections.py
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
DATA = REPO / "projects" / "db" / "assets" / "data"

W_OVERWORLD_MAP = 0xC6E8
BANK_SIZE = 0x4000
CONN_SIZE = 11

# Connections are emitted in this order (macros/scripts/maps.asm says so out loud), and
# LoadMapHeader reads them back in the same order.
DIRS = ["north", "south", "west", "east"]

# wCurMapConnections bits (constants/map_data_constants.asm)
CONN_BIT = {"east": 0x01, "west": 0x02, "south": 0x04, "north": 0x08}


# ── the `connection` macro, reimplemented exactly ───────────────────────────────────

def connection_struct(direction, cur_w, cur_h, to_id, to_w, to_h, to_data_ptr, offset):
    """The 11 bytes the macro emits, from nothing but the header's own arguments.

        _tgt = offset + 3
        if _tgt < 2:  _src = -_tgt ; _tgt = 0        <- THE CLAMP

    _tgt is where the strip lands in OUR ring; _src is how far into the NEIGHBOUR it
    starts. A negative offset would push the destination out of the buffer, so instead the
    game slides the source along and pins the destination to 0. That asymmetry is the
    whole trick.
    """
    _src = 0
    _tgt = offset + 3
    if _tgt < 2:
        _src = -_tgt
        _tgt = 0

    if direction == "north":
        _blk = to_w * (to_h - 3) + _src          # the neighbour's LAST 3 rows
        _map = _tgt
        _win = (to_w + 6) * to_h + 1
        _y = to_h * 2 - 1
        _x = offset * -2
        _len = min(cur_w + 3 - offset, to_w)
    elif direction == "south":
        _blk = _src                              # its FIRST row
        _map = (cur_w + 6) * (cur_h + 3) + _tgt
        _win = to_w + 7
        _y = 0
        _x = offset * -2
        _len = min(cur_w + 3 - offset, to_w)
    elif direction == "west":
        _blk = to_w * _src + to_w - 3            # its LAST 3 columns
        _map = (cur_w + 6) * _tgt
        _win = (to_w + 6) * 2 - 6
        _y = offset * -2
        _x = to_w * 2 - 1
        _len = min(cur_h + 3 - offset, to_h)
    elif direction == "east":
        _blk = to_w * _src                       # its FIRST column
        _map = (cur_w + 6) * _tgt + cur_w + 3
        _win = to_w + 7
        _y = offset * -2
        _x = 0
        _len = min(cur_h + 3 - offset, to_h)
    else:
        raise ValueError(direction)

    return {
        "connectedMap": to_id & 0xFF,
        "stripSrc": (to_data_ptr + _blk) & 0xFFFF,
        "stripDest": (W_OVERWORLD_MAP + _map) & 0xFFFF,
        "stripLength": (_len - _src) & 0xFF,
        "connectedMapWidth": to_w & 0xFF,
        "yAlign": _y & 0xFF,
        "xAlign": _x & 0xFF,
        "viewPointer": (W_OVERWORLD_MAP + _win) & 0xFFFF,
    }


def raw_offset(strip_move: int, strip_offset: int) -> int:
    """Recover the macro's `offset` argument from what maps.json kept.

    maps.json stores the POST-clamp pair: stripMove == _tgt - 3, stripOffset == _src. The
    clamp is invertible -- a non-zero _src can only have come from _tgt < 2, where
    _src = -(offset + 3) -- so offset = -_src - 3. Otherwise offset == stripMove.

    (Checked against the real headers: Route 4 south = -25 -> stored (-3, 22); Route 11
    east = -27 -> stored (-3, 24); Route 2 north = -5 -> stored (-3, 2); Viridian north
    = +5 -> stored (5, 0).)
    """
    return -strip_offset - 3 if strip_offset != 0 else strip_move


# ── reading the real ROM ────────────────────────────────────────────────────────────

def rom_offset(bank: int, addr: int) -> int:
    """A banked ROM address -> a file offset."""
    return bank * BANK_SIZE + (addr - 0x4000)


def find_header(rom: bytes, m: dict, conn_bits: int) -> int | None:
    """Locate a map's header in the ROM by its 10-byte signature.

    tileset, height, width, blocksPtr, textPtr, scriptPtr, connections -- exactly what
    map_header emits (macros/scripts/maps.asm) and exactly what LoadMapHeader copies into
    wCurMapHeader. Three 16-bit pointers in ten bytes is plenty unique.
    """
    sig = bytes([
        m["tilesetInd"], m["height"], m["width"],
        m["dataPtr"] & 0xFF, (m["dataPtr"] >> 8) & 0xFF,
        m["textPtr"] & 0xFF, (m["textPtr"] >> 8) & 0xFF,
        m["scriptPtr"] & 0xFF, (m["scriptPtr"] >> 8) & 0xFF,
        conn_bits,
    ])

    bank = m["bank"]
    start, end = bank * BANK_SIZE, (bank + 1) * BANK_SIZE

    hits = []
    i = rom.find(sig, start, end)
    while i != -1:
        hits.append(i)
        i = rom.find(sig, i + 1, end)

    if len(hits) != 1:
        return None
    return hits[0]


def parse_struct(rom: bytes, at: int) -> dict:
    b = rom[at:at + CONN_SIZE]
    return {
        "connectedMap": b[0],
        "stripSrc": b[1] | (b[2] << 8),
        "stripDest": b[3] | (b[4] << 8),
        "stripLength": b[5],
        "connectedMapWidth": b[6],
        "yAlign": b[7],
        "xAlign": b[8],
        "viewPointer": b[9] | (b[10] << 8),
    }


def main() -> int:
    if not ROM.exists():
        print(f"SKIP: no ROM at {ROM} (local-only verification)")
        return 2

    rom = ROM.read_bytes()
    maps = json.loads((DATA / "maps.json").read_text(encoding="utf-8"))
    tilesets = json.loads((DATA / "tileset.json").read_text(encoding="utf-8"))

    ts_ind = {t["nameAlias"]: t["ind"] for t in tilesets}
    by_name = {m["name"]: m for m in maps if m.get("name")}

    connected = [m for m in maps if m.get("connect") and m.get("width")]
    print(f"maps with connections : {len(connected)}")

    checked = mismatched = 0
    not_found = []
    cross_bank = []

    for m in connected:
        conn_bits = 0
        for d in DIRS:
            if m["connect"].get(d):
                conn_bits |= CONN_BIT[d]

        m = dict(m, tilesetInd=ts_ind.get(m.get("tileset"), 0))
        at = find_header(rom, m, conn_bits)
        if at is None:
            not_found.append(m["name"])
            continue

        cur = at + 10   # past the 10-byte header; the structs follow, in N/S/W/E order

        for d in DIRS:
            c = m["connect"].get(d)
            if not c:
                continue

            to = by_name.get(c["map"])
            if to is None:
                print(f"!! {m['name']} {d}: '{c['map']}' does not resolve")
                continue

            offset = raw_offset(int(c.get("stripMove", 0)), int(c.get("stripOffset", 0)))
            ours = connection_struct(d, m["width"], m["height"], to["ind"], to["width"],
                                     to["height"], to["dataPtr"], offset)
            theirs = parse_struct(rom, cur)
            cur += CONN_SIZE

            checked += 1
            bad = {k: (theirs[k], ours[k]) for k in ours if theirs[k] != ours[k]}
            if bad:
                mismatched += 1
                print(f"\nMISMATCH  {m['name']} {d} -> {c['map']}  (offset {offset}, "
                      f"stripMove={c.get('stripMove')}, stripOffset={c.get('stripOffset')}, "
                      f"flag={c.get('flag', False)})")
                for k, (t, o) in bad.items():
                    print(f"            {k:<18} ROM={t:<6} ours={o}")

            # The strip copy does SwitchToMapRomBank -- the CURRENT map's bank -- and then
            # reads the NEIGHBOUR's blocks through it. If the two maps' block data lives in
            # different banks, the game reads whatever is at that address in the current
            # bank instead. Worth knowing before we model the address space.
            if to["bank"] != m["bank"]:
                cross_bank.append(f"{m['name']} {d} -> {c['map']} (bank {m['bank']} vs {to['bank']})")

    print(f"\nconnections checked   : {checked}")
    print(f"mismatches            : {mismatched}")
    if not_found:
        print(f"headers not located   : {not_found}")
    print(f"cross-bank connections: {len(cross_bank)}  "
          f"(fine -- SwitchToMapRomBank is called with the CONNECTED map's id, so the "
          f"strip reads in the neighbour's bank)")

    # Does any strip read PAST the neighbour's block array? That decides whether the
    # renderer needs a real ROM address-space model or can get away with per-map arrays.
    print("\n=== over-read analysis (does a strip run off the end of the neighbour?) ===")
    overruns = []
    for m in connected:
        for d in DIRS:
            c = m["connect"].get(d)
            if not c:
                continue
            to = by_name.get(c["map"])
            if not to:
                continue

            offset = raw_offset(int(c.get("stripMove", 0)), int(c.get("stripOffset", 0)))
            s = connection_struct(d, m["width"], m["height"], to["ind"], to["width"],
                                  to["height"], to["dataPtr"], offset)

            base, W, H = to["dataPtr"], to["width"], to["height"]
            lo, hi = base, base + W * H - 1            # the neighbour's block array

            src, length = s["stripSrc"], s["stripLength"]
            if d in ("north", "south"):
                # 3 rows, `length` bytes each, stepping by the neighbour's width
                first = src
                last = src + 2 * W + length - 1
            else:
                # `length` rows, 3 bytes each
                first = src
                last = src + (length - 1) * W + 2

            if first < lo or last > hi:
                overruns.append(
                    f"{m['name']:<22} {d:<5} -> {c['map']:<22} reads "
                    f"0x{first:04X}..0x{last:04X}, the map is 0x{lo:04X}..0x{hi:04X} "
                    f"({'before' if first < lo else ''}{'/' if first < lo and last > hi else ''}"
                    f"{'past the end' if last > hi else ''}"
                    f", by {max(lo - first, 0) + max(last - hi, 0)} bytes)")

    if overruns:
        print(f"{len(overruns)} of {checked} strips read OUTSIDE the neighbour's block array:")
        for o in overruns:
            print(f"  {o}")
        print("\n  -> the renderer MUST model the ROM address space, not per-map arrays.")
    else:
        print("none -- every strip stays inside the neighbour's own block data.")

    return 1 if (mismatched or not_found) else 0


if __name__ == "__main__":
    sys.exit(main())
