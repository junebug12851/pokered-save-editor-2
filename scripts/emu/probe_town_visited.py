r"""Can you un-visit a town? -- and can you un-visit the one you are STANDING IN?

`wTownVisitedFlag` (save 0x29B7-0x29B8 -> $D70B-$D70C) is a `flag_array NUM_CITY_MAPS` = 11 bits,
and **bit i is map id i** -- `engine/overworld/toggleable_objects.asm` leaves no room for doubt:

    MarkTownVisitedAndLoadToggleableObjects::
        ld a, [wCurMap]
        cp FIRST_ROUTE_MAP
        jr nc, .notInTown       ; only maps BELOW the first route are towns
        ld c, a                 ; <- the flag index IS wCurMap, no translation
        ld b, FLAG_SET
        ld hl, wTownVisitedFlag ; mark town as visited (for flying)
        predef FlagActionPredef
    .notInTown

The claim under test, and it is the one that shapes the UI:

    `MarkTownVisitedAndLoadToggleableObjects` is the FIRST LINE of `LoadMapHeader` (home/overworld.asm
    line 2006), and `LoadMapHeader`'s `BIT_NO_PREVIOUS_MAP` early-out -- the linchpin that lets warps,
    signs and wild data survive a Continue -- is at line 2017, ELEVEN LINES LATER.

    => the flag is written BEFORE the protection every other per-map thing enjoys, so the town you
       are saved in should RE-MARK ITSELF on every Continue. You cannot un-visit where you stand.

...and its rider: the `cp FIRST_ROUTE_MAP` guard means this only fires when `wCurMap` is one of the
11 city maps. Saved indoors, or on a route, nothing is re-marked.

That is a load-bearing persistence claim read off the asm, and an asm read has been wrong before on
exactly this kind of question (sprite persistence, 2026-07-13; wMovementFlags clears per-routine,
2026-07-14). So the console gets asked.

BaseSAV is ideal for it: wCurMap = 0 (PALLET_TOWN, outdoors) and the flags are already 0xFF 0x07 --
all eleven set -- so a single cleared bit is a clean, isolated question.

  A  baseline        untouched BaseSAV                  -> all 11 set, as shipped
  B  clear CURRENT   Pallet's bit (0) := 0, in Pallet   -> predict: BACK to 1 (re-marked)
  C  clear OTHER     Viridian's bit (1) := 0, in Pallet -> predict: stays 0  (the control:
                     proves loading does not simply set everything)
  D  clear on ROUTE  Pallet's bit (0) := 0, on Route 12 -> predict: stays 0  (proves the
                     cp FIRST_ROUTE_MAP guard, using the forged Route 12 bench save)

C is what makes B mean anything. Without it, "the bit is set after loading" is also what you would
see if the loader set all of them, or if the write never happened at all.

Local-only; needs the gitignored ROM. Run:
    tmp\emu-venv\Scripts\python.exe scripts\emu\probe_town_visited.py
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
ROUTE12_SAV = REPO / "assets" / "saves" / "forged-maps" / "Route12.sav"
OUT = REPO / "tmp" / "emu-towns"

# ── save offsets ────────────────────────────────────────────────────────────────────
# sMainData starts at 0x25A3 and maps wMainDataStart = $D2F7, so  wram = sav + 0xAD54.
SAV_TOWN_VISITED = 0x29B7          # wTownVisitedFlag           $D70B  (2 bytes, 11 bits used)
SAV_CUR_MAP = 0x260A               # wCurMap                    $D35E

SAV_CHECKSUM = 0x3523
SAV_CHECKSUM_START = 0x2598
SAV_CHECKSUM_LEN = 0xF8B

# ── wram addresses ──────────────────────────────────────────────────────────────────
W_TOWN_VISITED = 0xD70B            # 0x29B7 + 0xAD54
W_CUR_MAP = 0xD35E

W_CUR_MAP_WIDTH = 0xD369
W_CUR_MAP_HEIGHT = 0xD368
W_OVERWORLD_MAP = 0xC6E8
W_TILEMAP = 0xC3A0
SCREEN_TILES = 20 * 18

NUM_CITY_MAPS = 11
FIRST_ROUTE_MAP = 12               # const_value after UNUSED_MAP_0B ($0B)

# Bit i == map id i. NOT a list order -- see notes/reference/town-visited.md §4 for the seven years
# fly.json spent disagreeing with this.
TOWNS = [
    "Pallet Town", "Viridian City", "Pewter City", "Cerulean City", "Lavender Town",
    "Vermilion City", "Celadon City", "Fuchsia City", "Cinnabar Island", "Indigo Plateau",
    "Saffron City",
]


def checksum(sav: bytearray) -> int:
    c = 0xFF
    for i in range(SAV_CHECKSUM_START, SAV_CHECKSUM_START + SAV_CHECKSUM_LEN):
        c = (c - sav[i]) & 0xFF
    return c


def sealed(sav: bytearray) -> bytes:
    sav[SAV_CHECKSUM] = checksum(sav)
    return bytes(sav)


def clear_town(base: bytes, town: int) -> bytes:
    """Clear exactly one town's bit -- byte = 0x29B7 + i/8, mask = 1 << (i%8) (FlagAction's own math)."""
    sav = bytearray(base)
    sav[SAV_TOWN_VISITED + (town >> 3)] &= ~(1 << (town & 7)) & 0xFF
    return sealed(sav)


def read_bits(hi: int, lo: int) -> list[int]:
    word = lo | (hi << 8)
    return [(word >> i) & 1 for i in range(NUM_CITY_MAPS)]


def boot(pyboy, budget: int = 9000) -> bool:
    def on_overworld() -> bool:
        mem = pyboy.memory
        w, h = mem[W_CUR_MAP_WIDTH], mem[W_CUR_MAP_HEIGHT]
        if not (0 < w <= 64 and 0 < h <= 96):
            return False
        blocks = bytes(mem[W_OVERWORLD_MAP:W_OVERWORLD_MAP + (w + 6) * (h + 6)])
        screen = bytes(mem[W_TILEMAP:W_TILEMAP + SCREEN_TILES])
        return len(set(blocks)) > 1 and len(set(screen)) > 1

    frames = 0
    while frames < budget and not on_overworld():
        pyboy.button("start" if (frames // 24) % 2 == 0 else "a", delay=8)
        for _ in range(24):
            pyboy.tick()
        frames += 24

    if not on_overworld():
        return False
    for _ in range(240):
        pyboy.tick()
    return True


def run(rom: Path, sav: bytes, label: str) -> dict | None:
    from pyboy import PyBoy
    (OUT / "rom.gb.ram").write_bytes(sav)
    pyboy = PyBoy(str(rom), window="null", sound_emulated=False)
    if not boot(pyboy):
        pyboy.stop(save=False)
        print(f"  {label}: never reached the overworld")
        return None
    mem = pyboy.memory
    r = {
        "curMap": mem[W_CUR_MAP],
        "lo": mem[W_TOWN_VISITED],
        "hi": mem[W_TOWN_VISITED + 1],
    }
    r["bits"] = read_bits(r["hi"], r["lo"])
    pyboy.screen.image.save(OUT / f"{label}.png")
    pyboy.stop(save=False)
    return r


def show(label: str, r: dict) -> None:
    inTown = r["curMap"] < FIRST_ROUTE_MAP
    where = TOWNS[r["curMap"]] if inTown else f"map {r['curMap']} (not a town)"
    print(f"\n  {label}")
    print(f"    wCurMap                 {r['curMap']}  -- {where}")
    print(f"    wTownVisitedFlag        0x{r['lo']:02X} 0x{r['hi']:02X}")
    on = [TOWNS[i] for i, b in enumerate(r["bits"]) if b]
    off = [TOWNS[i] for i, b in enumerate(r["bits"]) if not b]
    print(f"    visited ({len(on)})            {', '.join(on) if on else '(none)'}")
    print(f"    NOT visited ({len(off)})        {', '.join(off) if off else '(none)'}")


def main() -> int:
    if not ROM.exists():
        print("SKIP: no ROM (local-only verification)")
        return 2

    OUT.mkdir(parents=True, exist_ok=True)
    rom = OUT / "rom.gb"
    shutil.copyfile(ROM, rom)
    base = bytes(BASE_SAV.read_bytes())

    if base[SAV_CUR_MAP] >= FIRST_ROUTE_MAP:
        print(f"BaseSAV is on map {base[SAV_CUR_MAP]}, not in a town -- B is not a valid test.")
        return 1

    home = base[SAV_CUR_MAP]                       # the town the save is standing in (Pallet = 0)
    other = 1 if home != 1 else 2                  # any other town, to act as the control
    print(f"  BaseSAV stands in {TOWNS[home]} (map {home}); control town = {TOWNS[other]} ({other})")

    a = run(rom, sealed(bytearray(base)), "A_baseline")
    b = run(rom, clear_town(base, home), "B_clear_current_town")
    c = run(rom, clear_town(base, other), "C_clear_other_town")

    d = None
    if ROUTE12_SAV.exists():
        r12 = bytes(ROUTE12_SAV.read_bytes())
        if r12[SAV_CUR_MAP] >= FIRST_ROUTE_MAP:
            d = run(rom, clear_town(r12, home), "D_clear_while_on_a_route")
        else:
            print(f"  Route12.sav is on map {r12[SAV_CUR_MAP]} -- expected a non-town; skipping D")

    if a:
        show("A  baseline (untouched BaseSAV)", a)
    if b:
        show(f"B  {TOWNS[home]}'s bit CLEARED, saved IN {TOWNS[home]}", b)
    if c:
        show(f"C  {TOWNS[other]}'s bit CLEARED, saved in {TOWNS[home]} (control)", c)
    if d:
        show(f"D  {TOWNS[home]}'s bit CLEARED, saved on a ROUTE (guard test)", d)

    print("\n  VERDICT")
    ok = True

    if b:
        re_marked = b["bits"][home] == 1
        print(f"    Does the town you STAND IN re-mark itself?   "
              f"{'YES -- read back SET despite being cleared' if re_marked else 'NO -- stayed clear'}")
        print(f"      => you {'CANNOT' if re_marked else 'CAN'} un-visit the town you are saved in.")
        ok &= re_marked

    if c:
        stayed = c["bits"][other] == 0
        print(f"    Control: a DIFFERENT town's bit stays clear? "
              f"{'YES -- the edit held' if stayed else 'NO -- something re-set it too'}")
        print(f"      => the loader is {'not' if stayed else 'ALSO'} setting bits it was not asked to.")
        ok &= stayed

    if d:
        stayed = d["bits"][home] == 0
        print(f"    Guard: cleared while on a route, stays clear? "
              f"{'YES -- cp FIRST_ROUTE_MAP bailed' if stayed else 'NO -- re-marked anyway (!)'}")
        print(f"      => the re-mark is {'genuinely gated on standing in a town' if stayed else 'NOT gated as the asm reads'}.")
        ok &= stayed

    if b and c:
        print(f"\n    So the flags are DURABLE, with exactly one conditional exception: the bit for "
              f"the map the save sits on, when that map is a town.")
    print(f"\n    Matches the prediction in notes/reference/town-visited.md: "
          f"{'YES' if ok else 'NO -- the note must be corrected'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
