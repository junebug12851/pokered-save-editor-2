#!/usr/bin/env python3
"""Interactive PyBoy session child — ONE PyBoy, ONE process, driven over stdin/stdout.

This is the sanctioned shape for *interactive* emulator driving (the counterpart of the
single-shot probes). The lessons it encodes, from notes/reference/emulator-verification.md:

  * PyBoy CANNOT be re-instantiated in one process (the 2nd instance wedges) — so this
    process boots exactly ONCE. A second `boot` command is refused; the owner kills this
    process and spawns a fresh one instead.
  * Every loop is FRAME-BOUNDED — nothing in here can spin forever.
  * The process is designed to be OWNED: a parent (the dev MCP server, QProcess, etc.)
    holds the pipes, applies its own per-command timeout, and kills the whole tree on
    any overrun. EOF on stdin => clean stop + exit. atexit stops the emulator.
  * PyBoy reads battery RAM from `<rom>.gb.ram` BESIDE THE ROM — the sav is copied there.

Protocol: newline-delimited JSON. One object in => one object out (always with "ok").

  {"cmd":"boot","sav":"path"|null,"map":int?,"x":int?,"y":int?,
   "flags":["EVENT_..."]?,"flag_indices":[int]?,"all_flags":bool?,
   "pokes":{"0x260A":"0x21"}?,"budget":4000?,"mash":true?}
        -> forge (optional) + boot to overworld. mash=false: just run `budget` frames.
  {"cmd":"tick","n":60}                     -> advance n frames
  {"cmd":"button","b":"a","hold":4,"after":30}
        -> press b for `hold` frames (PyBoy delay), then run `after` frames
  {"cmd":"mem","addr":"0xD35E","len":16}    -> {"hex": "..."} (WRAM/HRAM/anywhere)
  {"cmd":"poke","addr":"0xD35E","bytes":"21FF"} -> write bytes into memory
  {"cmd":"state"}                           -> map/dims/coords/battle/on_overworld
  {"cmd":"shot","path":"tmp/emu/x.png"}     -> save the 160x144 framebuffer PNG
  {"cmd":"quit"}                            -> stop + exit 0

Autopilot commands (autopilot.py + navigate.py — pathfinding over our own shipped
map data, every step verified against WRAM; see notes/plans/dev-autopilot.md):

  {"cmd":"walk_to","x":10,"y":20,"on_battle":"run","on_trainer":"stop"}
  {"cmd":"goto","map":"Mt Moon B1F"|60,"x":..?,"y":..?}   -> cross-map navigation
  {"cmd":"battle","policy":"mash"|"run"|"move:2"}
  {"cmd":"hunt","max_steps":240,"policy":"stop"}          -> find a wild battle
  {"cmd":"talk","target":"Bug Catcher"|3,"dismiss":true}  -> talk to a (moving) NPC
  {"cmd":"dismiss"}                                       -> clear text boxes
  {"cmd":"script","steps":[{...},...],"stop_on_error":true} -> a whole run, one call

Exit codes: 0 ok · 2 unavailable (no ROM / no PyBoy — same contract as dump_state.py).
"""
from __future__ import annotations
import atexit
import json
import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"

sys.path.insert(0, str(Path(__file__).resolve().parent))
from forge_save import forge  # noqa: E402  (shared forge + checksum reseal)
import autopilot  # noqa: E402  (pathfinding walker/battler — dev-autopilot.md)

W_MAP, W_W, W_H, W_Y, W_X, W_BATTLE = 0xD35E, 0xD369, 0xD368, 0xD361, 0xD362, 0xD057
W_OVERWORLD, W_TILEMAP = 0xC6E8, 0xC3A0
SCREEN = 20 * 18

_pb = {"pb": None}
atexit.register(lambda: _pb["pb"] and _pb["pb"].stop(save=False))


def on_overworld(pb) -> bool:
    m = pb.memory
    w, h = m[W_W], m[W_H]
    if not (0 < w <= 64 and 0 < h <= 96):
        return False
    blocks = bytes(m[W_OVERWORLD:W_OVERWORLD + (w + 6) * (h + 6)])
    screen = bytes(m[W_TILEMAP:W_TILEMAP + SCREEN])
    return len(set(blocks)) > 1 and len(set(screen)) > 1


def state(pb) -> dict:
    m = pb.memory
    return {"map": m[W_MAP], "w": m[W_W], "h": m[W_H], "y": m[W_Y], "x": m[W_X],
            "battle": m[W_BATTLE], "on_overworld": on_overworld(pb),
            "facing": autopilot.facing(m), "last_map": m[autopilot.W_LASTMAP],
            "text_box": autopilot.font_up(m)}


def _int(v) -> int:
    return int(v, 0) if isinstance(v, str) else int(v)


def do_boot(req: dict) -> dict:
    if _pb["pb"] is not None:
        return {"ok": False, "error": "already booted — PyBoy cannot be re-instantiated "
                                      "in one process; kill this session and start a new one"}
    from pyboy import PyBoy

    work = REPO / "tmp" / "emu" / "session"
    work.mkdir(parents=True, exist_ok=True)
    rom = work / "rom.gb"
    shutil.copyfile(ROM, rom)

    sav_path = Path(req["sav"]) if req.get("sav") else BASE_SAV
    base = sav_path.read_bytes()
    needs_forge = any(k in req for k in ("map", "x", "y", "flags", "flag_indices",
                                         "all_flags", "pokes", "scripts", "filter_flags"))
    if needs_forge:
        pokes = {_int(k): _int(v) for k, v in (req.get("pokes") or {}).items()}
        base = forge(base, req.get("map"), req.get("y"), req.get("x"),
                     req.get("flags"), req.get("flag_indices"),
                     bool(req.get("all_flags")), pokes,
                     scripts=req.get("scripts"), filter_flags=req.get("filter_flags"))
    (work / "rom.gb.ram").write_bytes(base)

    pb = PyBoy(str(rom), window="null", sound_emulated=False)
    _pb["pb"] = pb

    budget = int(req.get("budget", 4000))
    f = 0
    if req.get("mash", True):
        while f < budget and not on_overworld(pb):
            pb.button("start" if (f // 24) % 2 == 0 else "a", delay=8)
            for _ in range(24):
                pb.tick()
            f += 24
    else:
        for _ in range(budget):
            pb.tick()
        f = budget
    return {"ok": True, "frames": f, "booted": on_overworld(pb), "state": state(pb)}


def handle(req: dict) -> dict:
    cmd = req.get("cmd")
    if cmd == "boot":
        return do_boot(req)
    pb = _pb["pb"]
    if cmd == "quit":
        return {"ok": True, "quit": True}
    if pb is None:
        return {"ok": False, "error": "not booted — send boot first"}
    if cmd == "tick":
        n = min(int(req.get("n", 60)), 36000)          # hard bound: <= 10 emu-minutes
        for _ in range(n):
            pb.tick()
        return {"ok": True, "state": state(pb)}
    if cmd == "button":
        pb.button(req["b"], delay=min(int(req.get("hold", 4)), 600))
        for _ in range(min(int(req.get("after", 30)), 36000)):
            pb.tick()
        return {"ok": True, "state": state(pb)}
    if cmd == "mem":
        addr, n = _int(req["addr"]), min(_int(req.get("len", 1)), 0x4000)
        return {"ok": True, "addr": addr, "hex": bytes(pb.memory[addr:addr + n]).hex()}
    if cmd == "poke":
        addr, data = _int(req["addr"]), bytes.fromhex(req["bytes"])
        for i, b in enumerate(data):
            pb.memory[addr + i] = b
        return {"ok": True, "wrote": len(data)}
    if cmd == "state":
        return {"ok": True, "state": state(pb)}
    if cmd == "shot":
        p = Path(req["path"])
        p.parent.mkdir(parents=True, exist_ok=True)
        pb.screen.image.save(str(p))
        return {"ok": True, "path": str(p)}
    # ---- autopilot verbs (each internally frame-bounded; the parent's own
    #      per-command timeout + tree-kill stays the outer safety net) ----
    if cmd == "walk_to":
        return {"ok": True, "result": autopilot.walk_to(
            pb, int(req["x"]), int(req["y"]),
            on_battle=req.get("on_battle", "run"),
            on_trainer=req.get("on_trainer", "stop"),
            max_steps=min(int(req.get("max_steps", 800)), 4000),
            surf=bool(req.get("surf", False)), cut=bool(req.get("cut", False)))}
    if cmd == "goto":
        return {"ok": True, "result": autopilot.goto(
            pb, req["map"], int(req.get("x", -1)), int(req.get("y", -1)),
            on_battle=req.get("on_battle", "run"),
            on_trainer=req.get("on_trainer", "stop"),
            unblock=bool(req.get("unblock", True)),
            bike=bool(req.get("bike", True)),
            surf=req.get("surf", "auto"), cut=req.get("cut", "auto"))}
    if cmd == "set_flag":
        return {"ok": True, "result": autopilot.set_event_flag(
            pb, req["flag"], bool(req.get("on", True)))}
    if cmd == "give_item":
        return {"ok": True, "result": autopilot.give_item(
            pb, int(req["item"]), int(req.get("qty", 1)))}
    if cmd == "move_sprite":
        return {"ok": True, "result": autopilot.move_sprite(
            pb, int(req["slot"]), int(req["x"]), int(req["y"]))}
    if cmd == "battle":
        return {"ok": True, "result": autopilot.battle(
            pb, req.get("policy", "mash"))}
    if cmd == "hunt":
        return {"ok": True, "result": autopilot.hunt(
            pb, max_steps=min(int(req.get("max_steps", 240)), 2000),
            policy=req.get("policy", "stop"))}
    if cmd == "talk":
        return {"ok": True, "result": autopilot.talk_to(
            pb, req["target"], dismiss_text=bool(req.get("dismiss", True)))}
    if cmd == "dismiss":
        return {"ok": True, "result": autopilot.dismiss(pb)}
    if cmd == "script":
        results = []
        stop_on_error = bool(req.get("stop_on_error", True))
        for i, sub in enumerate(req.get("steps") or []):
            if sub.get("cmd") in ("boot", "quit", "script"):
                results.append({"ok": False, "error": f"step {i}: {sub.get('cmd')} "
                                                      f"not allowed in a script"})
                if stop_on_error:
                    break
                continue
            r = handle(sub)
            results.append(r)
            failed = not r.get("ok") or (isinstance(r.get("result"), dict)
                                         and r["result"].get("ok") is False)
            if failed and stop_on_error:
                break
        return {"ok": all(r.get("ok") for r in results), "steps": results,
                "state": state(pb)}
    return {"ok": False, "error": f"unknown cmd: {cmd}"}


def main() -> int:
    if not ROM.exists():
        sys.stderr.write("no ROM at assets/references/backup.gb (local-only)\n")
        return 2
    try:
        import pyboy  # noqa: F401
    except ImportError:
        sys.stderr.write("PyBoy not installed — run under tmp/emu-venv\n")
        return 2

    print(json.dumps({"ok": True, "ready": True}), flush=True)
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            req = json.loads(line)
            resp = handle(req)
        except Exception as e:                          # a bad command never kills the session
            resp = {"ok": False, "error": repr(e)}
        print(json.dumps(resp), flush=True)
        if resp.get("quit"):
            break
    pb = _pb["pb"]
    if pb is not None:
        pb.stop(save=False)
        _pb["pb"] = None
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
