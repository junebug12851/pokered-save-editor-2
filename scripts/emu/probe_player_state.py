"""Which PLAYER / map-state bytes does the game KEEP from our save on Continue, and
which does it REWRITE the instant the save loads?

This is the question the fleshed-out Player details panel stands on. A careful read of
the Continue path (engine/menus/main_menu.asm -> SpecialEnterMap -> home/overworld.asm
EnterMap) predicts several rewrites -- and a careful read has been WRONG before (the
sprite persistence pass, notes/reference/sprites.md Part 5), so the console gets asked.

The predictions, from the source:

  * wPlayerDirection (playerCurDir)      -> FORCED to PLAYER_DIR_DOWN on every Continue
        main_menu.asm `.pressedA`:  ld a, PLAYER_DIR_DOWN / ld [wPlayerDirection], a
  * wStatusFlags3 (isBattle, isTrainerBattle, ...)  -> WHOLE BYTE zeroed
        SpecialEnterMap: `ld [wCableClubDestinationMap], a` (a=0); it aliases wStatusFlags3
  * wStatusFlags1 BIT_STRENGTH_ACTIVE (strengthOutsideBattle)  -> reset to 0 on a normal
        Continue (EnterMap calls ResetUsingStrengthOutOfBattleBit when the battle-over
        bit is CLEAR) -- but NOT reset when the battle-over bit is set (after-battle path).
  * wStatusFlags4 BIT_BATTLE_OVER_OR_BLACKOUT (battleEndedOrBlackout)  -> always `res` on
        map entry.
  * coords / blockcoords / walkBikeSurf / lastStopDir  -> kept (genuine persistent state).
  * the *_OffsetSinceLastSpecialWarp bytes and BIT_UNUSED_CARD_KEY -> kept but DEAD
        (written by the game, never read).

Two tampered saves are booted:
  N  battle-over bit CLEAR   -> the ordinary Continue: strength should be reset
  B  battle-over bit SET     -> the after-battle path: strength should SURVIVE, and
                               battle-over itself should still be cleared

Local-only; needs the gitignored ROM. Run: python scripts/emu/probe_player_state.py
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
OUT = REPO / "tmp" / "emu-player"

# sMainData starts at 0x25A3 -> wMainDataStart $D2F7, so  wram = sav + 0xAD54
DELTA = 0xAD54

# ── the player-state bytes: (label, sav offset, bit or None) ────────────────────────
YCOORD = 0x260D
XCOORD = 0x260E
YBLOCK = 0x260F
XBLOCK = 0x2610
YOFF = 0x278E                # wYOffsetSinceLastSpecialWarp  (dead)
XOFF = 0x278F                # wXOffsetSinceLastSpecialWarp  (dead)
MOVEDIR = 0x27D4             # wPlayerMovingDirection
LASTSTOP = 0x27D5            # wPlayerLastStopDirection
CURDIR = 0x27D6             # wPlayerDirection   (forced DOWN on Continue)
WALKBIKESURF = 0x29AC       # wWalkBikeSurfState
JUMPY = 0x29C0              # wPlayerJumpingYScreenCoordsIndex
SF1 = 0x29D4               # wStatusFlags1  (b0 strength, b1 surf, b7 unused card key)
SF3 = 0x29D9               # wStatusFlags3  (b6 talked-to-trainer, b7 print-end-battle)
SF4 = 0x29DA               # wStatusFlags4  (b4 no-battles, b5 battle-over, b6 link)
SF7 = 0x29DF               # wStatusFlags7  (b7 used-fly)
MOVE = 0x29E2              # wMovementFlags (b0 door, b1 exiting, b2 warp, b6 ledge, b7 spin)

SAV_CHECKSUM = 0x3523
SAV_CHECKSUM_START = 0x2598
SAV_CHECKSUM_LEN = 0xF8B

# byte fields to report whole
BYTES = [
    ("wYCoord                        ", YCOORD),
    ("wXCoord                        ", XCOORD),
    ("wYBlockCoord                   ", YBLOCK),
    ("wXBlockCoord                   ", XBLOCK),
    ("wYOffsetSinceLastSpecialWarp   ", YOFF),
    ("wXOffsetSinceLastSpecialWarp   ", XOFF),
    ("wPlayerMovingDirection         ", MOVEDIR),
    ("wPlayerLastStopDirection       ", LASTSTOP),
    ("wPlayerDirection (curDir)      ", CURDIR),
    ("wWalkBikeSurfState             ", WALKBIKESURF),
    ("wPlayerJumpingYScreenCoordsIdx ", JUMPY),
]

# bit fields to report per bit: (label, sav offset, bit)
BITS = [
    ("SF1.b0 strengthOutsideBattle   ", SF1, 0),
    ("SF1.b1 surfingAllowed          ", SF1, 1),
    ("SF1.b7 usedCardKey (dead)      ", SF1, 7),
    ("SF3.b6 isBattle                ", SF3, 6),
    ("SF3.b7 isTrainerBattle         ", SF3, 7),
    ("SF4.b4 noBattles               ", SF4, 4),
    ("SF4.b5 battleEndedOrBlackout   ", SF4, 5),
    ("SF4.b6 usingLinkCable          ", SF4, 6),
    ("SF7.b7 flyOutofBattle          ", SF7, 7),
    ("MOVE.b0 standingOnDoor         ", MOVE, 0),
    ("MOVE.b1 movingThroughDoor      ", MOVE, 1),
    ("MOVE.b2 standingOnWarp         ", MOVE, 2),
    ("MOVE.b6 finalLedgeJumping      ", MOVE, 6),
    ("MOVE.b7 spinPlayer             ", MOVE, 7),
]


def checksum(sav: bytearray) -> int:
    c = 0xFF
    for i in range(SAV_CHECKSUM_START, SAV_CHECKSUM_START + SAV_CHECKSUM_LEN):
        c = (c - sav[i]) & 0xFF
    return c


def sealed(sav: bytearray) -> bytes:
    sav[SAV_CHECKSUM] = checksum(sav)
    return bytes(sav)


def tamper(base: bytes, battle_over: bool) -> bytes:
    sav = bytearray(base)
    # coords: distinct, in-bounds for Pallet Town (18 tiles tall, 20 wide)
    sav[YCOORD] = 5
    sav[XCOORD] = 5
    sav[YBLOCK] = 1
    sav[XBLOCK] = 0
    sav[YOFF] = 0x11
    sav[XOFF] = 0x12
    sav[MOVEDIR] = 0x00        # standing still (any real save is 0 here)
    sav[LASTSTOP] = 0x04       # PLAYER_DIR_LEFT marker
    sav[CURDIR] = 0x08         # PLAYER_DIR_UP marker  -> expect console = DOWN
    sav[WALKBIKESURF] = 0x01   # biking  -> Pallet doesn't force, should survive
    sav[JUMPY] = 0x33
    # SF1: strength + surf + card key all on
    sav[SF1] |= (1 << 0) | (1 << 1) | (1 << 7)
    # SF3: talked-to-trainer + print-end-battle on
    sav[SF3] |= (1 << 6) | (1 << 7)
    # SF4: no-battles + link on; battle-over per variant
    sav[SF4] |= (1 << 4) | (1 << 6)
    if battle_over:
        sav[SF4] |= (1 << 5)
    else:
        sav[SF4] &= ~(1 << 5) & 0xFF
    # SF7: used-fly on
    sav[SF7] |= (1 << 7)
    # MOVE: every reportable bit on
    sav[MOVE] |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 6) | (1 << 7)
    return sealed(sav)


# ── boot to overworld, reused from probe_warp_persistence.py ─────────────────────────
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
    return {off: mem[off + DELTA] for _, off in BYTES} | {
        ("bit", off, bit): (mem[off + DELTA] >> bit) & 1 for _, off, bit in BITS
    }


def run(rom: Path, sav: bytes, label: str) -> dict | None:
    from pyboy import PyBoy
    # inject the RAM save PyBoy loads alongside the ROM (must exist before construction)
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

    n_sav = tamper(base, battle_over=False)
    b_sav = tamper(base, battle_over=True)
    n = run(rom, n_sav, "N_battle_over_clear")
    b = run(rom, b_sav, "B_battle_over_set")

    def report(label: str, wrote: bytes, got: dict) -> None:
        print(f"\n===== {label} =====")
        print("  byte fields:")
        for lab, off in BYTES:
            w, g = wrote[off], got[off]
            mark = "kept   " if w == g else "REWRITE"
            print(f"    {lab} wrote 0x{w:02X}  read 0x{g:02X}   {mark}")
        print("  bit fields:")
        for lab, off, bit in BITS:
            w = (wrote[off] >> bit) & 1
            g = got[("bit", off, bit)]
            mark = "kept   " if w == g else "REWRITE"
            print(f"    {lab} wrote {w}  read {g}   {mark}")

    if n:
        report("N  ordinary Continue (battle-over CLEAR)", n_sav, n)
    if b:
        report("B  after-battle Continue (battle-over SET)", b_sav, b)
    return 0


if __name__ == "__main__":
    sys.exit(main())
