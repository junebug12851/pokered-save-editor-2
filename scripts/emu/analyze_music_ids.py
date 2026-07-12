"""What actually happens when the save holds a GLITCH music id?

The save stores a music **id** (0x2607) and a music **bank** (0x2608). Nothing in the game
validates either. So: what does the console do with an id that isn't a real track?

This walks the real cartridge and answers it for every one of the 256 ids, in each of the
three audio banks -- exactly the way `AudioN_PlaySound` does:

    header address = SFX_Headers_N + id * 3          ("Song ids are calculated by address
                                                       to save space." -- music_constants.asm)
    byte 0 : %CCcc_ssss   CC = channel count - 1, ssss = software channel (0-7)
    byte 1-2 : pointer to that channel's command stream

...and then it *disassembles* each channel's command stream (the $10-$FF command set) and
follows sound_call / sound_loop until the track ends, loops forever, or wanders somewhere it
should not be.

The punchline it is looking for: because a real header is **3 bytes per channel**, a 3-channel
song eats **three consecutive ids**. Only the first is the song. The others are a header
whose channel-count bits read 0 -- a perfectly valid ONE-channel header pointing at that
song's channel 2 or channel 3. They are not garbage. They are the song's inner voices,
playable on their own.

Local-only, like every ROM tool here: needs assets/references/backup.gb (gitignored, never
committed, never shipped). No ROM -> exit 2.

Usage:
  python scripts/emu/analyze_music_ids.py [--rom <path>] [--out <json>] [--id N --bank B]
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]

AUDIO_BANKS = {2: "Audio1", 8: "Audio2", 31: "Audio3"}

# Every bank maps its headers at the same address; the id arithmetic in music_constants.asm
# is relative to SFX_Headers_1, and all three tables sit at the top of their bank.
HEADER_BASE = 0x4000

BANK_LO, BANK_HI = 0x4000, 0x8000  # the switchable ROM window

# ---------------------------------------------------------------------------- commands


def cmd_len(op: int, chan: int) -> tuple[int, str]:
    """(total byte length, mnemonic) for a command byte on software channel `chan` (0-7)."""
    hi = op & 0xF0
    is_noise = chan in (3, 7)  # music noise / sfx noise

    if op == 0xFF:
        return 1, "sound_ret"
    if op == 0xFE:
        return 4, "sound_loop"
    if op == 0xFD:
        return 3, "sound_call"
    if op == 0xFC:
        return 2, "duty_cycle_pattern"
    if op == 0xF8:
        return 1, "execute_music"
    if hi == 0xF0:
        return 2, "volume"
    if op == 0xEF:
        return 2, "unknownmusic0xef"
    if op == 0xEE:
        return 2, "stereo_panning"
    if op == 0xED:
        return 3, "tempo"
    if op == 0xEC:
        return 2, "duty_cycle"
    if op == 0xEB:
        return 3, "pitch_slide"
    if op == 0xEA:
        return 3, "vibrato"
    if op == 0xE8:
        return 1, "toggle_perfect_pitch"
    if hi == 0xE0:
        return 1, "octave"
    if hi == 0xD0:
        # note_type: 2 bytes, except the noise channel where it is drum_speed (1 byte)
        return (1, "drum_speed") if is_noise else (2, "note_type")
    if hi == 0xC0:
        return 1, "rest"
    if hi == 0xB0 and is_noise:
        return 2, "drum_note"
    if op == 0x10:
        return 2, "pitch_sweep"
    if hi == 0x20:
        # sfx_note: length+volume/fade+frequency (noise has one less byte)
        return (3, "noise_note") if is_noise else (4, "square_note")
    return 1, "note"


def disassemble(bank: bytes, ptr: int, chan: int, budget: int = 4000):
    """Follow one channel's command stream. Returns (events, verdict, notes_played)."""
    events: list[str] = []
    notes = 0
    call_stack: list[int] = []
    loops: dict[int, int] = {}
    seen: set[int] = set()
    steps = 0

    while True:
        steps += 1
        if steps > budget:
            return events, "runs-forever (budget)", notes
        if not (BANK_LO <= ptr < BANK_HI):
            return events, f"ESCAPES the bank window (${ptr:04X})", notes

        op = bank[ptr - BANK_LO]
        ln, name = cmd_len(op, chan)

        if name == "note":
            notes += 1
        if name in ("drum_note", "square_note", "noise_note"):
            notes += 1

        if name == "sound_ret":
            if call_stack:
                ptr = call_stack.pop()
                events.append("ret->call")
                continue
            events.append("sound_ret (END)")
            return events, "ends", notes

        if name == "sound_call":
            target = bank[ptr - BANK_LO + 1] | (bank[ptr - BANK_LO + 2] << 8)
            call_stack.append(ptr + 3)
            events.append(f"call ${target:04X}")
            ptr = target
            continue

        if name == "sound_loop":
            count = bank[ptr - BANK_LO + 1]
            target = bank[ptr - BANK_LO + 2] | (bank[ptr - BANK_LO + 3] << 8)
            if count == 0:
                events.append(f"loop forever -> ${target:04X}")
                return events, "loops forever (a real, looping track)", notes
            done = loops.get(ptr, 0) + 1
            loops[ptr] = done
            if done >= count:
                ptr += 4
            else:
                ptr = target
            continue

        if ptr in seen and name in ("rest",):
            pass
        seen.add(ptr)
        events.append(name)
        ptr += ln


def parse_header(bank: bytes, sid: int):
    off = HEADER_BASE + sid * 3 - BANK_LO
    if off + 3 > len(bank):
        return None
    b0 = bank[off]
    count = ((b0 & 0xC0) >> 6) + 1
    chans = []
    for k in range(count):
        o = off + k * 3
        if o + 3 > len(bank):
            return None
        spec = bank[o]
        ptr = bank[o + 1] | (bank[o + 2] << 8)
        chans.append({"software_channel": spec & 0x0F, "ptr": ptr})
    return {"count": count, "channels": chans}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--rom", default=str(REPO / "assets" / "references" / "backup.gb"))
    ap.add_argument("--out", default=str(REPO / "tmp" / "music_ids.json"))
    ap.add_argument("--id", type=int, default=None)
    ap.add_argument("--bank", type=int, default=None)
    args = ap.parse_args()

    rom_path = Path(args.rom)
    if not rom_path.exists():
        print(f"no ROM at {rom_path} -- skipping (this is expected off Twilight's machine)")
        return 2

    rom = rom_path.read_bytes()

    music_json = json.loads(
        (REPO / "projects" / "db" / "assets" / "data" / "music.json").read_text("utf-8")
    )
    known = {(m["bank"], m["id"]): m["name"] for m in music_json}

    # Every pointer that a REAL track's header hands out -- so we can recognise a glitch id
    # that lands on one of a song's inner voices.
    real_ptrs: dict[tuple[int, int], tuple[str, int]] = {}
    for (bnk, sid), name in known.items():
        data = rom[bnk * 0x4000 : (bnk + 1) * 0x4000]
        h = parse_header(data, sid)
        if h:
            for i, c in enumerate(h["channels"]):
                real_ptrs[(bnk, c["ptr"])] = (name, i + 1)

    banks = [args.bank] if args.bank else sorted(AUDIO_BANKS)
    ids = [args.id] if args.id is not None else range(256)

    out: dict[str, list] = {}
    for bnk in banks:
        data = rom[bnk * 0x4000 : (bnk + 1) * 0x4000]
        rows = []
        for sid in ids:
            h = parse_header(data, sid)
            if not h:
                continue
            row = {
                "bank": bnk,
                "id": sid,
                "known_track": known.get((bnk, sid)),
                "channel_count": h["count"],
                "channels": [],
            }
            for c in h["channels"]:
                ev, verdict, notes = (
                    disassemble(data, c["ptr"], c["software_channel"])
                    if BANK_LO <= c["ptr"] < BANK_HI
                    else ([], f"pointer OUTSIDE the bank window (${c['ptr']:04X})", 0)
                )
                origin = real_ptrs.get((bnk, c["ptr"]))
                row["channels"].append(
                    {
                        "software_channel": c["software_channel"],
                        "ptr": f"${c['ptr']:04X}",
                        "is_a_real_track_channel": (
                            f"{origin[0]} ch{origin[1]}" if origin else None
                        ),
                        "notes": notes,
                        "verdict": verdict,
                        "first_commands": ev[:12],
                    }
                )
            rows.append(row)
        out[str(bnk)] = rows

    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.out).write_text(json.dumps(out, indent=1), "utf-8")

    # ------------------------------------------------------------------ summary
    for bnk in banks:
        rows = out[str(bnk)]
        real = [r for r in rows if r["known_track"]]
        inner = [
            r
            for r in rows
            if not r["known_track"]
            and all(c["is_a_real_track_channel"] for c in r["channels"])
            and r["channels"]
        ]
        escapes = [
            r
            for r in rows
            if any("OUTSIDE" in c["verdict"] or "ESCAPES" in c["verdict"] for c in r["channels"])
        ]
        silent = [
            r
            for r in rows
            if not r["known_track"]
            and r["channels"]
            and all(c["notes"] == 0 for c in r["channels"])
        ]
        print(
            f"bank {bnk:2d} ({AUDIO_BANKS.get(bnk,'?')}): "
            f"{len(rows)} ids | {len(real):3d} real tracks | "
            f"{len(inner):3d} inner-voice ids (a real song's channel, solo) | "
            f"{len(silent):3d} silent | {len(escapes):3d} leave the bank window"
        )

    print(f"\nwrote {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
