"""Boot the real game with one of our save files and dump what it actually did.

This is the oracle. The editor can be internally consistent and still be wrong; the only
thing that can't be is the console. So we hand the ROM a save, let it load, and then read
the game's own RAM and its own screen back out.

What it writes to <out>:
  state.json          what the game says about where the player is (map, tileset, size,
                      coords, and the view pointer IT computed)
  wOverworldMap.bin   the block buffer the game built: the map ringed by its 3-block
                      border  (WRAM $C6E8, stride = width + 6)
  wSurroundingTiles.bin  the 6x5-block scratch area, expanded to 24x20 TILE ids ($C508)
  wTileMap.bin        the 20x18 tile ids actually on screen ($C3A0)
  screen.png          the 160x144 framebuffer (BG + sprites + palette)

Those three buffers are exactly what MapEngine reproduces, so tst_emu_parity can compare
them byte for byte. wTileMap is the strongest of them: it is the whole view pipeline in
one array -- blocks, the block->tile expansion, the scratch area and the half-block screen
offset -- with no sprites and no palettes in the way.

Local-only. Needs assets/references/backup.gb, a legally-backed-up cartridge dump that is
gitignored and MUST never be committed or redistributed. No ROM -> this exits 2 and the
test that calls it skips.

Usage:
  python scripts/emu/dump_state.py [--sav <path>] [--out <dir>] [--rom <path>]
"""

from __future__ import annotations

import argparse
import json
import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]

# WRAM (ram/wram.asm; addresses fixed by the linker and cross-checked arithmetically:
# wTileMap 360 bytes -> a 480-byte union -> wOverworldMap, which lands on $C6E8, the base
# the save's own view pointer is relative to).
W_TILEMAP = 0xC3A0            # 20 x 18 tile ids
W_SURROUNDING_TILES = 0xC508  # 24 x 20 tile ids
W_OVERWORLD_MAP = 0xC6E8      # (width + 6) x (height + 6) block ids
W_CUR_MAP = 0xD35E
W_VIEW_PTR = 0xD35F           # wCurrentTileBlockMapViewPointer, little endian
W_Y_COORD = 0xD361
W_X_COORD = 0xD362
W_Y_BLOCK = 0xD363
W_X_BLOCK = 0xD364
W_CUR_MAP_TILESET = 0xD367
W_CUR_MAP_HEIGHT = 0xD368
W_CUR_MAP_WIDTH = 0xD369

SCREEN_TILES = 20 * 18
SURROUNDING_TILES = 24 * 20


def _tick(pyboy, frames: int) -> None:
    for _ in range(frames):
        pyboy.tick()


def boot_to_overworld(pyboy, expect: dict, budget: int = 6000) -> None:
    """Drive the intro -> title -> CONTINUE -> overworld, mechanically and repeatably.

    Not "press A a few times and hope". We press through the front end while WATCHING the
    game's own RAM, and we stop the moment the save is actually loaded -- which we detect
    by the game agreeing with the save file we handed it (same map, same coordinates,
    and a sane map size). That is a fact we read straight out of the .sav, so it is not
    the thing under test: the view pointer, the block buffer and the tile maps stay
    untouched by this and remain honest comparisons.

    Two rules keep it safe:
      * A is pressed ONLY while the save has not loaded yet (A is what takes CONTINUE).
      * Once loaded, only B is used -- B dismisses the save-info box and, unlike A, does
        nothing at all in the overworld, so it can never talk to a sign or an NPC and
        change the state we are about to measure.
    """
    def save_in_wram() -> bool:
        """The save's state is in WRAM. NOT the same as being on the overworld."""
        mem = pyboy.memory
        width = mem[W_CUR_MAP_WIDTH]
        height = mem[W_CUR_MAP_HEIGHT]
        return (
            mem[W_CUR_MAP] == expect["curMap"]
            and mem[W_X_COORD] == expect["xCoord"]
            and mem[W_Y_COORD] == expect["yCoord"]
            and 0 < width <= 64
            and 0 < height <= 96
        )

    def loaded() -> bool:
        """Actually standing on the map, with the game's view built.

        The obvious check -- "does WRAM hold the save's map and coords?" -- is a TRAP: the
        main menu reads the save into WRAM just to decide whether to offer CONTINUE, so it
        passes while the title screen is still up and no map exists at all. (It fooled this
        harness once; the emulator screenshot is what gave it away.)

        wOverworldMap is the honest signal. Nothing fills it but LoadTileBlockMap, and
        nothing calls that but EnterMap -- so it stays blank right up until the player is
        genuinely standing on the map.
        """
        if not save_in_wram():
            return False

        mem = pyboy.memory
        width = mem[W_CUR_MAP_WIDTH]
        height = mem[W_CUR_MAP_HEIGHT]
        block_buffer = bytes(mem[W_OVERWORLD_MAP:W_OVERWORLD_MAP + (width + 6) * (height + 6)])
        screen = bytes(mem[W_TILEMAP:W_TILEMAP + SCREEN_TILES])

        return len(set(block_buffer)) > 1 and len(set(screen)) > 1

    frames = 0
    while frames < budget and not loaded():
        # START skips the title/demo; A takes CONTINUE off the main menu (it is the first
        # entry whenever a save exists, so no cursor movement is ever needed).
        pyboy.button("start" if (frames // 24) % 2 == 0 else "a", delay=8)
        _tick(pyboy, 24)
        frames += 24

        if frames % 480 == 0:
            m = pyboy.memory
            print(f"  boot f={frames:5d} map={m[W_CUR_MAP]:3d} x={m[W_X_COORD]:3d} "
                  f"y={m[W_Y_COORD]:3d} w={m[W_CUR_MAP_WIDTH]:3d} h={m[W_CUR_MAP_HEIGHT]:3d} "
                  f"tileset={m[W_CUR_MAP_TILESET]:3d}", file=sys.stderr)

    print(f"  boot: on the overworld after {frames} frames", file=sys.stderr)

    if not loaded():
        raise RuntimeError(
            f"the game never reached the overworld within {budget} frames "
            f"(save in WRAM: {save_in_wram()})"
        )

    # We are on the map and no longer touch a single button -- in particular never A,
    # which would talk to a sign or an NPC and change the very state we came to measure.
    _tick(pyboy, 180)

    # The screen buffer must actually hold a drawn map -- an all-zero wTileMap would mean
    # we are sitting on a blank/menu screen and everything downstream would be garbage.
    tilemap = bytes(pyboy.memory[W_TILEMAP:W_TILEMAP + SCREEN_TILES])
    if len(set(tilemap)) < 2:
        raise RuntimeError("wTileMap is blank -- not actually on the overworld")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--rom", default=str(REPO / "assets" / "references" / "backup.gb"))
    ap.add_argument("--sav", default=str(REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"))
    ap.add_argument("--out", default=str(REPO / "tmp" / "emu"))
    args = ap.parse_args()

    rom_src = Path(args.rom)
    if not rom_src.exists():
        print(f"SKIP: no ROM at {rom_src} (local-only verification)", file=sys.stderr)
        return 2

    try:
        from pyboy import PyBoy
    except ImportError:
        print("SKIP: pyboy not installed (run scripts/emu/setup.ps1)", file=sys.stderr)
        return 2

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    # PyBoy reads battery RAM from <rom>.ram, so work on COPIES in the scratch dir: the
    # real ROM and the real fixture are never touched, moved, or written to.
    rom = out / "rom.gb"
    sav = Path(args.sav)
    shutil.copyfile(rom_src, rom)
    shutil.copyfile(sav, out / "rom.gb.ram")

    # What the save file itself says -- read straight from its bytes. Used only to know
    # WHEN the game has finished loading it (see boot_to_overworld); never to fake a pass.
    raw = sav.read_bytes()
    expect = {
        "curMap": raw[0x260A],
        "xCoord": raw[0x260E],
        "yCoord": raw[0x260D],
    }

    pyboy = PyBoy(str(rom), window="null", sound_emulated=False)
    boot_to_overworld(pyboy, expect)

    mem = pyboy.memory

    width = mem[W_CUR_MAP_WIDTH]
    height = mem[W_CUR_MAP_HEIGHT]
    stride = width + 6
    rows = height + 6

    state = {
        "sav": str(Path(args.sav).name),
        "curMap": mem[W_CUR_MAP],
        # Bit 7 of the LIVE wCurMapTileset is not part of the tileset id -- it is
        # BIT_NO_PREVIOUS_MAP, a flag the game sets in RAM (home/overworld.asm). The save
        # file stores the id clean, which is why the editor never sees it; mask it here or
        # Overworld reads back as 128.
        "tileset": mem[W_CUR_MAP_TILESET] & 0x7F,
        "tilesetRaw": mem[W_CUR_MAP_TILESET],
        "width": width,
        "height": height,
        "xCoord": mem[W_X_COORD],
        "yCoord": mem[W_Y_COORD],
        "xBlockCoord": mem[W_X_BLOCK],
        "yBlockCoord": mem[W_Y_BLOCK],
        "viewPointer": mem[W_VIEW_PTR] | (mem[W_VIEW_PTR + 1] << 8),
        "stride": stride,
        "rows": rows,
    }

    (out / "state.json").write_text(json.dumps(state, indent=2))
    (out / "wOverworldMap.bin").write_bytes(bytes(mem[W_OVERWORLD_MAP:W_OVERWORLD_MAP + stride * rows]))
    (out / "wSurroundingTiles.bin").write_bytes(bytes(mem[W_SURROUNDING_TILES:W_SURROUNDING_TILES + SURROUNDING_TILES]))
    (out / "wTileMap.bin").write_bytes(bytes(mem[W_TILEMAP:W_TILEMAP + SCREEN_TILES]))

    try:
        pyboy.screen.image.save(out / "screen.png")
    except Exception as exc:  # Pillow missing -- the memory dumps are the important part
        print(f"note: no framebuffer PNG ({exc})", file=sys.stderr)

    pyboy.stop(save=False)

    print(json.dumps(state))
    return 0


if __name__ == "__main__":
    sys.exit(main())
