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

EV_START, EV_LEN = 0x29F3, 0x140          # wEventFlags in the file (320 bytes)
SAV_CUR_MAP, SAV_Y, SAV_X = 0x260A, 0x260D, 0x260E
CK, CK0, CKN = 0x3523, 0x2598, 0xF8B      # checksum byte; summed range


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
          pokes: dict[int, int] | None = None) -> bytes:
    """Return a resealed save forged from `base`. Only the requested bytes move."""
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
    if map_id is not None:
        sav[SAV_CUR_MAP] = map_id & 0xFF
    if y is not None:
        sav[SAV_Y] = y & 0xFF
    if x is not None:
        sav[SAV_X] = x & 0xFF
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
