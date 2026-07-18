r"""Do the eight "completed" one-shots survive a Continue -- and do their bytes get cleared WHOLE?

The eight (`WorldCompleted`), and the reason this probe is not optional:

    0x29D4  wStatusFlags1  b3 OLD_ROD   b4 GOOD_ROD   b5 SUPER_ROD   b6 SAFFRON_GUARDS_DRINK
    0x29DA  wStatusFlags4  b0 GOT_LAPRAS   b2 USED_POKECENTER   b3 GOT_STARTER
    0x29E0  wElite4Flags   b1 STARTED_ELITE_4

All eight live in SECTION "Main Data" and no loader was found writing them. But "in the saved block"
has never been sufficient on this project:

  * wTownVisitedFlag is in the saved block and the CURRENT TOWN's bit is re-set every Continue
    (probe_town_visited.py, 2026-07-17).
  * wStatusFlags3 is in the saved block and is ZEROED WHOLE on load.

...and, sharply: **AreaPlayer already marks bits in TWO OF THESE VERY BYTES as rewritten on load** --
0x29D4 b0 (strengthOutsideBattle, reset) and 0x29DA b5/b6 (battleEndedOrBlackout, usingLinkCable,
cleared on entry). If either byte were cleared WHOLE, it would take the three fishing rods, the
Saffron guards, Lapras and the starter with it.

PREDICTION: the rewrites are PER-BIT, not whole-byte, so all eight survive. ⚠️ That is exactly the
prediction a careful asm read got WRONG once already -- wMovementFlags clears bits per-routine, and
reading the source alone said whole-byte (2026-07-14, caught by probe_player_state.py). So the
console decides.

  A  baseline      untouched BaseSAV                     -> as shipped
  B  all eight ON  each of the 8 bits := 1               -> all kept?
  C  all eight OFF the control                           -> stay clear? (proves the loader sets none)
  D  bytes = 0xFF  0x29D4/0x29DA/0x29E0 := 0xFF          -> WHICH bits does the console clear?
                                                            This is the whole-byte question, and it
                                                            also re-proves AreaPlayer's rewrite marks
                                                            from the other direction.

D is the one that earns the probe: B passing tells you the eight survive, but only D tells you
whether they survive *because nothing touches the byte* or *because the clears are surgical*.

Local-only; needs the gitignored ROM. Run:
    tmp\emu-venv\Scripts\python.exe scripts\emu\probe_world_completed.py
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
OUT = REPO / "tmp" / "emu-completed"

# ── save offsets ────────────────────────────────────────────────────────────────────
# sMainData starts at 0x25A3 and maps wMainDataStart = $D2F7, so  wram = sav + 0xAD54.
SAV_STATUS_FLAGS_1 = 0x29D4        # wStatusFlags1   $D728
SAV_STATUS_FLAGS_4 = 0x29DA        # wStatusFlags4   $D72E
SAV_ELITE4_FLAGS = 0x29E0          # wElite4Flags    $D734

SAV_CHECKSUM = 0x3523
SAV_CHECKSUM_START = 0x2598
SAV_CHECKSUM_LEN = 0xF8B

# ── wram addresses ──────────────────────────────────────────────────────────────────
W_STATUS_FLAGS_1 = 0xD728
W_STATUS_FLAGS_4 = 0xD72E
W_ELITE4_FLAGS = 0xD734

W_CUR_MAP_WIDTH = 0xD369
W_CUR_MAP_HEIGHT = 0xD368
W_OVERWORLD_MAP = 0xC6E8
W_TILEMAP = 0xC3A0
SCREEN_TILES = 20 * 18

# (label, save offset, bit, wram addr) -- the eight WorldCompleted models.
FLAGS = [
    ("obtainedOldRod         (BIT_GOT_OLD_ROD)",              SAV_STATUS_FLAGS_1, 3, W_STATUS_FLAGS_1),
    ("obtainedGoodRod        (BIT_GOT_GOOD_ROD)",             SAV_STATUS_FLAGS_1, 4, W_STATUS_FLAGS_1),
    ("obtainedSuperRod       (BIT_GOT_SUPER_ROD)",            SAV_STATUS_FLAGS_1, 5, W_STATUS_FLAGS_1),
    ("satisfiedSaffronGuards (BIT_GAVE_SAFFRON_GUARDS_DRINK)", SAV_STATUS_FLAGS_1, 6, W_STATUS_FLAGS_1),
    ("obtainedLapras         (BIT_GOT_LAPRAS)",               SAV_STATUS_FLAGS_4, 0, W_STATUS_FLAGS_4),
    ("everHealedPokemon      (BIT_USED_POKECENTER)",          SAV_STATUS_FLAGS_4, 2, W_STATUS_FLAGS_4),
    ("obtainedStarterPokemon (BIT_GOT_STARTER)",              SAV_STATUS_FLAGS_4, 3, W_STATUS_FLAGS_4),
    ("startedElite4          (BIT_STARTED_ELITE_4)",          SAV_ELITE4_FLAGS,   1, W_ELITE4_FLAGS),
]

# Every bit of the three bytes, so D can report the whole picture. "!" marks the ones AreaPlayer
# already documents as rewritten on load -- D should reproduce those independently.
BYTE_MAP = {
    W_STATUS_FLAGS_1: ("wStatusFlags1", [
        "! strengthOutsideBattle (BIT_STRENGTH_ACTIVE)", "surfingAllowed (BIT_SURF_ALLOWED)",
        "- unused", "BIT_GOT_OLD_ROD", "BIT_GOT_GOOD_ROD", "BIT_GOT_SUPER_ROD",
        "BIT_GAVE_SAFFRON_GUARDS_DRINK", "BIT_UNUSED_CARD_KEY (dead)",
    ]),
    W_STATUS_FLAGS_4: ("wStatusFlags4", [
        "BIT_GOT_LAPRAS", "BIT_UNKNOWN_4_1", "BIT_USED_POKECENTER", "BIT_GOT_STARTER",
        "BIT_NO_BATTLES", "! BIT_BATTLE_OVER_OR_BLACKOUT", "! BIT_LINK_CONNECTED",
        "BIT_INIT_SCRIPTED_MOVEMENT",
    ]),
    W_ELITE4_FLAGS: ("wElite4Flags", [
        "BIT_UNUSED_BEAT_ELITE_4 (dead)", "BIT_STARTED_ELITE_4", "- unused", "- unused",
        "- unused", "- unused", "- unused", "- unused",
    ]),
}


def checksum(sav: bytearray) -> int:
    c = 0xFF
    for i in range(SAV_CHECKSUM_START, SAV_CHECKSUM_START + SAV_CHECKSUM_LEN):
        c = (c - sav[i]) & 0xFF
    return c


def sealed(sav: bytearray) -> bytes:
    sav[SAV_CHECKSUM] = checksum(sav)
    return bytes(sav)


def set_all(base: bytes, on: bool) -> bytes:
    """Set/clear exactly the eight modelled bits -- nothing else in the byte."""
    sav = bytearray(base)
    for _, off, bit, _w in FLAGS:
        if on:
            sav[off] |= (1 << bit)
        else:
            sav[off] &= ~(1 << bit) & 0xFF
    return sealed(sav)


def all_ff(base: bytes) -> bytes:
    """The whole-byte question: hand the console 0xFF and see what it gives back."""
    sav = bytearray(base)
    for off in (SAV_STATUS_FLAGS_1, SAV_STATUS_FLAGS_4, SAV_ELITE4_FLAGS):
        sav[off] = 0xFF
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
    r = {
        W_STATUS_FLAGS_1: mem[W_STATUS_FLAGS_1],
        W_STATUS_FLAGS_4: mem[W_STATUS_FLAGS_4],
        W_ELITE4_FLAGS: mem[W_ELITE4_FLAGS],
    }
    pyboy.screen.image.save(OUT / f"{label}.png")
    pyboy.stop(save=False)
    return r


def show(label: str, r: dict) -> None:
    print(f"\n  {label}")
    for w, (name, _bits) in BYTE_MAP.items():
        print(f"    {name:<16} 0x{r[w]:02X}")
    for name, _off, bit, w in FLAGS:
        print(f"      {'ON ' if (r[w] >> bit) & 1 else 'off'}  {name}")


def main() -> int:
    if not ROM.exists():
        print("SKIP: no ROM (local-only verification)")
        return 2

    OUT.mkdir(parents=True, exist_ok=True)
    rom = OUT / "rom.gb"
    shutil.copyfile(ROM, rom)
    base = bytes(BASE_SAV.read_bytes())

    a = run(rom, sealed(bytearray(base)), "A_baseline")
    b = run(rom, set_all(base, True), "B_all_eight_on")
    c = run(rom, set_all(base, False), "C_all_eight_off")
    d = run(rom, all_ff(base), "D_bytes_all_ff")

    if a:
        show("A  baseline (untouched BaseSAV)", a)
    if b:
        show("B  all eight SET in the save", b)
    if c:
        show("C  all eight CLEARED (control)", c)

    print("\n  VERDICT")
    ok = True

    if b:
        survived = [n for n, _o, bit, w in FLAGS if (b[w] >> bit) & 1]
        died = [n for n, _o, bit, w in FLAGS if not ((b[w] >> bit) & 1)]
        print(f"    All eight survive a Continue?   "
              f"{'YES -- ' + str(len(survived)) + '/8' if not died else 'NO -- these died:'}")
        for n in died:
            print(f"        DIED: {n}")
        ok &= not died

    if c:
        setByLoader = [n for n, _o, bit, w in FLAGS if (c[w] >> bit) & 1]
        print(f"    Control: cleared stay clear?    "
              f"{'YES -- the loader sets none of them' if not setByLoader else 'NO:'}")
        for n in setByLoader:
            print(f"        SET BY THE LOADER: {n}")
        ok &= not setByLoader

    if d:
        print(f"\n    D  the WHOLE-BYTE question -- handed 0xFF, the console gave back:")
        anyCleared = False
        for w, (name, bits) in BYTE_MAP.items():
            print(f"      {name:<16} 0x{d[w]:02X}" + ("   <- unchanged (0xFF)" if d[w] == 0xFF else ""))
            for bit in range(8):
                if not ((d[w] >> bit) & 1):
                    anyCleared = True
                    print(f"          bit {bit} CLEARED by the console: {bits[bit]}")
        if not anyCleared:
            print("      nothing cleared at all -- these bytes are not touched on this path.")
        print(f"\n    Are the bytes cleared WHOLE?    "
              f"{'NO -- clears are surgical (or absent). The eight are safe.' if all(d[w] != 0x00 for w in BYTE_MAP) else 'YES -- a byte was zeroed; the eight are NOT safe!'}")
        print("      (bits marked ! above are AreaPlayer's documented rewrites -- if they show up")
        print("       as cleared here, that is this probe independently reproducing them.)")

    print(f"\n    Matches the prediction in notes/reference/world-completed.md: "
          f"{'YES' if ok else 'NO -- the note must be corrected'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
