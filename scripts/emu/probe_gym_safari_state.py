r"""Do the six gym/safari minigame bytes SURVIVE a Continue -- and are our addresses exact?

Twilight briefed a "Map Storage" panel for six save bytes (see
`notes/reference/gym-safari-state.md`). They are NOT map/Area state and NOT per-map: they live in
the global Main Data block, one each save-wide, and this IS their persistent copy. The load-bearing
claim for a save editor is simply: *an edit to any of them is still there on the next Continue* (so
editing does something). No load-time routine is known to wipe them (unlike wStatusFlags3, which
shares a byte with a cleared variable), so the prediction is: all six read back exactly as written.

A careful asm read has been wrong before (sprites, 2026-07-13), so the console gets asked.

  A  baseline    untouched BaseSAV                     -> resting values
  B  written     six distinctive values, checksum re-sealed -> read back identical?

Distinctive values (chosen not to collide with resting state):
  wFirstLockTrashCanIndex   0x29EF -> $D743 := 0x0A
  wSecondLockTrashCanIndex  0x29F0 -> $D744 := 0x0C
  wOpponentAfterWrongAnswer 0x2CE4 -> $DA38 := 0x07
  wSafariSteps (BIG-endian) 0x29B9 -> $D70D := 0x01 , 0x29BA -> $D70E := 0xF6   (== 502)
  wSafariZoneGameOver       0x2CF2 -> $DA46 := 0x01
  wNumSafariBalls           0x2CF3 -> $DA47 := 0x1E   (== 30)

This ALSO confirms the exact WRAM addresses and the wSafariSteps big-endian byte order: if any
address were off by a byte, the read-back would not match.

NOT covered here (script-driven, needs an in-zone / mid-puzzle save + real movement, not a load):
  * steps/balls re-set to 502/30 by the Safari Zone gate on entry  (scripts/SafariZoneGate.asm)
  * wSafariZoneGameOver rewritten every step by SafariZoneCheck     (engine/.../safari_game.asm)
  * first trash can re-randomised by the Vermilion gym script       (scripts/VermilionCity.asm)
  * wOpponentAfterWrongAnswer zeroed by CinnabarGymResetScripts     (scripts/CinnabarGym.asm)
These are read straight from the disassembly and define the "armed window"; they are UX facts about
*when* an edit matters, not the persistence claim this probe pins.

BaseSAV is in Pallet Town (not the zone / a gym), so nothing consults or rewrites these on boot --
exactly why it is a clean persistence test.

Local-only; needs the gitignored ROM. Run:
    tmp\emu-venv\Scripts\python.exe scripts\emu\probe_gym_safari_state.py
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
OUT = REPO / "tmp" / "emu-gym-safari"

# ── save offsets (sMainData 0x25A3 -> wMainDataStart $D2F7, so wram = sav + 0xAD54) ──────
SAV_FIRST_TRASH = 0x29EF       # wFirstLockTrashCanIndex    $D743
SAV_SECOND_TRASH = 0x29F0      # wSecondLockTrashCanIndex   $D744
SAV_OPP_WRONG = 0x2CE4         # wOpponentAfterWrongAnswer  $DA38
SAV_SAFARI_STEPS = 0x29B9      # wSafariSteps (dw, BIG-endian: HIGH then LOW)  $D70D
SAV_SAFARI_GAMEOVER = 0x2CF2   # wSafariZoneGameOver        $DA46
SAV_SAFARI_BALLS = 0x2CF3      # wNumSafariBalls            $DA47

SAV_CHECKSUM = 0x3523
SAV_CHECKSUM_START = 0x2598
SAV_CHECKSUM_LEN = 0xF8B

# ── wram addresses (all in the saved Main Data block) ───────────────────────────────────
W_FIRST_TRASH = 0xD743
W_SECOND_TRASH = 0xD744
W_OPP_WRONG = 0xDA38
W_SAFARI_STEPS = 0xD70D         # HIGH byte; LOW at +1
W_SAFARI_GAMEOVER = 0xDA46
W_SAFARI_BALLS = 0xDA47

W_CUR_MAP_WIDTH = 0xD369
W_CUR_MAP_HEIGHT = 0xD368
W_OVERWORLD_MAP = 0xC6E8
W_TILEMAP = 0xC3A0
SCREEN_TILES = 20 * 18

# The six writes, as (save-offset, value) pairs. Steps is two bytes, big-endian == 502.
WRITES = [
    (SAV_FIRST_TRASH, 0x0A),
    (SAV_SECOND_TRASH, 0x0C),
    (SAV_OPP_WRONG, 0x07),
    (SAV_SAFARI_STEPS, 0x01),       # HIGH(502)
    (SAV_SAFARI_STEPS + 1, 0xF6),   # LOW(502)
    (SAV_SAFARI_GAMEOVER, 0x01),
    (SAV_SAFARI_BALLS, 0x1E),
]


def checksum(sav: bytearray) -> int:
    c = 0xFF
    for i in range(SAV_CHECKSUM_START, SAV_CHECKSUM_START + SAV_CHECKSUM_LEN):
        c = (c - sav[i]) & 0xFF
    return c


def sealed(sav: bytearray) -> bytes:
    sav[SAV_CHECKSUM] = checksum(sav)
    return bytes(sav)


def written(base: bytes) -> bytes:
    sav = bytearray(base)
    for off, val in WRITES:
        sav[off] = val
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


def read_console(pyboy) -> dict:
    mem = pyboy.memory
    return {
        "firstTrash": mem[W_FIRST_TRASH],
        "secondTrash": mem[W_SECOND_TRASH],
        "oppWrong": mem[W_OPP_WRONG],
        "safariStepsHi": mem[W_SAFARI_STEPS],
        "safariStepsLo": mem[W_SAFARI_STEPS + 1],
        "safariSteps": (mem[W_SAFARI_STEPS] << 8) | mem[W_SAFARI_STEPS + 1],  # big-endian
        "safariGameOver": mem[W_SAFARI_GAMEOVER],
        "safariBalls": mem[W_SAFARI_BALLS],
    }


def run(rom: Path, sav: bytes, label: str) -> dict | None:
    from pyboy import PyBoy
    (OUT / "rom.gb.ram").write_bytes(sav)
    pyboy = PyBoy(str(rom), window="null", sound_emulated=False)
    if not boot(pyboy):
        pyboy.stop(save=False)
        print(f"  {label}: never reached the overworld")
        return None
    r = read_console(pyboy)
    pyboy.screen.image.save(OUT / f"{label}.png")
    pyboy.stop(save=False)
    return r


def show(label: str, r: dict) -> None:
    print(f"\n  {label}")
    print(f"    wFirstLockTrashCanIndex    0x{r['firstTrash']:02X}")
    print(f"    wSecondLockTrashCanIndex   0x{r['secondTrash']:02X}")
    print(f"    wOpponentAfterWrongAnswer  0x{r['oppWrong']:02X}")
    print(f"    wSafariSteps               {r['safariSteps']}  "
          f"(hi 0x{r['safariStepsHi']:02X}, lo 0x{r['safariStepsLo']:02X})")
    print(f"    wSafariZoneGameOver        0x{r['safariGameOver']:02X}")
    print(f"    wNumSafariBalls            {r['safariBalls']}")


def main() -> int:
    if not ROM.exists():
        print("SKIP: no ROM (local-only verification)")
        return 2

    OUT.mkdir(parents=True, exist_ok=True)
    rom = OUT / "rom.gb"
    shutil.copyfile(ROM, rom)
    base = bytes(BASE_SAV.read_bytes())

    a = run(rom, sealed(bytearray(base)), "A_baseline")
    b = run(rom, written(base), "B_written")

    if a:
        show("A  baseline (untouched save)", a)
    if b:
        show("B  six values written into the save", b)

    print("\n  VERDICT (do the written values survive Continue?)")
    if b:
        want = {
            "wFirstLockTrashCanIndex": (b["firstTrash"], 0x0A),
            "wSecondLockTrashCanIndex": (b["secondTrash"], 0x0C),
            "wOpponentAfterWrongAnswer": (b["oppWrong"], 0x07),
            "wSafariSteps (big-endian == 502)": (b["safariSteps"], 502),
            "wSafariZoneGameOver": (b["safariGameOver"], 0x01),
            "wNumSafariBalls": (b["safariBalls"], 30),
        }
        all_ok = True
        for name, (got, exp) in want.items():
            ok = got == exp
            all_ok &= ok
            print(f"    {name:34s} {'OK' if ok else 'MISMATCH'}  (got {got}, want {exp})")
        print(f"\n    ALL SIX SURVIVE + ADDRESSES EXACT: {'YES' if all_ok else 'NO'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
