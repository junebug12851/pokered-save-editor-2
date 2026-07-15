r"""Does the "wild-encounter cooldown" flag survive a Continue -- and does it really grant 3 steps?

The save byte the editor calls `pauseMons3Steps` is `wStatusFlags2` bit 0 (save 0x29D8 -> $D72C),
whose real name in `pret/pokered` is **`BIT_WILD_ENCOUNTER_COOLDOWN`**. The disassembly says:

  * `end_of_battle.asm`      : `set BIT_WILD_ENCOUNTER_COOLDOWN, [hl]`  -- set after EVERY battle.
  * `home/overworld.asm`  EnterMap:
        bit BIT_WILD_ENCOUNTER_COOLDOWN, [hl]
        jr z, .skipGivingThreeStepsOfNoRandomBattles
        ld a, 3 ; minimum number of steps between battles
        ld [wNumberOfNoRandomBattleStepsLeft], a
  * `home/overworld.asm`  step-count: each completed step, if the bit is set, decrement
        `wNumberOfNoRandomBattleStepsLeft`; when it hits 0, `res` the bit.

So the PREDICTION for a save editor: setting the bit means "on the next Continue, EnterMap runs,
sees it, and gives you 3 encounter-free steps". The Continue path is `SpecialEnterMap` -> (falls
through to) `EnterMap`, and `SpecialEnterMap` clears `wCableClubDestinationMap`/`wStatusFlags3` but
NOT `wStatusFlags2` -- so the loaded bit should reach EnterMap intact. `ClearVariablesOnEnterMap`
does not touch `wStatusFlags2` either.

That is a load-bearing persistence claim, and a careful asm read has been wrong before
(sprites, 2026-07-13). So the console gets asked.

  A  baseline    untouched BaseSAV            -> flag + steps-left as the ROM leaves them
  B  cooldown ON   wStatusFlags2 bit0 := 1    -> flag kept? steps-left == 3?
  C  cooldown OFF  wStatusFlags2 bit0 := 0    -> control: steps-left should be 0

Read right after landing in the overworld, before any movement (the boot presses only start/a),
so nothing has been decremented yet.

Local-only; needs the gitignored ROM. Run:
    tmp\emu-venv\Scripts\python.exe scripts\emu\probe_wild_encounter_cooldown.py
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
OUT = REPO / "tmp" / "emu-cooldown"

# ── save offsets ────────────────────────────────────────────────────────────────────
# sMainData starts at 0x25A3 and maps wMainDataStart = $D2F7, so  wram = sav + 0xAD54.
SAV_STATUS_FLAGS_2 = 0x29D8       # wStatusFlags2              $D72C  (bit 0 = WILD_ENCOUNTER_COOLDOWN)
BIT_WILD_ENCOUNTER_COOLDOWN = 0

SAV_CHECKSUM = 0x3523
SAV_CHECKSUM_START = 0x2598
SAV_CHECKSUM_LEN = 0xF8B

# ── wram addresses ──────────────────────────────────────────────────────────────────
W_STATUS_FLAGS_2 = 0xD72C          # in the saved Main Data block (0x29D8 + 0xAD54)
# These two live BELOW wMainDataStart, so they are NOT in the save -- read live only.
# $D13B/$D13C are the well-known pokered addresses; the 0-vs-3 contrast below re-proves it.
W_STEP_COUNTER = 0xD13B            # wStepCounter
W_NO_RANDOM_STEPS = 0xD13C         # wNumberOfNoRandomBattleStepsLeft

W_CUR_MAP_WIDTH = 0xD369
W_CUR_MAP_HEIGHT = 0xD368
W_OVERWORLD_MAP = 0xC6E8
W_TILEMAP = 0xC3A0
SCREEN_TILES = 20 * 18


def checksum(sav: bytearray) -> int:
    c = 0xFF
    for i in range(SAV_CHECKSUM_START, SAV_CHECKSUM_START + SAV_CHECKSUM_LEN):
        c = (c - sav[i]) & 0xFF
    return c


def sealed(sav: bytearray) -> bytes:
    sav[SAV_CHECKSUM] = checksum(sav)
    return bytes(sav)


def set_cooldown(base: bytes, on: bool) -> bytes:
    sav = bytearray(base)
    if on:
        sav[SAV_STATUS_FLAGS_2] |= (1 << BIT_WILD_ENCOUNTER_COOLDOWN)
    else:
        sav[SAV_STATUS_FLAGS_2] &= ~(1 << BIT_WILD_ENCOUNTER_COOLDOWN) & 0xFF
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
    sf2 = mem[W_STATUS_FLAGS_2]
    return {
        "sf2": sf2,
        "cooldownBit": (sf2 >> BIT_WILD_ENCOUNTER_COOLDOWN) & 1,
        "stepsLeft": mem[W_NO_RANDOM_STEPS],
        "stepCounter": mem[W_STEP_COUNTER],
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
    print(f"    wStatusFlags2                 0x{r['sf2']:02X}")
    print(f"    BIT_WILD_ENCOUNTER_COOLDOWN   {r['cooldownBit']}")
    print(f"    wNumberOfNoRandomBattleStepsLeft  {r['stepsLeft']}")
    print(f"    wStepCounter                  {r['stepCounter']}")


def main() -> int:
    if not ROM.exists():
        print("SKIP: no ROM (local-only verification)")
        return 2

    OUT.mkdir(parents=True, exist_ok=True)
    rom = OUT / "rom.gb"
    shutil.copyfile(ROM, rom)
    base = bytes(BASE_SAV.read_bytes())

    a = run(rom, sealed(bytearray(base)), "A_baseline")
    b = run(rom, set_cooldown(base, True), "B_cooldown_on")
    c = run(rom, set_cooldown(base, False), "C_cooldown_off")

    if a:
        show("A  baseline (untouched save)", a)
    if b:
        show("B  cooldown bit SET in the save", b)
    if c:
        show("C  cooldown bit CLEARED in the save (control)", c)

    print("\n  VERDICT")
    if b:
        kept = b["cooldownBit"] == 1
        granted = b["stepsLeft"] == 3
        print(f"    Flag survives Continue?              "
              f"{'YES -- read back set' if kept else 'NO -- read back clear'}")
        print(f"    EnterMap granted 3 no-battle steps?  "
              f"{'YES -- wNumberOfNoRandomBattleStepsLeft == 3' if granted else 'NO ('+str(b['stepsLeft'])+')'}")
    if c:
        print(f"    Control (bit clear) steps-left == 0? "
              f"{'YES' if c['stepsLeft'] == 0 else 'NO ('+str(c['stepsLeft'])+')'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
