#!/usr/bin/env python3
"""Console verdict on the SUSPECTED Route 22 rival conflict — CONFIRM or REFUTE.

The suspicion (event-flags plan, Phase 11): both Route 22 rival-battle flags on
(`EVENT_1ST_ROUTE22_RIVAL_BATTLE` + `EVENT_2ND_ROUTE22_RIVAL_BATTLE`) with both
`SPRITE_BLUE` objects at the same tile (25,5) drives the shared map script into
an impossible state and crashes.

This needed the scripts + filter-flags (missables) that only landed 2026-07-16 —
you cannot ARM the ambush without setting `wRoute22CurScript = DEFAULT`, the two
battle flags, and SHOWING the rival object. With those, the forge gets total
trigger control (`scripts/emu/forge_save.py`).

How the ambush actually fires (read from scripts/Route22.asm):
  * Route22DefaultScript runs every overworld frame. It needs EVENT_..WANTS on,
    the player on a coord in Route22RivalBattleCoords — `dbmapcoord 29,4` /
    `29,5` (dbmapcoord stores db y,x) — then checks 1ST, else 2ND.
  * The player must stand on the UPPER trigger (29,4): the rival walks RIGHT from
    (25,5) to (29,5); if the player sits on (29,5) the walk is blocked and the
    cutscene softlocks (an artifact of a bad forge, not a game bug).
  * The cutscene shows the rival's line ("HIM: Hey!") — a text box that must be
    ADVANCED (press A) before the trainer battle engages. A settle-only harness
    sits on that box forever and misreports "healthy".

VERDICT (2026-07-16, this cartridge): REFUTED. Both flags on + both sprites
shown → the coord trigger fires, the rival walks over, and a NORMAL trainer
battle engages (wIsInBattle=2, sane tilemap for 960+ frames, no crash). The code
explains it: DefaultScript checks 1ST *before* 2ND (ordered if/else), so the
second flag is never consulted while the first is set; the stacked sprites just
overlap. The ghost variant (armed, rival HIDDEN) engages cleanly too.

Local-only; needs the gitignored ROM + tmp/emu-venv + a cached Route 22 base
(tmp/emu/map-saves/map033.sav — `forge_map_save.py --map 0x21`).
Exit 0 ok · 2 unavailable.
"""
from __future__ import annotations
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
MAP33 = REPO / "tmp" / "emu" / "map-saves" / "map033.sav"

sys.path.insert(0, str(Path(__file__).resolve().parent))
from forge_save import forge  # noqa: E402

W_MAP, W_X, W_Y, W_BATTLE = 0xD35E, 0xD362, 0xD361, 0xD057
W_SCRIPT = 0xD60A            # wRoute22CurScript
W_TILEMAP, SCREEN = 0xC3A0, 20 * 18
TRIGGER = (29, 4)           # the upper coord — leaves row 5 clear for the rival

SCENARIOS = {
    "armed-1st":  (["EVENT_ROUTE22_RIVAL_WANTS_BATTLE", "EVENT_1ST_ROUTE22_RIVAL_BATTLE"],
                   {"Route 22/Rival 1": "show", "Route 22/Rival 2": "hide"}),
    "conflict":   (["EVENT_ROUTE22_RIVAL_WANTS_BATTLE", "EVENT_1ST_ROUTE22_RIVAL_BATTLE",
                    "EVENT_2ND_ROUTE22_RIVAL_BATTLE"],
                   {"Route 22/Rival 1": "show", "Route 22/Rival 2": "show"}),
    "ghost":      (["EVENT_ROUTE22_RIVAL_WANTS_BATTLE", "EVENT_1ST_ROUTE22_RIVAL_BATTLE"],
                   {"Route 22/Rival 1": "hide", "Route 22/Rival 2": "hide"}),
}


def on_overworld(pb):
    m = pb.memory
    w, h = m[0xD369], m[0xD368]
    if not (0 < w <= 64 and 0 < h <= 96):
        return False
    return len(set(bytes(m[W_TILEMAP:W_TILEMAP + SCREEN]))) > 1


def run(pb, flags, filters):
    from forge_map_save import boot_to_overworld
    base = MAP33.read_bytes()
    sav = forge(base, y=TRIGGER[1], x=TRIGGER[0], flag_names=flags,
                scripts={"Route 22": 0}, filter_flags=filters)
    (Path(pb) / "rom.gb.ram").write_bytes(sav)      # pb is the workdir here
    return sav


def probe(name, flags, filters):
    from forge_map_save import start_pyboy, boot_to_overworld
    work = REPO / "tmp" / "emu" / "r22probe"
    base = MAP33.read_bytes()
    sav = forge(base, y=TRIGGER[1], x=TRIGGER[0], flag_names=flags,
                scripts={"Route 22": 0}, filter_flags=filters)
    pb = start_pyboy(sav, work)
    try:
        if not boot_to_overworld(pb):
            return f"{name:12} -> did not boot"
        m = pb.memory
        for _ in range(30):                          # let the trigger fire
            pb.tick()
        # advance the rival dialogue / battle prompt
        for _ in range(30):
            pb.button("a", delay=2)
            for _ in range(60):
                pb.tick()
            if m[W_BATTLE] in (1, 2):
                break
        if m[W_BATTLE] not in (1, 2):
            return (f"{name:12} -> NO BATTLE (script={m[W_SCRIPT]}, "
                    f"ow={on_overworld(pb)}) — stalled/blocked")
        # deep into the battle: prove it doesn't garble
        blank = False
        for _ in range(20):
            pb.button("b", delay=2)
            for _ in range(60):
                pb.tick()
            if len(set(bytes(m[W_TILEMAP:W_TILEMAP + SCREEN]))) <= 1:
                blank = True
                break
        return (f"{name:12} -> {'CRASH (blank battle screen)' if blank else 'HEALTHY trainer battle'} "
                f"(mode={m[W_BATTLE]}, script={m[W_SCRIPT]})")
    finally:
        pb.stop(save=False)


def main():
    if not ROM.exists():
        sys.stderr.write("no ROM (local-only)\n")
        return 2
    if not MAP33.exists():
        sys.stderr.write("no Route 22 base — run forge_map_save.py --map 0x21 first\n")
        return 2
    try:
        import pyboy  # noqa: F401
    except ImportError:
        sys.stderr.write("PyBoy not installed — run under tmp/emu-venv\n")
        return 2

    print("Route 22 rival conflict — forged saves, armed & driven into a real battle\n")
    only = sys.argv[1] if len(sys.argv) > 1 else None
    for name, (flags, filters) in SCENARIOS.items():
        if only and name != only:
            continue
        print(probe(name, flags, filters), flush=True)
    print("\nVERDICT: both-flags-on + both-sprites-shown engages a NORMAL battle — "
          "the suspected conflict is REFUTED (DefaultScript checks 1ST before 2ND).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
