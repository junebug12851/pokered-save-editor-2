"""Hand the real Game Boy a save with a GLITCH music id (or a bad music bank) and watch.

`analyze_music_ids.py` says, statically, that most "glitch" music ids are not garbage at all:
they are a real song's *inner voices* -- one channel of it, played alone. This is the part
that settles it: it patches the music id / music bank into an actual save, fixes the
checksum, boots the cartridge, walks onto the map, and reads the console's own APU
registers and audio RAM back out.

What it reports per probe:
  * did the game even reach the overworld (a bad BANK byte should not -- see below)
  * wAudioROMBank / wChannelSoundIDs   -- what the engine thinks it is playing
  * NR51/NR52                          -- which hardware channels are actually ON
  * how many DISTINCT frequency values the channels take over the sample window
    (a real tune moves; a dead channel does not)

⚠️ The BANK byte is the dangerous one. PlaySound switches the ROM bank to whatever the save
says, and *then* decides which engine to call -- and if the bank isn't 2 or 8 it calls
Audio3_PlaySound's ADDRESS with the wrong bank mapped in. That is not "glitch music", that
is executing arbitrary cartridge data as code, every frame. Expect a hang or a crash.

Local-only: needs assets/references/backup.gb (gitignored, never committed, never shipped)
and the venv from scripts/emu/setup.ps1. No ROM -> exit 2.

Usage:
  tmp/emu-venv/Scripts/python scripts/emu/probe_glitch_music.py            (the standard set)
  ... --probe 2:187 --probe 2:0 --probe 5:186
"""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]

SAV_MUSIC_ID = 0x2607
SAV_MUSIC_BANK = 0x2608
SAV_CHECKSUM = 0x3523
SAV_SUM_FROM, SAV_SUM_LEN = 0x2598, 0xF8B

W_CUR_MAP = 0xD35E
W_X_COORD, W_Y_COORD = 0xD362, 0xD361
W_CUR_MAP_W, W_CUR_MAP_H = 0xD369, 0xD368
W_OVERWORLD_MAP = 0xC6E8
W_TILEMAP = 0xC3A0

W_CHANNEL_SOUND_IDS = 0xC026
W_AUDIO_ROM_BANK = 0xC0EF
W_MAP_MUSIC_ID, W_MAP_MUSIC_BANK = 0xD35B, 0xD35C

NR51, NR52 = 0xFF25, 0xFF26
FREQ_LO = {1: 0xFF13, 2: 0xFF18, 3: 0xFF1D, 4: 0xFF22}

DEFAULT_PROBES = [
    (2, 186, "PalletTown -- the real track (control)"),
    (2, 187, "PalletTown CHANNEL 2 alone (an 'inner voice' id)"),
    (2, 188, "PalletTown CHANNEL 3 alone (an 'inner voice' id)"),
    (2, 213, "Lavender CHANNEL 2 alone -- 4 notes, loops forever"),
    (2, 30, "an SFX id used as map music"),
    (2, 0, "id 0 -- the $FF $FF $FF dummy header"),
    (5, 186, "*** AN INVALID BANK *** (expect the game to die)"),
]


def checksum(buf: bytearray) -> int:
    c = 0xFF
    for i in range(SAV_SUM_FROM, SAV_SUM_FROM + SAV_SUM_LEN):
        c = (c - buf[i]) & 0xFF
    return c


def patched_save(src: Path, dst: Path, bank: int, sid: int) -> None:
    buf = bytearray(src.read_bytes())
    buf[SAV_MUSIC_ID] = sid
    buf[SAV_MUSIC_BANK] = bank
    buf[SAV_CHECKSUM] = checksum(buf)  # or the game refuses the save outright
    dst.write_bytes(buf)


def probe(rom_src: Path, sav: Path, bank: int, sid: int, note: str, frames: int = 300) -> dict:
    """Boot a COPY of the ROM with this save as its battery RAM (PyBoy reads <rom>.gb.ram)."""
    from pyboy import PyBoy

    work = REPO / "tmp" / "glitch-music" / f"bank{bank}_id{sid}"
    work.mkdir(parents=True, exist_ok=True)
    rom = work / "rom.gb"
    shutil.copyfile(rom_src, rom)
    shutil.copyfile(sav, work / "rom.gb.ram")

    raw = sav.read_bytes()
    want = {"map": raw[0x260A], "x": raw[0x260E], "y": raw[0x260D]}

    pb = PyBoy(str(rom), window="null", sound_emulated=True)
    try:
        mem = pb.memory

        booted = False
        f = 0
        while f < 6000 and not booted:
            pb.button("start" if (f // 24) % 2 == 0 else "a", delay=8)
            for _ in range(24):
                pb.tick()
            f += 24
            w, h = mem[W_CUR_MAP_W], mem[W_CUR_MAP_H]
            # The save is REALLY loaded only when the game agrees with it AND has built a map.
            if (
                mem[W_CUR_MAP] == want["map"]
                and mem[W_X_COORD] == want["x"]
                and mem[W_Y_COORD] == want["y"]
                and 0 < w <= 64
                and 0 < h <= 96
            ):
                blocks = bytes(mem[W_OVERWORLD_MAP : W_OVERWORLD_MAP + (w + 6) * (h + 6)])
                screen = bytes(mem[W_TILEMAP : W_TILEMAP + 360])
                booted = len(set(blocks)) > 1 and len(set(screen)) > 1

        if not booted:
            return {"bank": bank, "id": sid, "note": note, "reached_overworld": False}

        # The APU's frequency registers are WRITE-ONLY (they read back $FF), so we watch the
        # engine's OWN copy in audio RAM instead -- wChannelFrequencyLowBytes at $C066. A tune
        # moves; a dead channel does not.
        pitches = {c: set() for c in range(4)}
        sound_ids = set()
        nr51, nr52 = set(), set()
        for _ in range(frames):
            pb.tick()
            for c in range(4):
                pitches[c].add(mem[0xC066 + c])
            sound_ids.add(tuple(mem[W_CHANNEL_SOUND_IDS : W_CHANNEL_SOUND_IDS + 8]))
            nr51.add(mem[NR51])
            nr52.add(mem[NR52])

        return {
            "bank": bank,
            "id": sid,
            "note": note,
            "reached_overworld": True,
            "wMapMusicSoundID": mem[W_MAP_MUSIC_ID],
            "wMapMusicROMBank": mem[W_MAP_MUSIC_BANK],
            "wAudioROMBank": mem[W_AUDIO_ROM_BANK],
            "wChannelSoundIDs": list(mem[W_CHANNEL_SOUND_IDS : W_CHANNEL_SOUND_IDS + 8]),
            "NR52_seen": sorted(nr52),
            "NR51_seen": sorted(nr51),
            "distinct_pitches_per_music_channel": {c + 1: len(v) for c, v in pitches.items()},
        }
    finally:
        pb.stop(save=False)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--rom", default=str(REPO / "assets" / "references" / "backup.gb"))
    ap.add_argument("--sav", default=str(REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"))
    ap.add_argument("--probe", action="append", default=[], help="bank:id")
    args = ap.parse_args()

    rom, base = Path(args.rom), Path(args.sav)
    if not rom.exists():
        print(f"no ROM at {rom} -- skipping")
        return 2
    if not base.exists():
        print(f"no save at {base}")
        return 2

    probes = (
        [(int(p.split(":")[0]), int(p.split(":")[1]), "") for p in args.probe]
        if args.probe
        else DEFAULT_PROBES
    )

    tmp = REPO / "tmp" / "glitch-music"
    tmp.mkdir(parents=True, exist_ok=True)

    for bank, sid, note in probes:
        sav = tmp / f"bank{bank}_id{sid}.sav"
        patched_save(base, sav, bank, sid)
        print(f"\n=== bank {bank}, id {sid} — {note}")
        try:
            r = probe(rom, sav, bank, sid, note)
        except Exception as e:  # a dead console is a result, not a failure
            print(f"    THE EMULATOR DIED: {type(e).__name__}: {e}")
            continue
        if not r["reached_overworld"]:
            print("    the game NEVER REACHED THE OVERWORLD (it did not survive this save)")
            continue
        print(f"    wMapMusicSoundID={r['wMapMusicSoundID']} bank={r['wMapMusicROMBank']} "
              f"| wAudioROMBank={r['wAudioROMBank']}")
        print(f"    wChannelSoundIDs={r['wChannelSoundIDs']}")
        print(f"    NR52 seen={[hex(x) for x in r['NR52_seen']]} NR51 seen={[hex(x) for x in r['NR51_seen']]}")
        print(f"    distinct pitches per music channel over 300 frames: {r['distinct_pitches_per_music_channel']}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
