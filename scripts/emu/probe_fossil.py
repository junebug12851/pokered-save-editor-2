r"""Do the two fossil bytes survive a Continue -- and does wFossilMon really take any species?

  wFossilItem  save 0x29BB -> $D70F   "item given to cinnabar lab"  (an ITEM id)
  wFossilMon   save 0x29BC -> $D710   "mon that will result"        (a SPECIES index)

Both sit in SECTION "Main Data", between wSafariSteps (0x29B9, 2 bytes) and wTownVisitedFlag's
neighbourhood -- a region where five separate probes have now found durable data. That is a strong
argument. It is still an argument, so the bytes get asked.

The second question is the interesting one. `CinnabarLabFossilRoom.asm` hands the mon over with:

    ld a, [wFossilMon]
    ld b, a
    ld c, 30              ; always level 30
    call GivePokemon

-- it reads the BYTE, and never re-derives it from wFossilItem. So the two bytes are independent
after handover, and wFossilMon alone decides what walks out of the machine. A save editor can write
any species there. This probe proves the byte survives the trip intact at a value the game would
never itself write (MEWTWO), because "durable" for a 1-bit flag and "durable across the full 0-255
range" are different claims.

  A  baseline    untouched BaseSAV                       -> as shipped
  B  a real pair  DOME_FOSSIL / KABUTO                   -> kept?
  C  a HACK pair  DOME_FOSSIL / MEWTWO                   -> kept? (the mismatched pair the notes
                                                            say is legal -- if the console
                                                            re-derived it, C would come back KABUTO)
  D  zeroed       both := 0                              -> control: stays 0 (the loader writes none)

C is the one that earns the probe: it is the difference between "these two bytes agree" and "these
two bytes are DERIVED from each other", and the whole UI doctrine turns on it -- a derived pair would
have to be kept in sync, an independent pair must not be.

Local-only; needs the gitignored ROM. Run:
    tmp\emu-venv\Scripts\python.exe scripts\emu\probe_fossil.py
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
OUT = REPO / "tmp" / "emu-fossil"

# ── save offsets ────────────────────────────────────────────────────────────────────
SAV_FOSSIL_ITEM = 0x29BB           # wFossilItem  $D70F
SAV_FOSSIL_MON = 0x29BC            # wFossilMon   $D710

SAV_CHECKSUM = 0x3523
SAV_CHECKSUM_START = 0x2598
SAV_CHECKSUM_LEN = 0xF8B

# ── wram addresses ──────────────────────────────────────────────────────────────────
W_FOSSIL_ITEM = 0xD70F
W_FOSSIL_MON = 0xD710

W_CUR_MAP_WIDTH = 0xD369
W_CUR_MAP_HEIGHT = 0xD368
W_OVERWORLD_MAP = 0xC6E8
W_TILEMAP = 0xC3A0
SCREEN_TILES = 20 * 18

# item ids (constants/item_constants.asm) and species INTERNAL indices (not pokedex numbers).
DOME_FOSSIL = 0x29
KABUTO = 0xB1
MEWTWO = 0x83

ITEMS = {0x00: "(none)", 0x29: "DOME_FOSSIL", 0x2A: "HELIX_FOSSIL", 0x1F: "OLD_AMBER"}
MONS = {0x00: "(none)", 0xB1: "KABUTO", 0xB2: "OMANYTE", 0xAB: "AERODACTYL", 0x83: "MEWTWO"}


def checksum(sav: bytearray) -> int:
    c = 0xFF
    for i in range(SAV_CHECKSUM_START, SAV_CHECKSUM_START + SAV_CHECKSUM_LEN):
        c = (c - sav[i]) & 0xFF
    return c


def sealed(sav: bytearray) -> bytes:
    sav[SAV_CHECKSUM] = checksum(sav)
    return bytes(sav)


def with_fossil(base: bytes, item: int, mon: int) -> bytes:
    sav = bytearray(base)
    sav[SAV_FOSSIL_ITEM] = item
    sav[SAV_FOSSIL_MON] = mon
    return sealed(sav)


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
    r = {"item": mem[W_FOSSIL_ITEM], "mon": mem[W_FOSSIL_MON]}
    pyboy.stop(save=False)
    return r


def show(label: str, r: dict) -> None:
    print(f"\n  {label}")
    print(f"    wFossilItem  0x{r['item']:02X}  {ITEMS.get(r['item'], '(other)')}")
    print(f"    wFossilMon   0x{r['mon']:02X}  {MONS.get(r['mon'], '(other)')}")


def main() -> int:
    if not ROM.exists():
        print("SKIP: no ROM (local-only verification)")
        return 2

    OUT.mkdir(parents=True, exist_ok=True)
    rom = OUT / "rom.gb"
    shutil.copyfile(ROM, rom)
    base = bytes(BASE_SAV.read_bytes())

    a = run(rom, sealed(bytearray(base)), "A_baseline")
    b = run(rom, with_fossil(base, DOME_FOSSIL, KABUTO), "B_real_pair")
    c = run(rom, with_fossil(base, DOME_FOSSIL, MEWTWO), "C_hack_pair")
    d = run(rom, with_fossil(base, 0, 0), "D_zeroed")

    if a:
        show("A  baseline (untouched BaseSAV)", a)
    if b:
        show("B  DOME_FOSSIL / KABUTO -- the pair the game would write", b)
    if c:
        show("C  DOME_FOSSIL / MEWTWO -- the MISMATCHED pair", c)
    if d:
        show("D  both zeroed (control)", d)

    print("\n  VERDICT")
    ok = True

    if b:
        kept = b["item"] == DOME_FOSSIL and b["mon"] == KABUTO
        print(f"    A real pair survives a Continue?      "
              f"{'YES -- both bytes intact' if kept else 'NO'}")
        ok &= kept

    if c:
        keptItem = c["item"] == DOME_FOSSIL
        keptMon = c["mon"] == MEWTWO
        print(f"    The MISMATCHED pair survives?         "
              f"{'YES -- item AND mon both intact' if (keptItem and keptMon) else 'NO'}")
        print(f"      wFossilMon still MEWTWO?            "
              f"{'YES' if keptMon else 'NO -- came back as ' + MONS.get(c['mon'], hex(c['mon']))}")
        print(f"      => the two bytes are {'INDEPENDENT -- the console does not re-derive the mon' if keptMon else 'DERIVED -- the note is WRONG and must be corrected'}.")
        print(f"      => the derived-value sync doctrine {'does NOT apply here; show both, sync neither' if keptMon else 'DOES apply -- sync them'}.")
        ok &= keptItem and keptMon

    if d:
        stayed = d["item"] == 0 and d["mon"] == 0
        print(f"    Control: zeroed stay zero?            "
              f"{'YES -- the loader writes neither' if stayed else 'NO -- something wrote them'}")
        ok &= stayed

    print(f"\n    Matches the prediction in notes/reference/fossil-revival.md: "
          f"{'YES' if ok else 'NO -- the note must be corrected'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
