"""Prove our palette ("contrast") maths against the real Game Boy -- all 10 values.

What Twilight calls CONTRAST is `wMapPalOffset` (save 0x2609). The game does something
delightfully unsafe with it (home/fade.asm):

    LoadGBPal:
        b  = wMapPalOffset
        hl = FadePal4 - b          ; SUBTRACT it from a pointer INTO a table
        rBGP  = [hl+]
        rOBP0 = [hl+]
        rOBP1 = [hl+]

The fade table is eight contiguous 3-byte palettes (FadePal1..FadePal8). Subtracting a
multiple of 3 lands on a real entry -- 0, 3, 6, 9 give FadePal4, 3, 2, 1: the four contrast
levels (0 = normal, 6 = the dark "needs FLASH" cave palette, 9 = black). Subtract anything
else and the read STRADDLES TWO ENTRIES, producing a palette that does not exist anywhere in
the game: 1, 2, 4, 5, 7, 8 -- exactly the six glitch palettes.

Nothing here is asserted from reading the disassembly. Each value is written into a save, the
real ROM is booted with it, and the palette registers the console actually ends up holding
(rBGP $FF47, rOBP0 $FF48, rOBP1 $FF49) are compared against ours.

Local-only; needs the gitignored ROM. Run: python scripts/emu/verify_palettes.py
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
OUT = REPO / "tmp" / "emu-pal"

SAV_CONTRAST = 0x2609          # wMapPalOffset
SAV_CHECKSUM = 0x3523
SAV_CHECKSUM_START = 0x2598
SAV_CHECKSUM_LEN = 0xF8B

W_CUR_MAP = 0xD35E
W_CUR_MAP_WIDTH = 0xD369
W_CUR_MAP_HEIGHT = 0xD368
W_OVERWORLD_MAP = 0xC6E8
W_TILEMAP = 0xC3A0
SCREEN_TILES = 20 * 18

R_BGP = 0xFF47
R_OBP0 = 0xFF48
R_OBP1 = 0xFF49


# ── the fade table, as bytes ────────────────────────────────────────────────────────
#
# `dc a,b,c,d` packs four 2-bit shades into one byte, most-significant first -- which is
# exactly the Game Boy's palette register layout (bits 7-6 = colour 3 ... bits 1-0 = colour
# 0). So `dc 3,2,1,0` is 0xE4, the identity palette.
def dc(c3: int, c2: int, c1: int, c0: int) -> int:
    return (c3 << 6) | (c2 << 4) | (c1 << 2) | c0


FADE_TABLE = bytes([
    dc(3, 3, 3, 3), dc(3, 3, 3, 3), dc(3, 3, 3, 3),   # FadePal1  @ 0
    dc(3, 3, 3, 2), dc(3, 3, 3, 2), dc(3, 3, 2, 0),   # FadePal2  @ 3
    dc(3, 3, 2, 1), dc(3, 2, 1, 0), dc(3, 2, 1, 0),   # FadePal3  @ 6
    dc(3, 2, 1, 0), dc(3, 1, 0, 0), dc(3, 2, 0, 0),   # FadePal4  @ 9   <- the base
    dc(3, 2, 1, 0), dc(3, 1, 0, 0), dc(3, 2, 0, 0),   # FadePal5  @ 12
    dc(2, 1, 0, 0), dc(2, 0, 0, 0), dc(2, 1, 0, 0),   # FadePal6  @ 15
    dc(1, 0, 0, 0), dc(1, 0, 0, 0), dc(1, 0, 0, 0),   # FadePal7  @ 18
    dc(0, 0, 0, 0), dc(0, 0, 0, 0), dc(0, 0, 0, 0),   # FadePal8  @ 21
])

FADE_PAL4 = 9   # FadePal4's index in the table -- what the game subtracts from


def palette_for(contrast: int):
    """(rBGP, rOBP0, rOBP1) for a contrast value -- FadePal4 - contrast, three bytes."""
    at = FADE_PAL4 - contrast
    if at < 0 or at + 3 > len(FADE_TABLE):
        return None   # past the table: it would read whatever ROM sits there
    return tuple(FADE_TABLE[at:at + 3])


def build_save(base: bytes, contrast: int) -> bytes:
    sav = bytearray(base)
    sav[SAV_CONTRAST] = contrast

    checksum = 0xFF
    for i in range(SAV_CHECKSUM_START, SAV_CHECKSUM_START + SAV_CHECKSUM_LEN):
        checksum = (checksum - sav[i]) & 0xFF
    sav[SAV_CHECKSUM] = checksum
    return bytes(sav)


def boot(pyboy, budget: int = 8000) -> bool:
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

    # Settle: the game fades in, so the registers move for a while after the map is up.
    for _ in range(240):
        pyboy.tick()
    return True


def main() -> int:
    if not ROM.exists():
        print("SKIP: no ROM (local-only verification)")
        return 2

    from pyboy import PyBoy

    OUT.mkdir(parents=True, exist_ok=True)
    rom = OUT / "rom.gb"
    shutil.copyfile(ROM, rom)
    base = BASE_SAV.read_bytes()

    print("contrast  the console        ours              verdict")
    print("--------  ----------------   ----------------  -------")

    bad = 0
    for contrast in range(10):
        (OUT / "rom.gb.ram").write_bytes(build_save(base, contrast))

        pyboy = PyBoy(str(rom), window="null", sound_emulated=False)
        if not boot(pyboy):
            print(f"  {contrast:<7}  (never reached the overworld)")
            bad += 1
            pyboy.stop(save=False)
            continue

        mem = pyboy.memory
        theirs = (mem[R_BGP], mem[R_OBP0], mem[R_OBP1])
        ours = palette_for(contrast)

        pyboy.screen.image.save(OUT / f"contrast_{contrast}.png")
        pyboy.stop(save=False)

        ok = (ours == theirs)
        if not ok:
            bad += 1

        kind = "level" if contrast % 3 == 0 else "GLITCH"
        t = " ".join(f"{b:02X}" for b in theirs)
        o = " ".join(f"{b:02X}" for b in ours) if ours else "(past the table)"
        print(f"  {contrast:<7} {t:<18} {o:<17} {'MATCH' if ok else '*** MISMATCH ***'}  {kind}")

    print()
    print("(rBGP is what the MAP is drawn with; rOBP0/1 are the sprite palettes -- which is why")
    print(" contrast 1 and 2 look normal on the background but wreck the sprites.)")
    print(f"\nmismatches: {bad}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
