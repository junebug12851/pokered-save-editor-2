"""Does editing wEventFlags crash the game? (Phase 4 console test)

Leadership reports that turning ALL event flags on crashes the game. Mechanism
(from the disassembly): flags drive wCur<Map>Script via CheckEvent chains, which
dispatch through script-pointer tables ending in `jp hl`; impossible combinations
resolve a bad pointer. This boots the real ROM (PyBoy) on Continue with several
event-flag mutations and reports whether the game reaches — and stays on — a sane
overworld. A game that never reaches the overworld (or whose map state goes
garbage) has crashed.

NOTE ON SCOPE: BaseSAV starts the player in Pallet Town, so this reliably tests
the LOAD path. A crash that only fires on a specific map / on a battle trigger
(e.g. the two Route 22 rival battles) needs the player driven to that trigger —
flagged in the output, not silently claimed.

Local-only; needs the gitignored ROM (assets/references/backup.gb) and the
tmp/emu-venv PyBoy venv (scripts/emu/setup.ps1). Run with that venv's python.
"""
from __future__ import annotations
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
OUT = REPO / "tmp" / "emu-eventflags"
OUT.mkdir(parents=True, exist_ok=True)

# wEventFlags: file 0x29F3 .. 0x2B32 (320 bytes) — verified vs pret.
EV_START, EV_LEN = 0x29F3, 0x140
SAV_CHECKSUM = 0x3523
SAV_CHECKSUM_START = 0x2598
SAV_CHECKSUM_LEN = 0xF8B

W_CUR_MAP_WIDTH = 0xD369
W_CUR_MAP_HEIGHT = 0xD368
W_OVERWORLD_MAP = 0xC6E8
W_TILEMAP = 0xC3A0
W_CUR_MAP = 0xD35E
SCREEN_TILES = 20 * 18

# a couple of named flags for the contradiction case (indices from event_constants)
# 1ST/2ND Route 22 rival battle "in progress" flags share the same NPC/script.
EVENT_1ST_ROUTE22 = None  # resolved from the canonical map at runtime
EVENT_2ND_ROUTE22 = None
EVENT_ROUTE22_WANTS = None


def checksum(sav: bytearray) -> int:
    c = 0xFF
    for i in range(SAV_CHECKSUM_START, SAV_CHECKSUM_START + SAV_CHECKSUM_LEN):
        c = (c - sav[i]) & 0xFF
    return c


def sealed(sav: bytearray) -> bytes:
    sav[SAV_CHECKSUM] = checksum(sav)
    return bytes(sav)


def set_bit(sav: bytearray, index: int):
    sav[EV_START + index // 8] |= (1 << (index % 8))


def resolve_indices():
    import json
    global EVENT_1ST_ROUTE22, EVENT_2ND_ROUTE22, EVENT_ROUTE22_WANTS
    canon = json.loads((REPO / "tmp" / "event-flags" /
                        "event_flags_canonical.json").read_text(encoding="utf-8"))
    by = {r["name"]: r["index"] for r in canon if r["name"]}
    EVENT_1ST_ROUTE22 = by.get("EVENT_1ST_ROUTE22_RIVAL_BATTLE")
    EVENT_2ND_ROUTE22 = by.get("EVENT_2ND_ROUTE22_RIVAL_BATTLE")
    EVENT_ROUTE22_WANTS = by.get("EVENT_ROUTE22_RIVAL_WANTS_BATTLE")


def mut_baseline(base): return bytes(base)


def mut_all_on(base):
    sav = bytearray(base)
    for i in range(EV_LEN):
        sav[EV_START + i] = 0xFF
    return sealed(sav)


def mut_route22_both(base):
    sav = bytearray(base)
    for idx in (EVENT_1ST_ROUTE22, EVENT_2ND_ROUTE22, EVENT_ROUTE22_WANTS):
        if idx is not None:
            set_bit(sav, idx)
    return sealed(sav)


def on_overworld(pyboy) -> bool:
    mem = pyboy.memory
    w, h = mem[W_CUR_MAP_WIDTH], mem[W_CUR_MAP_HEIGHT]
    if not (0 < w <= 64 and 0 < h <= 96):
        return False
    blocks = bytes(mem[W_OVERWORLD_MAP:W_OVERWORLD_MAP + (w + 6) * (h + 6)])
    screen = bytes(mem[W_TILEMAP:W_TILEMAP + SCREEN_TILES])
    return len(set(blocks)) > 1 and len(set(screen)) > 1


def boot(pyboy, budget=9000) -> bool:
    frames = 0
    while frames < budget and not on_overworld(pyboy):
        pyboy.button("start" if (frames // 24) % 2 == 0 else "a", delay=8)
        for _ in range(24):
            pyboy.tick()
        frames += 24
    return on_overworld(pyboy)


def run(sav: bytes, label: str) -> str:
    from pyboy import PyBoy
    (OUT / "rom.gb.ram").write_bytes(sav)
    # PyBoy loads <rom>.ram beside the rom; point it at our injected RAM.
    rom_copy = OUT / "rom.gb"
    rom_copy.write_bytes(ROM.read_bytes())
    (OUT / "rom.gb.ram").write_bytes(sav)
    pyboy = PyBoy(str(rom_copy), window="null", sound_emulated=False)
    ok = boot(pyboy)
    if not ok:
        pyboy.stop(save=False)
        return f"{label:28} -> NEVER reached overworld (crash/hang on load)"
    cur = pyboy.memory[W_CUR_MAP]
    # heartbeat: run 900 more frames, must stay on a sane overworld
    healthy = True
    for _ in range(900):
        pyboy.tick()
        if not on_overworld(pyboy):
            healthy = False
            break
    pyboy.stop(save=False)
    tag = "healthy overworld" if healthy else "DESTABILISED after load"
    return f"{label:28} -> reached overworld (map 0x{cur:02X}); {tag}"


def main():
    if not ROM.exists():
        raise SystemExit(f"ROM missing: {ROM} (place your own dump; git-ignored)")
    resolve_indices()
    base = BASE_SAV.read_bytes()
    print("Event-flag crash probe (Continue path; player starts in Pallet Town)\n")
    print(run(mut_baseline(base), "A baseline (control)"))
    print(run(mut_all_on(base), "B ALL flags on"))
    print(run(mut_route22_both(base),
              f"C Route22 rival 1ST+2ND+WANTS"))
    print("\nNote: BaseSAV loads in Pallet Town, so this tests the LOAD path. A")
    print("crash that only fires on a specific map/battle trigger (e.g. the Route")
    print("22 rival) needs the player driven onto that map — a further test.")


if __name__ == "__main__":
    main()
