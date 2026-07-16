"""Forge a save AT Route 22 with both rival-battle flags on, drive the player up
into the rival trigger, and see if the console crashes. (Phase 4 confirmation of
a suspected conflicting-flags pair.)

Route 22 has TWO SPRITE_BLUE objects at the same tile (25,5): ROUTE22_RIVAL1 and
ROUTE22_RIVAL2, one per battle. Leadership reports both battles' flags on crashes
on battle. We now have total map-state control, so we forge the exact save.

Local-only; needs the gitignored ROM + tmp/emu-venv PyBoy venv.
"""
from __future__ import annotations
from pathlib import Path
import json

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
OUT = REPO / "tmp" / "emu-route22"
OUT.mkdir(parents=True, exist_ok=True)

EV_START, EV_LEN = 0x29F3, 0x140
SAV_CUR_MAP = 0x260A     # wCurMap  $D35E
SAV_YCOORD = 0x260D      # wYCoord  $D361
SAV_XCOORD = 0x260E      # wXCoord  $D362
SAV_CHECKSUM = 0x3523
SAV_CHECKSUM_START = 0x2598
SAV_CHECKSUM_LEN = 0xF8B

ROUTE_22 = 0x21
W_CUR_MAP = 0xD35E
W_CUR_MAP_WIDTH = 0xD369
W_CUR_MAP_HEIGHT = 0xD368
W_OVERWORLD_MAP = 0xC6E8
W_TILEMAP = 0xC3A0
W_IS_IN_BATTLE = 0xD057
SCREEN = 20 * 18


def checksum(sav):
    c = 0xFF
    for i in range(SAV_CHECKSUM_START, SAV_CHECKSUM_START + SAV_CHECKSUM_LEN):
        c = (c - sav[i]) & 0xFF
    return c


def idx(name):
    canon = json.loads((REPO / "tmp" / "event-flags" /
                        "event_flags_canonical.json").read_text(encoding="utf-8"))
    for r in canon:
        if r["name"] == name:
            return r["index"]
    raise KeyError(name)


def forge(set_flags, y, x):
    sav = bytearray(BASE_SAV.read_bytes())
    sav[SAV_CUR_MAP] = ROUTE_22
    sav[SAV_YCOORD] = y
    sav[SAV_XCOORD] = x
    for name in set_flags:
        i = idx(name)
        sav[EV_START + i // 8] |= (1 << (i % 8))
    sav[SAV_CHECKSUM] = checksum(sav)
    return bytes(sav)


def on_overworld(pb):
    m = pb.memory
    w, h = m[W_CUR_MAP_WIDTH], m[W_CUR_MAP_HEIGHT]
    if not (0 < w <= 64 and 0 < h <= 96):
        return False
    blocks = bytes(m[W_OVERWORLD_MAP:W_OVERWORLD_MAP + (w + 6) * (h + 6)])
    screen = bytes(m[W_TILEMAP:W_TILEMAP + SCREEN])
    return len(set(blocks)) > 1 and len(set(screen)) > 1


def sane(pb):
    """healthy if on a sane overworld OR in a sane battle (screen has variety)."""
    if on_overworld(pb):
        return True
    m = pb.memory
    if m[W_IS_IN_BATTLE] in (1, 2):
        screen = bytes(m[W_TILEMAP:W_TILEMAP + SCREEN])
        return len(set(screen)) > 1
    return False


def boot(pb, budget=9000):
    f = 0
    while f < budget and not on_overworld(pb):
        pb.button("start" if (f // 24) % 2 == 0 else "a", delay=8)
        for _ in range(24):
            pb.tick()
        f += 24
    return on_overworld(pb)


def run(label, sav, drive_up=200):
    from pyboy import PyBoy
    rom = OUT / "rom.gb"
    rom.write_bytes(ROM.read_bytes())
    (OUT / "rom.gb.ram").write_bytes(sav)
    pb = PyBoy(str(rom), window="null", sound_emulated=False)
    if not boot(pb):
        pb.stop(save=False)
        return f"{label:34} -> did not reach overworld on Route 22"
    cur = pb.memory[W_CUR_MAP]
    entered_battle = False
    crashed_at = None
    for step in range(drive_up):
        pb.button("up", delay=4)
        for _ in range(8):
            pb.tick()
        if pb.memory[W_IS_IN_BATTLE] in (1, 2):
            entered_battle = True
        if not sane(pb):
            crashed_at = step
            break
    pb.stop(save=False)
    where = f"map 0x{cur:02X}"
    if crashed_at is not None:
        return f"{label:34} -> CRASH/garbage at drive step {crashed_at} ({where}, battle={entered_battle})"
    return f"{label:34} -> survived {drive_up} steps ({where}, battle={entered_battle})"


def main():
    if not ROM.exists():
        raise SystemExit("ROM missing")
    print("Route 22 rival conflict — forged saves, player driven up into the trigger\n")
    # spawn just below the rival tile (25,5): x=25, y=8, walk up
    print(run("A control (no rival flags)", forge([], 8, 25)))
    print(run("B WANTS only (normal 1st battle)",
              forge(["EVENT_ROUTE22_RIVAL_WANTS_BATTLE"], 8, 25)))
    print(run("C 1ST+2ND+WANTS (the conflict)",
              forge(["EVENT_1ST_ROUTE22_RIVAL_BATTLE",
                     "EVENT_2ND_ROUTE22_RIVAL_BATTLE",
                     "EVENT_ROUTE22_RIVAL_WANTS_BATTLE"], 8, 25)))


if __name__ == "__main__":
    main()
