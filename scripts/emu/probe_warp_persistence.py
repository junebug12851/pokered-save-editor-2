"""Which warp bytes does the game KEEP from our save, and which does it REBUILD?

This is the question the warp editor stands on, and the disassembly's answer has two halves
that pull in opposite directions.

`LoadMapHeader` (home/overworld.asm) reloads the map's warp list from the ROM on every map
load -- unconditionally, with no escape-hatch bit (unlike the sprites, which get one):

    ld a, [hli]
    ld [wNumberOfWarps], a          ; <- ROM
    ...
    ld de, wWarpEntries             ; <- ROM, 4 bytes x N

...BUT it never gets that far on a Continue, because `LoadMainData` (engine/menus/save.asm)
does this the moment the save is read:

    ld hl, wCurMapTileset
    set BIT_NO_PREVIOUS_MAP, [hl]   ; <- bit 7

and `LoadMapHeader` opens with:

    bit BIT_NO_PREVIOUS_MAP, b
    ret nz                          ; <- RETURNS. No header. No warps. No signs. No sprites.

So the prediction is: **our warps are live on Continue, and rebuilt from ROM the moment the
player walks out of the map and back in.** That is exactly the sprite story, by a different
mechanism -- and it is far too load-bearing to take on trust from a careful read of asm,
which is precisely what was wrong last time (see notes/reference/sprites.md, Part 5).

So the console gets asked.

  A  baseline    untouched BaseSAV                          -> what the ROM says Pallet Town is
  B  tampered    warp 0 moved + re-aimed, a 4th warp added  -> does the console keep it?
  C  tampered    every warp STATE byte set to a marker      -> which ones survive the load?

Local-only; needs the gitignored ROM. Run: python scripts/emu/probe_warp_persistence.py
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
OUT = REPO / "tmp" / "emu-warps"

# ── save offsets ────────────────────────────────────────────────────────────────────
# sMainData starts at 0x25A3 and maps wMainDataStart = $D2F7, so
#     wram = sav + 0xAD54
SAV_LAST_MAP = 0x2611              # wLastMap                   $D365
SAV_CUR_MAP_TILESET = 0x2613       # wCurMapTileset             $D367  (bit 7 = NO_PREVIOUS_MAP)
SAV_NUM_WARPS = 0x265A             # wNumberOfWarps             $D3AE
SAV_WARP_ENTRIES = 0x265B          # wWarpEntries               $D3AF  (Y, X, warpID, mapID) x32
SAV_DEST_WARP_ID = 0x26DB          # wDestinationWarpID         $D42F
SAV_LAST_BLACKOUT_MAP = 0x29C5     # wLastBlackoutMap           $D719
SAV_DESTINATION_MAP = 0x29C6       # wDestinationMap            $D71A
SAV_DUNGEON_WARP_DEST = 0x29C9     # wDungeonWarpDestinationMap $D71D
SAV_WHICH_DUNGEON_WARP = 0x29CA    # wWhichDungeonWarp          $D71E
SAV_STATUS_FLAGS_3 = 0x29D9        # wStatusFlags3              $D72D
SAV_STATUS_FLAGS_6 = 0x29DE        # wStatusFlags6              $D732
SAV_STATUS_FLAGS_7 = 0x29DF        # wStatusFlags7              $D733
SAV_WARPED_FROM_WARP = 0x29E7      # wWarpedFromWhichWarp       $D73B
SAV_WARPED_FROM_MAP = 0x29E8       # wWarpedFromWhichMap        $D73C

SAV_CHECKSUM = 0x3523
SAV_CHECKSUM_START = 0x2598
SAV_CHECKSUM_LEN = 0xF8B

# ── wram addresses ──────────────────────────────────────────────────────────────────
W_LAST_MAP = 0xD365
W_CUR_MAP_TILESET = 0xD367
W_NUM_WARPS = 0xD3AE
W_WARP_ENTRIES = 0xD3AF
W_DEST_WARP_ID = 0xD42F
W_LAST_BLACKOUT_MAP = 0xD719
W_DESTINATION_MAP = 0xD71A
W_DUNGEON_WARP_DEST = 0xD71D
W_WHICH_DUNGEON_WARP = 0xD71E
W_STATUS_FLAGS_3 = 0xD72D
W_STATUS_FLAGS_6 = 0xD732
W_STATUS_FLAGS_7 = 0xD733
W_WARPED_FROM_WARP = 0xD73B
W_WARPED_FROM_MAP = 0xD73C

W_CUR_MAP_WIDTH = 0xD369
W_CUR_MAP_HEIGHT = 0xD368
W_OVERWORLD_MAP = 0xC6E8
W_TILEMAP = 0xC3A0
SCREEN_TILES = 20 * 18

# the state bytes, in the order we report them: (label, sav offset, wram addr)
STATE = [
    ("wLastMap                  ", SAV_LAST_MAP, W_LAST_MAP),
    ("wDestinationWarpID        ", SAV_DEST_WARP_ID, W_DEST_WARP_ID),
    ("wLastBlackoutMap          ", SAV_LAST_BLACKOUT_MAP, W_LAST_BLACKOUT_MAP),
    ("wDestinationMap           ", SAV_DESTINATION_MAP, W_DESTINATION_MAP),
    ("wDungeonWarpDestinationMap", SAV_DUNGEON_WARP_DEST, W_DUNGEON_WARP_DEST),
    ("wWhichDungeonWarp         ", SAV_WHICH_DUNGEON_WARP, W_WHICH_DUNGEON_WARP),
    ("wStatusFlags3             ", SAV_STATUS_FLAGS_3, W_STATUS_FLAGS_3),
    ("wStatusFlags6             ", SAV_STATUS_FLAGS_6, W_STATUS_FLAGS_6),
    ("wStatusFlags7             ", SAV_STATUS_FLAGS_7, W_STATUS_FLAGS_7),
    ("wWarpedFromWhichWarp      ", SAV_WARPED_FROM_WARP, W_WARPED_FROM_WARP),
    ("wWarpedFromWhichMap       ", SAV_WARPED_FROM_MAP, W_WARPED_FROM_MAP),
    ("wCurMapTileset            ", SAV_CUR_MAP_TILESET, W_CUR_MAP_TILESET),
]


def checksum(sav: bytearray) -> int:
    c = 0xFF
    for i in range(SAV_CHECKSUM_START, SAV_CHECKSUM_START + SAV_CHECKSUM_LEN):
        c = (c - sav[i]) & 0xFF
    return c


def sealed(sav: bytearray) -> bytes:
    sav[SAV_CHECKSUM] = checksum(sav)
    return bytes(sav)


def tamper_warps(base: bytes) -> bytes:
    """Move warp 0, re-aim it, and invent a 4th warp in a 3-warp town."""
    sav = bytearray(base)

    # warp 0: park it at (2, 2), aiming at warp 3 of map 0x2A (Viridian Forest)
    e = SAV_WARP_ENTRIES + 4 * 0
    sav[e + 0] = 2       # Y
    sav[e + 1] = 2       # X
    sav[e + 2] = 3       # destination warp id
    sav[e + 3] = 0x2A    # destination map

    # invent a 4th warp (Pallet Town has 3) at (9, 9) -> warp 0 of map 0x25
    e = SAV_WARP_ENTRIES + 4 * 3
    sav[e + 0] = 9
    sav[e + 1] = 9
    sav[e + 2] = 0
    sav[e + 3] = 0x25
    sav[SAV_NUM_WARPS] = 4

    return sealed(sav)


def tamper_state(base: bytes) -> bytes:
    """Stamp a distinct marker into every warp-STATE byte and see which ones come back."""
    sav = bytearray(tamper_warps(base))
    sav[SAV_LAST_MAP] = 0x0C                 # Route 1-ish marker
    sav[SAV_DEST_WARP_ID] = 0x02
    sav[SAV_LAST_BLACKOUT_MAP] = 0x03
    sav[SAV_DESTINATION_MAP] = 0x04
    sav[SAV_DUNGEON_WARP_DEST] = 0xC0        # SEAFOAM_ISLANDS_B1F is $C0? marker either way
    sav[SAV_WHICH_DUNGEON_WARP] = 0x01
    sav[SAV_WARPED_FROM_WARP] = 0x07
    sav[SAV_WARPED_FROM_MAP] = 0x08
    sav[SAV_STATUS_FLAGS_3] |= (1 << 4)      # BIT_ON_DUNGEON_WARP
    sav[SAV_STATUS_FLAGS_7] |= (1 << 2)      # BIT_FORCED_WARP
    return sealed(sav)


def tamper_flags3(base: bytes) -> bytes:
    """Set EVERY bit of wStatusFlags3. If the console gives back 0x00, the whole byte is
    being wiped -- which it is, because `wCableClubDestinationMap` is an ALIAS for
    `wStatusFlags3` and `SpecialEnterMap` (on the Continue path) does `xor a / ld
    [wCableClubDestinationMap], a`."""
    sav = bytearray(base)
    sav[SAV_STATUS_FLAGS_3] = 0xFF
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
    n = mem[W_NUM_WARPS]
    warps = []
    for i in range(min(n, 8) if n else 4):
        b = W_WARP_ENTRIES + 4 * i
        warps.append((mem[b], mem[b + 1], mem[b + 2], mem[b + 3]))
    return {
        "numWarps": n,
        "warps": warps,
        "state": {label: mem[addr] for label, _, addr in STATE},
    }


def show(label: str, r: dict) -> None:
    print(f"\n  {label}")
    print(f"    wNumberOfWarps  {r['numWarps']}")
    for i, (y, x, wid, mid) in enumerate(r["warps"]):
        print(f"      warp {i}   tile=({x:>2},{y:>2})   -> map 0x{mid:02X}, warp {wid}")
    for k, v in r["state"].items():
        print(f"    {k}  0x{v:02X}")


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
    base = bytearray(BASE_SAV.read_bytes())

    print("What we WROTE into the tampered saves:")
    print("      warp 0   tile=( 2, 2)   -> map 0x2A, warp 3")
    print("      warp 3   tile=( 9, 9)   -> map 0x25, warp 0   (invented -- wNumberOfWarps=4)")

    a = run(rom, sealed(bytearray(base)), "A_baseline")
    b = run(rom, tamper_warps(bytes(base)), "B_warps_tampered")
    c = run(rom, tamper_state(bytes(base)), "C_state_tampered")
    d = run(rom, tamper_flags3(bytes(base)), "D_statusflags3_all_ones")

    if a:
        show("A  baseline (untouched save)", a)
    if b:
        show("B  warp LIST tampered", b)
    if c:
        show("C  warp list + every STATE byte tampered", c)
    if d:
        print("\n  D  wStatusFlags3 = 0xFF  (is the WHOLE byte wiped, or just one bit?)")
        got = d["state"]["wStatusFlags3             "]
        print(f"    wrote 0xFF -> console has 0x{got:02X}   "
              f"{'WHOLE BYTE ZEROED' if got == 0 else 'only some bits cleared'}")
        print("    (wCableClubDestinationMap is an ALIAS for wStatusFlags3, and")
        print("     SpecialEnterMap does `xor a / ld [wCableClubDestinationMap], a`)")

    print("\n  VERDICT")
    if a and b:
        kept = (b["numWarps"] == 4 and b["warps"][0] == (2, 2, 3, 0x2A))
        print("    Does the console run on OUR warp list?   "
              f"{'YES -- the save wins on Continue' if kept else 'NO -- rebuilt from ROM'}")
    if c:
        print("\n    state bytes: does our value survive the load?")
        for label, sav_off, _ in STATE:
            want = tamper_state(bytes(base))[sav_off]
            got = c["state"][label]
            mark = "kept  " if want == got else "CHANGED"
            print(f"      {label}  wrote 0x{want:02X}  read 0x{got:02X}   {mark}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
