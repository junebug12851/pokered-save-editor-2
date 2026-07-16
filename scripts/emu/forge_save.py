#!/usr/bin/env python3
"""Forge a synthetic Gen-1 save at ANY map / position / flag state — no PyBoy needed.

This is the STANDING METHOD from notes/reference/emulator-verification.md, extracted
into one reusable, importable module so every probe (and the dev MCP server) shares a
single forge + checksum-reseal implementation instead of each carrying its own copy.

Pure byte manipulation: works under any Python, no dependencies, no ROM required.
The checksum is the 0xFF-running-subtract over 0x2598..0x2598+0xF8B, stored at 0x3523;
without resealing it the game silently rejects the save and boots a NEW GAME (the trap
that fooled the glitch-music probe once).

Usage (CLI):
    python forge_save.py --out tmp/emu/forged.sav [--base assets/saves/natural-clean/BaseSAV.sav]
                         [--map 0x21] [--x 8] [--y 8]
                         [--flag EVENT_...]* [--flag-index N]* [--all-flags]
                         [--poke 0x260A=0x21]*

Usage (import):
    from forge_save import forge, reseal
"""
from __future__ import annotations
import argparse
import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
CANON = REPO / "tmp" / "event-flags" / "event_flags_canonical.json"
SCRIPTS_JSON = REPO / "projects" / "db" / "assets" / "data" / "scripts.json"
MISSABLES_JSON = REPO / "projects" / "db" / "assets" / "data" / "missables.json"

SCRIPTS_START = 0x289C     # w<Map>CurScript block (97 values; layout = scripts.json)
FILTER_START = 0x2852      # wToggleableObjectFlags (filter flags; bit SET = HIDDEN)


def script_offsets() -> dict:
    """Per-map script-progress save offsets, keyed by BOTH the entry ind and its
    name(s) — the 0x289C layout walk (sizes + skips), pinned byte-exact by
    tst_world::scripts_writeExactlyTheirByte."""
    out = {}
    off = SCRIPTS_START
    for e in json.loads(SCRIPTS_JSON.read_text(encoding="utf-8")):
        out[e["ind"]] = off
        out[e["name"]] = off
        for mn in e.get("maps") or []:
            out[mn] = off
        off += e.get("size", 1) + e.get("skip", 0)
    return out


def filter_flag_index() -> dict:
    """Filter-flag ("missable") bit indexes keyed by ind and by 'Map/Name'."""
    out = {}
    for e in json.loads(MISSABLES_JSON.read_text(encoding="utf-8")):
        out[e["ind"]] = e["ind"]
        out[f"{e['map']}/{e['name']}"] = e["ind"]
    return out

EV_START, EV_LEN = 0x29F3, 0x140          # wEventFlags in the file (320 bytes)
SAV_CUR_MAP, SAV_Y, SAV_X = 0x260A, 0x260D, 0x260E
SAV_YBLK, SAV_XBLK = 0x260F, 0x2610       # wY/XBlockCoord -- coord & 1 (BaseSAV-verified)
SAV_VIEW = 0x260B                          # wCurrentTileBlockMapViewPointer (LE)
SAV_W, SAV_H = 0x2615, 0x2614             # wCurMapWidth/Height (blocks)
CK, CK0, CKN = 0x3523, 0x2598, 0xF8B      # checksum byte; summed range

OVERWORLD_MAP_ADDR = 0xC6E8               # wOverworldMap (MapEngine::overworldMapAddr)
MAP_BORDER = 3                             # border blocks each side


def view_pointer(x: int, y: int, map_width_blocks: int) -> int:
    """The WRAM address the game stores in wCurrentTileBlockMapViewPointer, from
    the player's square coords + the map width. MapEngine::viewPointer verbatim —
    cartridge-verified byte-for-byte (tst_map viewPointer_matchesWhatTheGameStored,
    re-confirmed against BaseSAV: (5,6) w=10 -> 0xC72B)."""
    bx = MAP_BORDER + (x // 2) - 2
    by = MAP_BORDER + (y // 2) - 2
    return OVERWORLD_MAP_ADDR + by * (map_width_blocks + 2 * MAP_BORDER) + bx


def relocate(sav: bytearray, x: int, y: int) -> None:
    """Move the player WITHIN the save's current map, keeping every derived byte
    in sync — the 'little calculations on the side': block coords (coord & 1) and
    the view pointer (recomputed, little-endian). Bounds-checked against the
    map's own dimensions (coords are in SQUARES = 2x the block dims); refusing an
    out-of-range coord is a stability guarantee, not a limitation. Does NOT
    reseal — callers compose further edits, then reseal()."""
    w_blocks, h_blocks = sav[SAV_W], sav[SAV_H]
    if not (0 <= x < 2 * w_blocks and 0 <= y < 2 * h_blocks):
        raise ValueError(f"({x},{y}) is off the map ({2*w_blocks}x{2*h_blocks} squares)")
    sav[SAV_Y], sav[SAV_X] = y, x
    sav[SAV_YBLK], sav[SAV_XBLK] = y & 1, x & 1
    vp = view_pointer(x, y, w_blocks)
    sav[SAV_VIEW] = vp & 0xFF
    sav[SAV_VIEW + 1] = (vp >> 8) & 0xFF


def flag_name_index() -> dict:
    """pret EVENT_* name -> bit index, from the canonical import (empty if absent)."""
    if not CANON.exists():
        return {}
    return {r["name"]: r["index"]
            for r in json.loads(CANON.read_text(encoding="utf-8")) if r.get("name")}


def reseal(sav: bytearray) -> bytes:
    """Recompute + store the main-data checksum. Returns immutable bytes."""
    c = 0xFF
    for i in range(CK0, CK0 + CKN):
        c = (c - sav[i]) & 0xFF
    sav[CK] = c
    return bytes(sav)


def forge(base: bytes,
          map_id: int | None = None,
          y: int | None = None,
          x: int | None = None,
          flag_names: list[str] | None = None,
          flag_indices: list[int] | None = None,
          all_flags: bool = False,
          pokes: dict[int, int] | None = None,
          scripts: dict | None = None,
          filter_flags: dict | None = None) -> bytes:
    """Return a resealed save forged from `base`. Only the requested bytes move.

    Coordinates go through relocate() — block coords and the view pointer stay in
    sync (a bare y/x poke leaves a stale view pointer the console TRUSTS on load).

    ⚠️ `map_id` must match the base save's own map (or be omitted). Writing a
    DIFFERENT map id makes a CHIMERA — the Area block still belongs to the old
    map and the console hard-wedges ~100 frames after Continue (verified
    2026-07-16). For a real cross-map save use scripts/emu/forge_map_save.py,
    which has the console author the whole state itself."""
    sav = bytearray(base)
    if all_flags:
        for i in range(EV_LEN):
            sav[EV_START + i] = 0xFF
    if flag_names:
        idx = flag_name_index()
        for nm in flag_names:
            i = idx.get(nm)
            if i is None:
                raise KeyError(f"unknown event flag name: {nm} "
                               f"(is tmp/event-flags/event_flags_canonical.json present?)")
            sav[EV_START + i // 8] |= (1 << (i % 8))
    for i in (flag_indices or []):
        if not 0 <= i < EV_LEN * 8:
            raise ValueError(f"flag index out of range: {i}")
        sav[EV_START + i // 8] |= (1 << (i % 8))
    if map_id is not None and map_id != sav[SAV_CUR_MAP]:
        raise ValueError(
            f"map_id {map_id:#04x} != the base save's map {sav[SAV_CUR_MAP]:#04x} — "
            f"a map-id-only forge is a chimera that wedges the console. Generate a "
            f"real base with scripts/emu/forge_map_save.py first.")
    if y is not None or x is not None:
        relocate(sav,
                 sav[SAV_X] if x is None else x,
                 sav[SAV_Y] if y is None else y)
    # Per-map script progress: {"Route 22": 0} or {22: 0} — the map's story step.
    if scripts:
        offs = script_offsets()
        for key, val in scripts.items():
            if key not in offs:
                raise KeyError(f"unknown map script: {key!r}")
            sav[offs[key]] = int(val) & 0xFF
    # Filter flags (missables): {"Route 22/Rival 1": "show"} or {34: "hide"} —
    # bit SET = HIDDEN in wToggleableObjectFlags.
    if filter_flags:
        idx = filter_flag_index()
        for key, state in filter_flags.items():
            if key not in idx:
                raise KeyError(f"unknown filter flag: {key!r}")
            bit = idx[key]
            hidden = state in (True, 1) or (isinstance(state, str)
                                            and state.lower() in ("hide", "hidden"))
            if hidden:
                sav[FILTER_START + bit // 8] |= (1 << (bit % 8))
            else:
                sav[FILTER_START + bit // 8] &= ~(1 << (bit % 8))
    for addr, val in (pokes or {}).items():
        if not 0 <= addr < len(sav):
            raise ValueError(f"poke address out of range: {addr:#x}")
        sav[addr] = val & 0xFF
    return reseal(sav)


def _int(s: str) -> int:
    return int(s, 0)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--base", default=str(BASE_SAV))
    ap.add_argument("--out", required=True)
    ap.add_argument("--map", type=_int, default=None)
    ap.add_argument("--x", type=_int, default=None)
    ap.add_argument("--y", type=_int, default=None)
    ap.add_argument("--flag", action="append", default=[], help="EVENT_* name (repeatable)")
    ap.add_argument("--flag-index", action="append", type=_int, default=[],
                    help="raw bit index 0..2559 (repeatable)")
    ap.add_argument("--all-flags", action="store_true")
    ap.add_argument("--poke", action="append", default=[],
                    help="OFFSET=VALUE raw byte poke, e.g. 0x29B9=0x01 (repeatable)")
    a = ap.parse_args()

    pokes = {}
    for p in a.poke:
        off, _, val = p.partition("=")
        pokes[_int(off)] = _int(val)

    base = Path(a.base).read_bytes()
    out = forge(base, a.map, a.y, a.x, a.flag, a.flag_index, a.all_flags, pokes)
    Path(a.out).parent.mkdir(parents=True, exist_ok=True)
    Path(a.out).write_bytes(out)
    print(f"forged {a.out} ({len(out)} bytes, checksum {out[CK]:#04x})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
