r"""Do the ten in-game trade "done" flags survive a Continue?

`wCompletedInGameTradeFlags` (save 0x29E3-0x29E4 -> $D737-$D738) is 10 bits in 2 bytes, one per
entry of `TradeMons`. `engine/events/in_game_trades.asm` reads it with the same `FlagAction` the
event flags use -- byte = base + i/8, mask = 1 << (i%8):

    DoInGameTradeDialogue:
        ...
        ld hl, wCompletedInGameTradeFlags
        ld a, [wWhichTrade]
        ld c, a
        ld b, FLAG_TEST
        predef FlagActionPredef
        ld a, c
        and a
        ld a, TRADETEXT_AFTER_TRADE
        ld [wInGameTradeTextPointerTableIndex], a
        jr nz, .printText              ; <- set => only ever the after-trade line, and RETURN

...and `InGameTrade_DoTrade` sets it (`ld b, FLAG_SET`) once a swap completes. **Nothing else gates a
trade** -- no event flag, no script step -- so the bit alone decides whether the NPC will deal with
you. That is exactly why clearing it is a feature: it re-arms the trade.

The claims under test:

  1. Is the bit DURABLE across a Continue? It lives in SECTION "Main Data" (inside the saved block,
     between wMovementFlags and wWarpedFromWhichWarp) and no loader was found writing it -- unlike
     wStatusFlags3, which is zeroed whole, and wTownVisitedFlag, which re-marks the current town.
     A source read that finds "nothing writes it" is the weakest kind of read: it is an argument from
     absence. The console settles it.
  2. Are the SIX SPARE BITS (2-7 of 0x29E4) left alone? `WorldTrades::save()` hands setBitField only
     10 bools and the guard `if((i+n) < src.size())` should mean bits 11-15 are never written -- but
     the console is also asked whether IT leaves them alone, because a save editor that preserves a
     bit the game clobbers is preserving nothing.

  A  baseline     untouched BaseSAV (trades 0x00 0x00) -> nothing done, as shipped
  B  all ten SET  0x29E3 := 0xFF, 0x29E4 := 0x03       -> all 10 kept?
  C  all clear    the control                          -> stays 0 (proves the loader does not set them)
  D  spare bits   0x29E4 := 0xFF (bits 2-7 too)        -> does the console preserve or clear them?

⏳ NOT covered here, and honestly so: whether a SET bit actually mutes the offer, and whether
clearing it re-arms the trade in play. Both need walking to a trader and talking (the autopilot's
emu_goto + emu_talk_to). The nearest trader to BaseSAV's Pallet Town is several maps away; that is
its own run. What this probe settles is the persistence the UI rests on.

Local-only; needs the gitignored ROM. Run:
    tmp\emu-venv\Scripts\python.exe scripts\emu\probe_in_game_trades.py
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
OUT = REPO / "tmp" / "emu-trades"

# ── save offsets ────────────────────────────────────────────────────────────────────
# sMainData starts at 0x25A3 and maps wMainDataStart = $D2F7, so  wram = sav + 0xAD54.
SAV_TRADE_FLAGS = 0x29E3           # wCompletedInGameTradeFlags  $D737  (2 bytes, 10 bits used)

SAV_CHECKSUM = 0x3523
SAV_CHECKSUM_START = 0x2598
SAV_CHECKSUM_LEN = 0xF8B

# ── wram addresses ──────────────────────────────────────────────────────────────────
W_TRADE_FLAGS = 0xD737             # 0x29E3 + 0xAD54

W_CUR_MAP_WIDTH = 0xD369
W_CUR_MAP_HEIGHT = 0xD368
W_OVERWORLD_MAP = 0xC6E8
W_TILEMAP = 0xC3A0
SCREEN_TILES = 20 * 18

NUM_NPC_TRADES = 10

# bit -> (nickname of the mon you RECEIVE, give, get). ⚠️ The 4th npctrade field is the received
# Pokémon's NICKNAME, not the trader's name -- TERRY is the Nidorina. See
# notes/reference/in-game-trades.md §2a.
TRADES = [
    ("TERRY",      "Nidorino",   "Nidorina"),
    ("MARCEL",     "Abra",       "Mr. Mime"),
    ("CHIKUCHIKU", "Butterfree", "Beedrill"),   # unused -- no script, no map, no coords
    ("SAILOR",     "Ponyta",     "Seel"),
    ("DUX",        "Spearow",    "Farfetch'd"),
    ("MARC",       "Slowbro",    "Lickitung"),
    ("LOLA",       "Poliwhirl",  "Jynx"),
    ("DORIS",      "Raichu",     "Electrode"),
    ("CRINKLES",   "Venonat",    "Tangela"),
    ("SPOT",       "Nidoran M",  "Nidoran F"),
]
UNUSED_TRADE = 2


def checksum(sav: bytearray) -> int:
    c = 0xFF
    for i in range(SAV_CHECKSUM_START, SAV_CHECKSUM_START + SAV_CHECKSUM_LEN):
        c = (c - sav[i]) & 0xFF
    return c


def sealed(sav: bytearray) -> bytes:
    sav[SAV_CHECKSUM] = checksum(sav)
    return bytes(sav)


def with_flags(base: bytes, lo: int, hi: int) -> bytes:
    sav = bytearray(base)
    sav[SAV_TRADE_FLAGS] = lo
    sav[SAV_TRADE_FLAGS + 1] = hi
    return sealed(sav)


def read_bits(lo: int, hi: int) -> list[int]:
    word = lo | (hi << 8)
    return [(word >> i) & 1 for i in range(NUM_NPC_TRADES)]


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
    r = {"lo": mem[W_TRADE_FLAGS], "hi": mem[W_TRADE_FLAGS + 1]}
    r["bits"] = read_bits(r["lo"], r["hi"])
    r["spare"] = (r["hi"] >> 2) & 0x3F      # bits 2-7 of the high byte: the six the game never uses
    pyboy.screen.image.save(OUT / f"{label}.png")
    pyboy.stop(save=False)
    return r


def show(label: str, r: dict) -> None:
    print(f"\n  {label}")
    print(f"    wCompletedInGameTradeFlags   0x{r['lo']:02X} 0x{r['hi']:02X}")
    done = [f"{TRADES[i][0]}" for i, b in enumerate(r["bits"]) if b]
    print(f"    completed ({len(done)})               {', '.join(done) if done else '(none)'}")
    print(f"    spare bits (11-15)           0x{r['spare']:02X}")


def main() -> int:
    if not ROM.exists():
        print("SKIP: no ROM (local-only verification)")
        return 2

    OUT.mkdir(parents=True, exist_ok=True)
    rom = OUT / "rom.gb"
    shutil.copyfile(ROM, rom)
    base = bytes(BASE_SAV.read_bytes())

    a = run(rom, sealed(bytearray(base)), "A_baseline")
    b = run(rom, with_flags(base, 0xFF, 0x03), "B_all_ten_set")
    c = run(rom, with_flags(base, 0x00, 0x00), "C_all_clear")
    d = run(rom, with_flags(base, 0xFF, 0xFF), "D_spare_bits_set")

    if a:
        show("A  baseline (untouched BaseSAV)", a)
    if b:
        show("B  all ten trade flags SET in the save", b)
    if c:
        show("C  all ten CLEARED (control)", c)
    if d:
        show("D  spare bits 11-15 set too (does the console keep them?)", d)

    print("\n  VERDICT")
    ok = True

    if b:
        kept = all(b["bits"])
        print(f"    All ten survive a Continue?          "
              f"{'YES -- read back set' if kept else 'NO -- ' + str(sum(b['bits'])) + '/10 survived'}")
        print(f"      => the trade flags are {'DURABLE' if kept else 'NOT durable'}; editing one sticks.")
        ok &= kept
        # The unused trade's bit is real storage even though no script reads it.
        print(f"    ...including the UNUSED trade ({TRADES[UNUSED_TRADE][0]})?  "
              f"{'YES' if b['bits'][UNUSED_TRADE] else 'NO'}")

    if c:
        stayed = not any(c["bits"])
        print(f"    Control: cleared flags stay clear?   "
              f"{'YES -- the loader sets none of them' if stayed else 'NO -- something set one'}")
        ok &= stayed

    if d:
        kept_spare = d["spare"] == 0x3F
        print(f"    Spare bits 11-15 preserved?          "
              f"{'YES -- the game leaves them alone' if kept_spare else 'NO -- console cleared them (0x' + format(d['spare'], '02X') + ')'}")
        print(f"      => preserving them in WorldTrades::save() is "
              f"{'meaningful' if kept_spare else 'pointless -- the console clobbers them anyway'}.")

    print(f"\n    Matches the prediction in notes/reference/in-game-trades.md: "
          f"{'YES' if ok else 'NO -- the note must be corrected'}")
    print("\n    ⏳ Still owed: does a SET bit mute the offer, and does clearing it re-arm the")
    print("       trade in play? Needs the autopilot to walk to a trader and talk. Separate run.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
