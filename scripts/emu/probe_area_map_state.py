"""Which AREA-MAP ("Map" page) bytes does the game KEEP from our save on Continue, and
which does it REWRITE / DERIVE the instant the save loads?

These are the fields on v1's "Map" page under "Area" that Twilight is bringing to the
map-details panel (shown when nothing is selected). A careful read of the Continue path
(engine/menus/main_menu.asm -> SpecialEnterMap -> home/overworld.asm EnterMap, which does
`call LoadMapData` then `farcall ClearVariablesOnEnterMap`) predicts several rewrites --
and a careful read has been WRONG before (the sprite persistence pass,
notes/reference/sprites.md Part 5), so the console gets asked.

VERIFIED against the cartridge (Pallet Town fixture, ordinary Continue). The console's
verdict, with two SURPRISES a source read alone would have gotten wrong:

  * wCurrentTileBlockMapViewPointer (0x260B) -> KEPT (trusted from the save), NOT recomputed.
        Predicted "derived/recomputed"; WRONG. The coords->pointer recompute lives in
        CheckMapConnections (a step across a map edge) and warp-arrival, NOT on the Continue
        path -- and LoadMapHeader bails early on Continue (BIT_NO_PREVIOUS_MAP, the warp
        linchpin). So the save's pointer is trusted as-is: writing 0xFFFF here made the
        console DRAW GARBAGE from $FFFF (see areamap.png). It IS a derived value (the game
        computed it from coords while you played). Doctrine (Twilight 2026-07-15): keep it
        SYNCED to the coords by default; power users can break sync (toggle / alert on manual
        entry / drag the view box on the canvas). Not a bare address in the normal case.
  * wMapViewVRAMPointer (0x27D2) -> RESET to $9800 (vBGMap0) every load. LoadMapData:
        `ld a, HIGH(vBGMap0) / ld [wMapViewVRAMPointer+1], a / xor a / ld [..], a`. Live
        render pointer, not persistent state.
  * wCardKeyDoorY/X (0x29EB/0x29EC) -> ZEROED every map enter. ClearVariablesOnEnterMap:
        `ld hl, wCardKeyDoorY / ld [hli], a / ld [hl], a` (a=0). Scratch for the Silph
        card-key puzzle; set by card_key.asm, read by the SilphCo scripts, cleared here.
  * wStatusFlags7 BIT_USE_CUR_MAP_SCRIPT (0x29DF b4) -> KEPT on Pallet, NOT cleared.
        Predicted "cleared"; WRONG for a quiet map. ExecuteCurMapScriptInTable does clear it,
        but only trainer/scripted maps route through it -- on a map without that path the
        one-shot handoff bit is never consumed and SURVIVES. So a save can carry it set to
        auto-run a chosen map script on load: an editable lever, not dead plumbing.
  * wStatusFlags6 BIT_ALWAYS_ON_BIKE (0x29DE b5) -> KEPT (persists; nothing on the load path
        clears it -- oak_speech.asm even flags that it wrongly carries into a NEW game).
        Durable, meaningful, editable (the Cycling Road "always on bike" lock).
  * wCurMapScript (0x2CE5) -> KEPT. Never reset on the load path; Pallet's own script did not
        re-derive it on the first tick. Durable per-map story-script progress index.

So "rewritten at boot" = mapViewVRAMPointer (->$9800) and cardKeyDoorX/Y (->0). Trusted =
viewPointer (derived; sync-by-default), curMapScript, forceBikeRide, and curMapNextFrame
(an auto-run-script-on-load lever on maps where it persists).

Local-only; needs the gitignored ROM. Run: python scripts/emu/probe_area_map_state.py
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
OUT = REPO / "tmp" / "emu-areamap"

# sMainData starts at 0x25A3 -> wMainDataStart $D2F7, so  wram = sav + 0xAD54
DELTA = 0xAD54

# ── the area-map bytes: (label, sav offset) ──────────────────────────────────────────
VIEWPTR = 0x260B        # wCurrentTileBlockMapViewPointer  (word, LE) -- derived from coords
VRAMPTR = 0x27D2        # wMapViewVRAMPointer              (word, LE) -- reset to $9800
CARDKEYY = 0x29EB       # wCardKeyDoorY                    (byte)     -- zeroed on enter
CARDKEYX = 0x29EC       # wCardKeyDoorX                    (byte)     -- zeroed on enter
CURMAPSCRIPT = 0x2CE5   # wCurMapScript                    (byte)     -- persistent progress
SF6 = 0x29DE            # wStatusFlags6  (b5 BIT_ALWAYS_ON_BIKE)
SF7 = 0x29DF            # wStatusFlags7  (b4 BIT_USE_CUR_MAP_SCRIPT)

SAV_CHECKSUM = 0x3523
SAV_CHECKSUM_START = 0x2598
SAV_CHECKSUM_LEN = 0xF8B

WORDS = [
    ("wCurrentTileBlockMapViewPointer", VIEWPTR),
    ("wMapViewVRAMPointer            ", VRAMPTR),
]
BYTES = [
    ("wCardKeyDoorY                  ", CARDKEYY),
    ("wCardKeyDoorX                  ", CARDKEYX),
    ("wCurMapScript                  ", CURMAPSCRIPT),
]
BITS = [
    ("SF6.b5 forceBikeRide (ALWAYS_ON_BIKE)  ", SF6, 5),
    ("SF7.b4 curMapNextFrame (USE_CUR_SCRIPT)", SF7, 4),
]


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
    # two view pointers: stamp with recognisable garbage; expect both replaced
    sav[VIEWPTR] = 0xFF
    sav[VIEWPTR + 1] = 0xFF
    sav[VRAMPTR] = 0xFF
    sav[VRAMPTR + 1] = 0xFF
    # card-key door scratch: distinct markers; expect both zeroed
    sav[CARDKEYY] = 0x11
    sav[CARDKEYX] = 0x12
    # map script progress: a marker; expect kept (Pallet Town has no reset)
    sav[CURMAPSCRIPT] = 0x03
    # the two flags on; expect ALWAYS_ON_BIKE kept, USE_CUR_MAP_SCRIPT cleared
    sav[SF6] |= (1 << 5)
    sav[SF7] |= (1 << 4)
    return sealed(sav)


# ── boot to overworld, reused from probe_player_state.py ─────────────────────────────
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
    out: dict = {}
    for _, off in WORDS:
        out[off] = mem[off + DELTA] | (mem[off + DELTA + 1] << 8)
    for _, off in BYTES:
        out[off] = mem[off + DELTA]
    for _, off, bit in BITS:
        out[("bit", off, bit)] = (mem[off + DELTA] >> bit) & 1
    return out


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
    got = run(rom, sav, "areamap")
    if not got:
        return 1

    print("\n===== Area-map state on an ordinary Continue (Pallet Town fixture) =====")
    print("  word fields (view pointers):")
    for lab, off in WORDS:
        w = sav[off] | (sav[off + 1] << 8)
        g = got[off]
        mark = "kept   " if w == g else "DERIVED/RESET"
        print(f"    {lab} wrote 0x{w:04X}  read 0x{g:04X}   {mark}")
    print("  byte fields:")
    for lab, off in BYTES:
        w, g = sav[off], got[off]
        mark = "kept   " if w == g else "REWRITE"
        print(f"    {lab} wrote 0x{w:02X}  read 0x{g:02X}   {mark}")
    print("  bit fields:")
    for lab, off, bit in BITS:
        w = (sav[off] >> bit) & 1
        g = got[("bit", off, bit)]
        mark = "kept   " if w == g else "REWRITE"
        print(f"    {lab} wrote {w}  read {g}   {mark}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
