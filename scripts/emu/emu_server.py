"""Persistent PyBoy session with file IPC — boot the ROM ONCE, then drive it
with cheap commands across many short calls (works around per-call time caps).

Start (detached):
    python emu_server.py <map_hex> <y> <x> <flag1,flag2,...>
It forges a save at that map/pos with those event flags set, boots to the
overworld, then serves commands from tmp/emu-ipc/cmd.txt, writing JSON results
to tmp/emu-ipc/out.txt:

    read              -> current state (map/pos/battle/health + a few sprites)
    step <dir> <n>    -> press dir n times, then state
    quit              -> stop

The client (PowerShell) deletes out.txt, writes a command to cmd.txt, and polls
out.txt until it reappears — each round-trip is milliseconds.

Local-only; needs the gitignored ROM + tmp/emu-venv PyBoy venv.
"""
from __future__ import annotations
import json
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from probe_route22_conflict import forge, on_overworld, sane, OUT, ROM  # noqa

IPC = Path(__file__).resolve().parents[2] / "tmp" / "emu-ipc"
IPC.mkdir(parents=True, exist_ok=True)
CMD = IPC / "cmd.txt"
RES = IPC / "out.txt"
HEARTBEAT = IPC / "alive.txt"

W_CUR_MAP, W_W, W_H, W_Y, W_X, W_BATTLE = 0xD35E, 0xD369, 0xD368, 0xD361, 0xD362, 0xD057
SS1 = 0xC100   # wSpriteStateData1: +4 screenY px, +6 screenX px (16 bytes/sprite)


def dump(pb):
    m = pb.memory
    sprites = []
    for n in range(1, 8):
        base = SS1 + n * 16
        pic = m[base + 0]
        if pic:
            sprites.append({"n": n, "pic": pic, "sy": m[base + 4], "sx": m[base + 6]})
    return {
        "map": m[W_CUR_MAP], "w": m[W_W], "h": m[W_H],
        "Y": m[W_Y], "X": m[W_X], "battle": m[W_BATTLE],
        "onOW": on_overworld(pb), "sane": sane(pb), "sprites": sprites,
    }


def main():
    map_hex = int(sys.argv[1], 16) if len(sys.argv) > 1 else 0x21
    y = int(sys.argv[2]) if len(sys.argv) > 2 else 8
    x = int(sys.argv[3]) if len(sys.argv) > 3 else 25
    flags = [f for f in (sys.argv[4].split(",") if len(sys.argv) > 4 else []) if f]

    from pyboy import PyBoy
    sav = bytearray(forge(flags, y, x))
    sav[0x260A] = map_hex
    # reseal after the map override
    c = 0xFF
    for i in range(0x2598, 0x2598 + 0xF8B):
        c = (c - sav[i]) & 0xFF
    sav[0x3523] = c
    (OUT / "rom.gb").write_bytes(ROM.read_bytes())
    (OUT / "rom.gb.ram").write_bytes(bytes(sav))
    pb = PyBoy(str(OUT / "rom.gb"), window="null", sound_emulated=False)
    f = 0
    while f < 4000 and not on_overworld(pb):
        pb.button("start" if (f // 24) % 2 == 0 else "a", delay=8)
        for _ in range(24):
            pb.tick()
        f += 24
    RES.write_text(json.dumps({"booted": on_overworld(pb), "frames": f, **dump(pb)}))

    while True:
        HEARTBEAT.write_text(str(time.time()))
        if CMD.exists():
            try:
                cmd = CMD.read_text().strip()
                CMD.unlink()
            except OSError:
                time.sleep(0.05)
                continue
            if not cmd:
                continue
            parts = cmd.split()
            if parts[0] == "quit":
                RES.write_text(json.dumps({"quit": True}))
                pb.stop(save=False)
                return
            if parts[0] == "step":
                d = parts[1] if len(parts) > 1 else "up"
                n = int(parts[2]) if len(parts) > 2 else 1
                for _ in range(n):
                    pb.button(d, delay=4)
                    for _ in range(10):
                        pb.tick()
                    if not sane(pb):
                        break
            RES.write_text(json.dumps(dump(pb)))
        else:
            time.sleep(0.03)


if __name__ == "__main__":
    main()
