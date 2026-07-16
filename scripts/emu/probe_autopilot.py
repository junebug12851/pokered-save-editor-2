#!/usr/bin/env python3
"""Console verification of the dev autopilot (navigate.py + autopilot.py).

Each case runs in its OWN drive_session.py child (one PyBoy per process, NDJSON
over pipes, per-command timeout, tree-kill on overrun — the sanctioned shape).
Everything is asserted from WRAM (map id + coords), never from pixels.

Cases:
  addrs      the sub-save WRAM addresses the autopilot trusts (wFontLoaded,
             wCurrentMenuItem, wGrassTile) — validated live before anything else
  walk       in-map A* walk across Pallet (target computed from our own grid)
  warp       Pallet -> Red's 1F -> back out (door + 'Last Map' mat resolution)
  cross      Pallet -> Route 1 -> Viridian City (connection crossings)
  mtmoon     Mt Moon 1F entrance -> B2F (the brief's named case: ladders,
             cave pair-collisions, cross-floor route)
  hunt       Route 1: find a wild battle pacing grass, then RUN from it
  talk       Pallet: talk to a WANDERING NPC (the moving-target chase)

Exit codes: 0 all pass · 1 failures · 2 unavailable (no ROM / no PyBoy).
Results: tmp/emu/autopilot-verdicts.json.  Usage: [--only case] [--list]
"""
from __future__ import annotations

import argparse
import json
import queue
import subprocess
import sys
import threading
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
SESSION = REPO / "scripts" / "emu" / "drive_session.py"
OUT = REPO / "tmp" / "emu" / "autopilot-verdicts.json"

sys.path.insert(0, str(Path(__file__).resolve().parent))
from navigate import World, astar  # noqa: E402


class Child:
    """One owned drive_session child. Every send has a hard timeout."""

    def __init__(self):
        self.p = subprocess.Popen([sys.executable, str(SESSION)], cwd=str(REPO),
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.DEVNULL, text=True,
                                  encoding="utf-8")
        self.q: queue.Queue = queue.Queue()

        def reader():
            for line in self.p.stdout:
                line = line.strip()
                if line.startswith("{"):
                    try:
                        self.q.put(json.loads(line))
                    except json.JSONDecodeError:
                        pass
            self.q.put({"ok": False, "error": "child ended"})

        threading.Thread(target=reader, daemon=True).start()
        ready = self.q.get(timeout=60)
        if not ready.get("ready"):
            raise RuntimeError(f"session child unavailable: {ready}")

    def send(self, obj: dict, timeout_s: float = 300) -> dict:
        self.p.stdin.write(json.dumps(obj) + "\n")
        self.p.stdin.flush()
        try:
            return self.q.get(timeout=timeout_s)
        except queue.Empty:
            self.kill()
            return {"ok": False, "error": f"no answer in {timeout_s}s (killed)"}

    def kill(self):
        if self.p.poll() is None:
            subprocess.run(["taskkill", "/PID", str(self.p.pid), "/T", "/F"],
                           capture_output=True)

    def close(self):
        try:
            self.send({"cmd": "quit"}, 10)
        finally:
            self.kill()


def ensure_map_base(map_id: int) -> None:
    cache = REPO / "tmp" / "emu" / "map-saves" / f"map{map_id:03d}.sav"
    if cache.exists():
        return
    r = subprocess.run([sys.executable, str(REPO / "scripts" / "emu" / "forge_map_save.py"),
                        "--map", str(map_id)], capture_output=True, text=True,
                       timeout=420, cwd=str(REPO))
    if r.returncode != 0 or not cache.exists():
        raise RuntimeError(f"map base {map_id} generation failed:\n{r.stdout}{r.stderr}")


def boot(c: Child, **kw) -> dict:
    r = c.send({"cmd": "boot", **kw}, 180)
    if not (r.get("ok") and r.get("booted")):
        raise RuntimeError(f"boot failed: {r}")
    return r


# ------------------------------------------------------------------ the cases

def case_addrs(w: World) -> dict:
    """The autopilot's sub-save addresses, validated on the live console."""
    c = Child()
    try:
        boot(c, sav=None)
        font0 = int(c.send({"cmd": "mem", "addr": "0xCFC4"})["hex"], 16) & 1
        c.send({"cmd": "button", "b": "start", "hold": 6, "after": 60})
        font1 = int(c.send({"cmd": "mem", "addr": "0xCFC4"})["hex"], 16) & 1
        m0 = int(c.send({"cmd": "mem", "addr": "0xCC26"})["hex"], 16)
        c.send({"cmd": "button", "b": "down", "hold": 4, "after": 30})
        m1 = int(c.send({"cmd": "mem", "addr": "0xCC26"})["hex"], 16)
        c.send({"cmd": "button", "b": "b", "hold": 4, "after": 40})
        # grass tile: Pallet is OUTDOOR; wGrassTile is loaded from its header
        grass = int(c.send({"cmd": "mem", "addr": "0xD535"})["hex"], 16)
        ok = (font0 == 0 and font1 == 1 and m1 != m0 and 0 < grass < 255)
        return {"ok": ok, "font_closed": font0, "font_menu": font1,
                "menu_before": m0, "menu_after_down": m1, "grass_tile": grass}
    finally:
        c.close()


def _far_walkable(w: World, ind: int, sx: int, sy: int) -> tuple[int, int]:
    """The farthest square our own A* can reach from (sx, sy) — a guaranteed-
    reachable, non-warp target computed from the same data the walker uses."""
    g = w.grid(ind)
    e = w.by_ind[ind]
    warps = {(wp["x"], wp["y"]) for wp in e.get("warpOut") or []}
    best, best_d = (sx, sy), -1
    for y in range(g.h2):
        for x in range(g.w2):
            if not g.passable[y][x] or (x, y) in warps:
                continue
            d = abs(x - sx) + abs(y - sy)
            if d > best_d and astar(g, (sx, sy), (x, y),
                                    frozenset(warps)) is not None:
                best, best_d = (x, y), d
    return best


def case_walk(w: World) -> dict:
    c = Child()
    try:
        r = boot(c, sav=None)
        st = r["state"]
        tx, ty = _far_walkable(w, st["map"], st["x"], st["y"])
        res = c.send({"cmd": "walk_to", "x": tx, "y": ty}, 420)["result"]
        ok = res.get("ok") and res["x"] == tx and res["y"] == ty
        return {"ok": bool(ok), "target": [tx, ty], "result": res}
    finally:
        c.close()


def case_warp(w: World) -> dict:
    c = Child()
    try:
        boot(c, sav=None)
        reds = w.resolve("Reds House 1F")["ind"]
        pallet = w.resolve("Pallet Town")["ind"]
        r1 = c.send({"cmd": "goto", "map": reds}, 420)["result"]
        r2 = c.send({"cmd": "goto", "map": pallet}, 420)["result"]
        ok = r1.get("ok") and r1["map"] == reds and \
            r2.get("ok") and r2["map"] == pallet
        return {"ok": bool(ok), "into": r1, "out": r2}
    finally:
        c.close()


def case_cross(w: World) -> dict:
    c = Child()
    try:
        boot(c, sav=None)
        viridian = w.resolve("Viridian City")["ind"]
        r = c.send({"cmd": "goto", "map": viridian}, 900)["result"]
        return {"ok": bool(r.get("ok") and r["map"] == viridian), "result": r}
    finally:
        c.close()


def case_mtmoon(w: World) -> dict:
    m1f = w.resolve("Mt Moon 1F")
    b2f = w.resolve("Mt Moon B2F")
    ensure_map_base(m1f["ind"])
    c = Child()
    try:
        boot(c, sav=str(REPO / "tmp" / "emu" / "map-saves" / f"map{m1f['ind']:03d}.sav"))
        r = c.send({"cmd": "goto", "map": b2f["ind"], "on_battle": "run"},
                   1200)["result"]
        return {"ok": bool(r.get("ok") and r["map"] == b2f["ind"]), "result": r}
    finally:
        c.close()


def case_hunt(w: World) -> dict:
    """Route 1 is one of the 8 warpless routes — no base needed: the autopilot
    just WALKS there from Pallet (which is itself part of the test)."""
    r1 = w.resolve("Route 1")
    c = Child()
    try:
        boot(c, sav=None)
        g = c.send({"cmd": "goto", "map": r1["ind"]}, 600)["result"]
        if not g.get("ok"):
            return {"ok": False, "reason": "never reached Route 1", "goto": g}
        r = c.send({"cmd": "hunt", "max_steps": 400, "policy": "run"},
                   900)["result"]
        st = c.send({"cmd": "state"})["state"]
        ok = r.get("ok") and r.get("enemy", {}).get("level", 0) > 0 \
            and st["battle"] == 0
        return {"ok": bool(ok), "result": r, "after": st}
    finally:
        c.close()


def case_talk(w: World) -> dict:
    """Pallet's wandering NPC — the moving-target chase."""
    c = Child()
    try:
        boot(c, sav=None)
        e = w.resolve("Pallet Town")
        walkers = [i + 1 for i, sp in enumerate(e.get("sprites") or [])
                   if sp.get("move") == "Walk"]
        target = walkers[0] if walkers else 1
        r = c.send({"cmd": "talk", "target": target}, 420)["result"]
        return {"ok": bool(r.get("ok")), "slot": target, "result": r}
    finally:
        c.close()


CASES = {"addrs": case_addrs, "walk": case_walk, "warp": case_warp,
         "cross": case_cross, "mtmoon": case_mtmoon, "hunt": case_hunt,
         "talk": case_talk}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--only", default="", help="run one case")
    ap.add_argument("--list", action="store_true")
    a = ap.parse_args()
    if a.list:
        print(" ".join(CASES))
        return 0
    if not ROM.exists():
        sys.stderr.write("no ROM at assets/references/backup.gb (local-only)\n")
        return 2

    w = World()
    names = [a.only] if a.only else list(CASES)
    verdicts = {}
    t0 = time.time()
    for name in names:
        if name not in CASES:
            sys.stderr.write(f"no case {name}\n")
            return 1
        print(f"== {name}", flush=True)
        try:
            v = CASES[name](w)
        except Exception as e:
            v = {"ok": False, "error": repr(e)}
        verdicts[name] = v
        print(f"   {'PASS' if v.get('ok') else 'FAIL'}  "
              f"{json.dumps({k: v[k] for k in v if k != 'result'}, default=str)[:300]}",
              flush=True)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(verdicts, indent=1, default=str))
    n_ok = sum(1 for v in verdicts.values() if v.get("ok"))
    print(f"\n{n_ok}/{len(verdicts)} passed in {time.time() - t0:.0f}s -> {OUT}")
    return 0 if n_ok == len(verdicts) else 1


if __name__ == "__main__":
    raise SystemExit(main())
