#!/usr/bin/env python3
"""Event-flag scenario TEST RUNNER — single-shot, self-terminating, robust.

The reliable pattern (same as scripts/emu/dump_state.py, which tst_emu_parity
drives): boot PyBoy in ONE process, do the work, write results to a file, and
EXIT. No persistent session, no IPC, no interactive driving — those are what made
earlier attempts fragile (accumulating zombie processes, poll races). Each
scenario gets its OWN PyBoy instance that is ALWAYS stopped in a `finally`, and
every loop is frame-bounded so nothing can hang.

Reads a scenarios JSON, forges a save for each (any map/pos/flags), boots on
Continue, optionally drives the player, and classifies the outcome:
    healthy  — reached & stayed on a sane overworld / battle
    crash    — went to garbage (insane map dims / dead tilemap) => a real crash
    no-boot  — never reached the overworld (crash on load, or bad forge)
    hang     — assigned BY THE OWNER (tst_flag_scenarios / the dev MCP server)
               when this process never returns: a hard-crashed console executes
               STOP, the clocks halt, the frame never completes, and PyBoy's
               tick() spins forever (reproduced 2026-07-16). This process cannot
               classify its own wedge — the owner's timeout + TREE-kill is the
               watchdog (the venv python.exe is a launcher; kill its tree, not it).

⚠️ Forging ONLY wCurMap onto a different map makes a CHIMERA (the Area block —
dims/tileset/scripts — still belongs to the old map): it Continues onto the new
map id and wedges within ~100 frames. Cross-map scenarios therefore report
"hang" until a consistent whole-Area forge exists. Same-map forges (flags,
coords) are fully consistent. See notes/reference/emulator-verification.md.

Exit codes: 0 ok · 2 unavailable (no ROM / not run under the emu venv).

Usage (run under tmp/emu-venv/Scripts/python.exe):
    python run_flag_scenarios.py [--scenarios scripts/emu/flag_scenarios.json]
                                 [--out tmp/emu/flag_scenarios_result.json]
"""
from __future__ import annotations
import argparse
import atexit
import json
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
CANON = REPO / "tmp" / "event-flags" / "event_flags_canonical.json"

EV_START, EV_LEN = 0x29F3, 0x140
SAV_CUR_MAP, SAV_Y, SAV_X = 0x260A, 0x260D, 0x260E
CK, CK0, CKN = 0x3523, 0x2598, 0xF8B

W_MAP, W_W, W_H, W_Y, W_X, W_BATTLE = 0xD35E, 0xD369, 0xD368, 0xD361, 0xD362, 0xD057
W_OVERWORLD, W_TILEMAP = 0xC6E8, 0xC3A0
SCREEN = 20 * 18

_LIVE = {"pb": None}
atexit.register(lambda: _LIVE["pb"] and _LIVE["pb"].stop(save=False))


def name_to_index():
    if not CANON.exists():
        return {}
    return {r["name"]: r["index"] for r in json.loads(CANON.read_text(encoding="utf-8"))
            if r["name"]}


def reseal(sav):
    c = 0xFF
    for i in range(CK0, CK0 + CKN):
        c = (c - sav[i]) & 0xFF
    sav[CK] = c
    return bytes(sav)


def forge(base, names, name_idx, mapid, y, x, all_on):
    sav = bytearray(base)
    if all_on:
        for i in range(EV_LEN):
            sav[EV_START + i] = 0xFF
    for nm in names:
        i = name_idx.get(nm)
        if i is not None:
            sav[EV_START + i // 8] |= (1 << (i % 8))
    if mapid is not None:
        sav[SAV_CUR_MAP] = mapid
        sav[SAV_Y] = y
        sav[SAV_X] = x
    return reseal(sav)


def on_overworld(pb):
    m = pb.memory
    w, h = m[W_W], m[W_H]
    if not (0 < w <= 64 and 0 < h <= 96):
        return False
    blocks = bytes(m[W_OVERWORLD:W_OVERWORLD + (w + 6) * (h + 6)])
    screen = bytes(m[W_TILEMAP:W_TILEMAP + SCREEN])
    return len(set(blocks)) > 1 and len(set(screen)) > 1


def sane(pb):
    if on_overworld(pb):
        return True
    m = pb.memory
    if m[W_BATTLE] in (1, 2):
        return len(set(bytes(m[W_TILEMAP:W_TILEMAP + SCREEN]))) > 1
    return False


def boot(pb, budget=4000):
    f = 0
    while f < budget and not on_overworld(pb):
        pb.button("start" if (f // 24) % 2 == 0 else "a", delay=8)
        for _ in range(24):
            pb.tick()
        f += 24
    return on_overworld(pb), f


def run_scenario(sc, base, name_idx, PyBoy, out_rom):
    sav = forge(base, sc.get("flags", []), name_idx,
                (int(sc["map"], 16) if isinstance(sc.get("map"), str) else sc.get("map")),
                sc.get("y", 8), sc.get("x", 8), sc.get("flags") == "ALL")
    (out_rom.parent / "rom.gb.ram").write_bytes(sav)
    pb = PyBoy(str(out_rom), window="null", sound_emulated=False)
    _LIVE["pb"] = pb
    try:
        booted, frames = boot(pb)
        if not booted:
            return {"name": sc["name"], "result": "no-boot", "frames": frames}
        entered_battle = False
        crash_step = None
        for spec in sc.get("drive", []):
            d, n = spec.split(":")
            for step in range(int(n)):
                pb.button(d, delay=4)
                for _ in range(10):
                    pb.tick()
                if pb.memory[W_BATTLE] in (1, 2):
                    entered_battle = True
                if not sane(pb):
                    crash_step = f"{d}:{step}"
                    break
            if crash_step:
                break
        # settle + final health check
        for _ in range(120):
            pb.tick()
        result = "healthy" if sane(pb) and crash_step is None else "crash"
        return {"name": sc["name"], "result": result, "map": pb.memory[W_MAP],
                "battle": entered_battle, "crash_at": crash_step,
                "expect": sc.get("expect")}
    finally:
        pb.stop(save=False)
        _LIVE["pb"] = None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--scenarios", default=str(REPO / "scripts" / "emu" / "flag_scenarios.json"))
    ap.add_argument("--out", default=str(REPO / "tmp" / "emu" / "flag_scenarios_result.json"))
    ap.add_argument("--only", default=None,
                    help="run just this scenario by name (PyBoy is not safe to "
                         "re-instantiate in one process, so batches are best run "
                         "one scenario per process)")
    a = ap.parse_args()

    if not ROM.exists():
        sys.stderr.write("no ROM at assets/references/backup.gb (local-only)\n")
        return 2
    try:
        from pyboy import PyBoy
    except ImportError:
        sys.stderr.write("PyBoy not installed — run under tmp/emu-venv\n")
        return 2

    out = Path(a.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out_rom = out.parent / "rom.gb"
    out_rom.write_bytes(ROM.read_bytes())

    scenarios = json.loads(Path(a.scenarios).read_text(encoding="utf-8"))
    if a.only:
        scenarios = [s for s in scenarios if s.get("name") == a.only]
    base = BASE_SAV.read_bytes()
    name_idx = name_to_index()

    results = []
    started = time.time()
    for sc in scenarios:
        try:
            r = run_scenario(sc, base, name_idx, PyBoy, out_rom)
        except Exception as e:  # one bad scenario never kills the batch
            r = {"name": sc.get("name", "?"), "result": "error", "error": repr(e)}
        results.append(r)
        out.write_text(json.dumps({"elapsed": round(time.time() - started, 1),
                                   "results": results}, indent=1))
        print(r, flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
