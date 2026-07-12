"""Dump the console's own sound-engine state, every frame, while it plays a track.

This is the oracle for the audio work, and it is the same doctrine as the map: the editor can be
internally consistent and still be wrong; the console cannot.

Gen 1's whole sound engine lives in **243 bytes at $C000** -- every command pointer, note delay,
octave, duty, volume, vibrato counter and pitch-slide accumulator. So we boot the real cartridge with
a track patched into the save, wait until it is genuinely playing, and then photograph that block on
**every single frame**. `tst_sound_parity` seeds our C++ engine from the first photograph and demands
it reproduce all the others.

What it writes to <out>/<bank>_<id>/:
  meta.json    the track, the frame count, and the APU registers + wave RAM at the start frame
  state.bin    K x 256 bytes: $C000-$C0FF, one photograph per frame

Local-only: needs assets/references/backup.gb (gitignored, never committed, never shipped). No ROM
-> exit 2 and the test SKIPs.

Usage:
  python scripts/emu/dump_sound_state.py --bank 2 --id 186 [--frames 240]
"""

from __future__ import annotations

import argparse
import json
import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]

SAV_MUSIC_ID, SAV_MUSIC_BANK = 0x2607, 0x2608
SAV_CHECKSUM, SUM_FROM, SUM_LEN = 0x3523, 0x2598, 0xF8B

W_AUDIO = 0xC000            # the engine's entire mind: $C000-$C0FF
W_SOUND_IDS = 0xC026
W_AUDIO_ROM_BANK = 0xC0EF   # which engine/bank the console is actually running
W_CUR_MAP, W_Y, W_X = 0xD35E, 0xD361, 0xD362
W_MAP_H, W_MAP_W = 0xD368, 0xD369
W_OVERWORLD_MAP, W_TILEMAP = 0xC6E8, 0xC3A0


def patch_save(src: Path, dst: Path, bank: int, sid: int) -> None:
    b = bytearray(src.read_bytes())
    b[SAV_MUSIC_ID] = sid
    b[SAV_MUSIC_BANK] = bank
    c = 0xFF
    for i in range(SUM_FROM, SUM_FROM + SUM_LEN):
        c = (c - b[i]) & 0xFF
    b[SAV_CHECKSUM] = c   # without this the game rejects the save and you silently test a NEW GAME
    dst.write_bytes(b)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--rom", default=str(REPO / "assets" / "references" / "backup.gb"))
    ap.add_argument("--sav", default=str(REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"))
    ap.add_argument("--out", default=str(REPO / "tmp" / "sound"))
    ap.add_argument("--bank", type=int, required=True)
    ap.add_argument("--id", type=int, required=True)
    ap.add_argument("--frames", type=int, default=240)
    args = ap.parse_args()

    rom_src = Path(args.rom)
    if not rom_src.exists():
        print(f"SKIP: no ROM at {rom_src}", file=sys.stderr)
        return 2
    try:
        from pyboy import PyBoy
    except ImportError:
        print("SKIP: pyboy not installed (scripts/emu/setup.ps1)", file=sys.stderr)
        return 2

    out = Path(args.out) / f"{args.bank}_{args.id}"
    out.mkdir(parents=True, exist_ok=True)

    rom = out / "rom.gb"
    shutil.copyfile(rom_src, rom)
    patch_save(Path(args.sav), out / "rom.gb.ram", args.bank, args.id)   # PyBoy reads <rom>.gb.ram

    raw = Path(args.sav).read_bytes()
    want = {"map": raw[0x260A], "x": raw[0x260E], "y": raw[0x260D]}

    pb = PyBoy(str(rom), window="null", sound_emulated=True)
    mem = pb.memory

    # ---- boot to the overworld (and confirm it is OUR save, not a new game)
    booted, f = False, 0
    while f < 6000 and not booted:
        pb.button("start" if (f // 24) % 2 == 0 else "a", delay=8)
        for _ in range(24):
            pb.tick()
        f += 24
        w, h = mem[W_MAP_W], mem[W_MAP_H]
        if (mem[W_CUR_MAP] == want["map"] and mem[W_X] == want["x"] and mem[W_Y] == want["y"]
                and 0 < w <= 64 and 0 < h <= 96):
            blocks = bytes(mem[W_OVERWORLD_MAP: W_OVERWORLD_MAP + (w + 6) * (h + 6)])
            screen = bytes(mem[W_TILEMAP: W_TILEMAP + 360])
            booted = len(set(blocks)) > 1 and len(set(screen)) > 1

    if not booted:
        print("the game never reached the overworld with this save", file=sys.stderr)
        return 1

    # ---- wait until the track is genuinely PLAYING, in ITS OWN BANK.
    #
    # ⚠️ The trap: entering a map FADES THE OLD MUSIC OUT first, and during that fade the channel
    # sound-ids can already read as the new track while `wAudioROMBank` is still the OLD bank -- so
    # the command pointers are addresses in a DIFFERENT bank's data. Photographing that state gives
    # a dump that looks plausible and is complete nonsense. (It did, on 2026-07-12: half the tracks
    # "failed" parity because the dump was of the wrong engine.) So we insist on the bank too.
    settled = False
    for _ in range(1200):
        pb.tick()
        ids = list(mem[W_SOUND_IDS: W_SOUND_IDS + 4])
        if mem[W_AUDIO_ROM_BANK] == args.bank and any(i == args.id for i in ids):
            settled = True
            break
    if not settled:
        print(f"the console never started sound id {args.id} in bank {args.bank} "
              f"(audio bank is {mem[W_AUDIO_ROM_BANK]})", file=sys.stderr)
        return 1

    # A few more frames so the first note is under way and nothing is half-initialised.
    for _ in range(8):
        pb.tick()

    # ---- photograph the engine's mind, every frame
    frames = bytearray()
    for _ in range(args.frames):
        frames += bytes(mem[W_AUDIO: W_AUDIO + 0x100])
        pb.tick()

    (out / "state.bin").write_bytes(bytes(frames))
    (out / "meta.json").write_text(json.dumps({
        "bank": args.bank,
        "id": args.id,
        "frames": args.frames,
        # The engine READS some registers back (rAUDTERM, rAUDVOL, the NRx1 duty bits), so our APU
        # has to start where the console's was.
        "apu": [mem[a] for a in range(0xFF10, 0xFF27)],
        "wave": [mem[a] for a in range(0xFF30, 0xFF40)],
    }, indent=1), "utf-8")

    print(f"wrote {out} ({args.frames} frames)")
    pb.stop(save=False)
    return 0


if __name__ == "__main__":
    sys.exit(main())
