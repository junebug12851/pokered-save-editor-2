"""Does the game KEEP the sprite bytes we edit into a save -- or throw them away?

This is the question the whole sprite editor stands on, and the disassembly's answer is
alarming. `LoadMapHeader` (home/overworld.asm, `.loadSpriteData`) does this on EVERY map
load -- Continue included:

    ld a, [wStatusFlags4]
    bit BIT_BATTLE_OVER_OR_BLACKOUT, a
    jp nz, .finishUp          ; skip -- "battles don't destroy this data"
    ...                        ; else:
    ld [wNumSprites], a        ;   number of sprites <- ROM
    ; zero out sprite state data for sprites 01-15   <- ZEROES $F0 bytes of BOTH tables
    ; then reload pictureID / MAPY / MAPX / MOVEMENTBYTE1 from the ROM map header,
    ; and rebuild wMapSpriteData (movement byte 2, text id) + wMapSpriteExtraData.

If that reading is right, then every NPC byte an editor writes into sSpriteData /
wMapSpriteData / wMapSpriteExtraData / wNumSprites is erased the instant the map loads --
exactly like the sprite-set cache (see notes/reference/sprite-sets.md). And the escape
hatch is that one status bit: set wStatusFlags4 bit 5 and the console skips the reload and
runs on OUR bytes.

Nothing here is asserted from reading the asm. Three saves are built, the real ROM is
booted with each, and the console's own WRAM is read back after it reaches the overworld.

  A  baseline    untouched BaseSAV                     -> what the ROM says Pallet Town is
  B  tampered    sprite 1 moved + repictured, +1 NPC   -> does the console keep it?
  C  tampered + wStatusFlags4 bit 5 set                -> does the bit rescue it?

Local-only; needs the gitignored ROM. Run: python scripts/emu/probe_sprite_persistence.py
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
OUT = REPO / "tmp" / "emu-sprites"

# ── save offsets ────────────────────────────────────────────────────────────────────
# sMainData starts at 0x25A3 and maps wMainDataStart = $D2F7, so
#     wram = 0xD2F7 + (sav - 0x25A3)
SAV_NUM_SPRITES = 0x278D          # wNumSprites          $D4E1
SAV_MAP_SPRITE_DATA = 0x2790      # wMapSpriteData       $D4E4  (movement byte 2, text id) x16
SAV_MAP_SPRITE_EXTRA = 0x27B0     # wMapSpriteExtraData  $D504  (class/item, set id) x16
SAV_TOGGLE_LIST = 0x287A          # wToggleableObjectList$D5CE
SAV_STATUS_FLAGS_4 = 0x29DA       # wStatusFlags4        $D72E
SAV_SPRITE_DATA_1 = 0x2D2C        # sSpriteData -> wSpriteStateData1 $C100
SAV_SPRITE_DATA_2 = 0x2E2C        #              wSpriteStateData2 $C200

SAV_CHECKSUM = 0x3523
SAV_CHECKSUM_START = 0x2598
SAV_CHECKSUM_LEN = 0xF8B

BIT_BATTLE_OVER_OR_BLACKOUT = 5

# ── wram addresses ──────────────────────────────────────────────────────────────────
W_SPRITE_STATE_1 = 0xC100
W_SPRITE_STATE_2 = 0xC200
W_NUM_SPRITES = 0xD4E1
W_MAP_SPRITE_DATA = 0xD4E4
W_MAP_SPRITE_EXTRA = 0xD504
W_TOGGLE_LIST = 0xD5CE

W_CUR_MAP_WIDTH = 0xD369
W_CUR_MAP_HEIGHT = 0xD368
W_OVERWORLD_MAP = 0xC6E8
W_TILEMAP = 0xC3A0
SCREEN_TILES = 20 * 18

# spritestatedata1 fields
S1_PICTUREID = 0x0
S1_FACINGDIRECTION = 0x9
# spritestatedata2 fields
S2_MAPY = 0x4
S2_MAPX = 0x5
S2_MOVEMENTBYTE1 = 0x6
S2_GRASSPRIORITY = 0x7
S2_ORIGFACINGDIRECTION = 0x9


def checksum(sav: bytearray) -> int:
    c = 0xFF
    for i in range(SAV_CHECKSUM_START, SAV_CHECKSUM_START + SAV_CHECKSUM_LEN):
        c = (c - sav[i]) & 0xFF
    return c


def tamper(base: bytes, *, set_battle_bit: bool) -> bytes:
    """Move sprite 1, give it a different picture, and add a 4th NPC out of nowhere."""
    sav = bytearray(base)

    # sprite 1: picture -> 0x2E (a very different-looking sprite), and park it at (9, 9)
    sav[SAV_SPRITE_DATA_1 + 0x10 * 1 + S1_PICTUREID] = 0x2E
    sav[SAV_SPRITE_DATA_2 + 0x10 * 1 + S2_MAPY] = 4 + 9
    sav[SAV_SPRITE_DATA_2 + 0x10 * 1 + S2_MAPX] = 4 + 9
    sav[SAV_SPRITE_DATA_2 + 0x10 * 1 + S2_MOVEMENTBYTE1] = 0xFF  # STAY

    # invent a 4th NPC in slot 4 (Pallet Town normally has 3)
    sav[SAV_SPRITE_DATA_1 + 0x10 * 4 + S1_PICTUREID] = 0x05
    sav[SAV_SPRITE_DATA_2 + 0x10 * 4 + S2_MAPY] = 4 + 11
    sav[SAV_SPRITE_DATA_2 + 0x10 * 4 + S2_MAPX] = 4 + 6
    sav[SAV_SPRITE_DATA_2 + 0x10 * 4 + S2_MOVEMENTBYTE1] = 0xFF
    sav[SAV_NUM_SPRITES] = 4

    if set_battle_bit:
        sav[SAV_STATUS_FLAGS_4] |= 1 << BIT_BATTLE_OVER_OR_BLACKOUT

    sav[SAV_CHECKSUM] = checksum(sav)
    return bytes(sav)


def rebased(base: bytes) -> bytes:
    sav = bytearray(base)
    sav[SAV_CHECKSUM] = checksum(sav)
    return bytes(sav)


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
    n = mem[W_NUM_SPRITES]
    slots = []
    for i in range(1, 8):
        slots.append({
            "picture": mem[W_SPRITE_STATE_1 + 0x10 * i + S1_PICTUREID],
            "facing": mem[W_SPRITE_STATE_1 + 0x10 * i + S1_FACINGDIRECTION],
            "mapY": mem[W_SPRITE_STATE_2 + 0x10 * i + S2_MAPY],
            "mapX": mem[W_SPRITE_STATE_2 + 0x10 * i + S2_MAPX],
            "move1": mem[W_SPRITE_STATE_2 + 0x10 * i + S2_MOVEMENTBYTE1],
            "grass": mem[W_SPRITE_STATE_2 + 0x10 * i + S2_GRASSPRIORITY],
            "origFacing": mem[W_SPRITE_STATE_2 + 0x10 * i + S2_ORIGFACINGDIRECTION],
        })
    return {
        "numSprites": n,
        "slots": slots,
        "mapSpriteData": bytes(mem[W_MAP_SPRITE_DATA:W_MAP_SPRITE_DATA + 8]),
        "mapSpriteExtra": bytes(mem[W_MAP_SPRITE_EXTRA:W_MAP_SPRITE_EXTRA + 8]),
        "toggleList": bytes(mem[W_TOGGLE_LIST:W_TOGGLE_LIST + 6]),
    }


def show(label: str, r: dict) -> None:
    print(f"\n  {label}")
    print(f"    wNumSprites        {r['numSprites']}")
    for i, s in enumerate(r["slots"][:5], start=1):
        print(f"    slot {i}  picture=0x{s['picture']:02X}  "
              f"map=({s['mapX'] - 4:>3},{s['mapY'] - 4:>3})  move1=0x{s['move1']:02X}  "
              f"facing=0x{s['facing']:02X}  origFacing=0x{s['origFacing']:02X}  "
              f"grass=0x{s['grass']:02X}")
    print(f"    wMapSpriteData     {' '.join(f'{b:02X}' for b in r['mapSpriteData'])}")
    print(f"    wMapSpriteExtra    {' '.join(f'{b:02X}' for b in r['mapSpriteExtra'])}")
    print(f"    wToggleableObjList {' '.join(f'{b:02X}' for b in r['toggleList'])}")


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
    base = BASE_SAV.read_bytes()

    print("What we WROTE into the tampered saves:")
    print("    slot 1  picture=0x2E  map=(  9,  9)  move1=0xFF")
    print("    slot 4  picture=0x05  map=(  6, 11)  move1=0xFF   (invented -- wNumSprites=4)")

    a = run(rom, rebased(base), "A_baseline")
    b = run(rom, tamper(base, set_battle_bit=False), "B_tampered")
    c = run(rom, tamper(base, set_battle_bit=True), "C_tampered_battlebit")

    if a:
        show("A  baseline (untouched save)", a)
    if b:
        show("B  tampered, status bit CLEAR", b)
    if c:
        show("C  tampered, wStatusFlags4 bit 5 SET", c)

    print("\n  VERDICT")
    if a and b:
        kept = (b["slots"][0]["picture"] == 0x2E and b["numSprites"] == 4)
        print(f"    B == A (edits erased)?          {'no -- edits SURVIVED' if kept else 'YES -- the console THREW OUR EDITS AWAY'}")
    if a and c:
        kept = (c["slots"][0]["picture"] == 0x2E and c["numSprites"] == 4)
        print(f"    C keeps our edits?              {'YES -- the status bit rescues them' if kept else 'no -- erased even with the bit'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
