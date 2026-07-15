"""Which of the nine CHARACTER-STATE flags (v1's "NPC" page) does the game KEEP from our
save on Continue, and which does it REWRITE the instant the save loads?

This is the question the right-hand Character panel stands on (map-screen.md Phase 9). A
careful read of the Continue path predicts the answers below -- and a careful read has been
WRONG before (the sprite persistence pass, notes/reference/sprites.md Part 5), so the console
gets asked. Companion note: notes/reference/npc-character-state.md.

The nine, with their real WRAM homes (wram = sav + 0xAD54):

  wStatusFlags3 (0x29D9)  b0 initTradeCenterFacing   BIT_INIT_TRADE_CENTER_FACING
                          b5 npcsDoNotFacePlayer      BIT_NO_NPC_FACE_PLAYER
        -> WHOLE BYTE zeroed on Continue: SpecialEnterMap does
           `ld [wCableClubDestinationMap], a` (a=0), and that label aliases wStatusFlags3.
           (Already seen from the other side by probe_player_state.py -- confirmed here too.)
  wStatusFlags4 (0x29DA)  b7 initScriptedMovement     BIT_INIT_SCRIPTED_MOVEMENT
  wStatusFlags5 (0x29DC)  b0 scriptedNpcMoving        BIT_SCRIPTED_NPC_MOVEMENT
                          b5 disableJoypad            BIT_DISABLE_JOYPAD
                          b7 scriptedMovementActive   BIT_SCRIPTED_MOVEMENT_STATE
  wStatusFlags7 (0x29DF)  b0 testBattle               BIT_TEST_BATTLE (debug)
                          b3 trainerBattle            BIT_TRAINER_BATTLE
  wTrainerHeaderPtr (0x2CDC, word LE)  the pending trainer's header pointer

The predictions from the source (to be confirmed / overturned):
  * SF3 b0,b5           -> 0 (whole-byte wipe).
  * everything else     -> transient script/battle scratch. Whether the raw bit/word SURVIVES
                          the load, and whether landing in the overworld CLEARS it, is exactly
                          what a source-read can't settle. wStatusFlags7 is NOT whole-wiped
                          (b1 NO_MAP_MUSIC / b7 USED_FLY persist), so testBattle/trainerBattle
                          may read back as written -- inert, but present.

Local-only; needs the gitignored ROM. Run:
  tmp/emu-venv/Scripts/python.exe scripts/emu/probe_npc_character_state.py
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
OUT = REPO / "tmp" / "emu-npc"

# sMainData starts at 0x25A3 -> wMainDataStart $D2F7, so  wram = sav + 0xAD54
DELTA = 0xAD54

SF3 = 0x29D9   # wStatusFlags3
SF4 = 0x29DA   # wStatusFlags4
SF5 = 0x29DC   # wStatusFlags5
SF7 = 0x29DF   # wStatusFlags7
TRAINER_PTR = 0x2CDC  # wTrainerHeaderPtr (word, little-endian)

SAV_CHECKSUM = 0x3523
SAV_CHECKSUM_START = 0x2598
SAV_CHECKSUM_LEN = 0xF8B

# (label, sav offset, bit)
BITS = [
    ("SF3.b0 initTradeCenterFacing  ", SF3, 0),
    ("SF3.b5 npcsDoNotFacePlayer     ", SF3, 5),
    ("SF4.b7 initScriptedMovement    ", SF4, 7),
    ("SF5.b0 scriptedNpcMoving       ", SF5, 0),
    ("SF5.b5 disableJoypad           ", SF5, 5),
    ("SF5.b7 scriptedMovementActive  ", SF5, 7),
    ("SF7.b0 testBattle (debug)      ", SF7, 0),
    ("SF7.b3 trainerBattle           ", SF7, 3),
]

SENTINEL_PTR = 0x5AA5  # a distinctive, harmless value for wTrainerHeaderPtr


def checksum(sav: bytearray) -> int:
    c = 0xFF
    for i in range(SAV_CHECKSUM_START, SAV_CHECKSUM_START + SAV_CHECKSUM_LEN):
        c = (c - sav[i]) & 0xFF
    return c


def sealed(sav: bytearray) -> bytes:
    sav[SAV_CHECKSUM] = checksum(sav)
    return bytes(sav)


def tamper(base: bytes) -> bytes:
    sav = bytearray(base)
    # every reportable bit ON
    for _, off, bit in BITS:
        sav[off] |= (1 << bit) & 0xFF
    # a distinctive trainer-header pointer
    sav[TRAINER_PTR] = SENTINEL_PTR & 0xFF
    sav[TRAINER_PTR + 1] = (SENTINEL_PTR >> 8) & 0xFF
    return sealed(sav)


# boot to overworld, reused verbatim from probe_player_state.py
W_CUR_MAP_WIDTH = 0xD369
W_CUR_MAP_HEIGHT = 0xD368
W_OVERWORLD_MAP = 0xC6E8
W_TILEMAP = 0xC3A0
SCREEN_TILES = 20 * 18


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
    r = {("bit", off, bit): (mem[off + DELTA] >> bit) & 1 for _, off, bit in BITS}
    lo = mem[TRAINER_PTR + DELTA]
    hi = mem[TRAINER_PTR + 1 + DELTA]
    r["ptr"] = lo | (hi << 8)
    return r


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


def main() -> int:
    if not ROM.exists():
        print("SKIP: no ROM (local-only verification)")
        return 2
    OUT.mkdir(parents=True, exist_ok=True)
    rom = OUT / "rom.gb"
    shutil.copyfile(ROM, rom)
    base = bytes(BASE_SAV.read_bytes())

    sav = tamper(base)
    got = run(rom, sav, "npc_all_bits_set")
    if not got:
        return 1

    print("\n===== Character-state flags on Continue (all stamped to 1) =====")
    for lab, off, bit in BITS:
        w = (sav[off] >> bit) & 1
        g = got[("bit", off, bit)]
        mark = "kept   " if w == g else "REWRITE"
        print(f"  {lab} wrote {w}  read {g}   {mark}")
    pw = SENTINEL_PTR
    pg = got["ptr"]
    mark = "kept   " if pw == pg else "REWRITE"
    print(f"  wTrainerHeaderPtr              wrote 0x{pw:04X}  read 0x{pg:04X}   {mark}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
