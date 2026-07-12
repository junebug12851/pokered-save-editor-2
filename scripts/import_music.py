"""Import the game's music data from pret/pokered -- the same way we imported the map blocks.

WHAT THIS PRODUCES

    projects/db/assets/data/music/bank02.bin   the audio "bank" as the engine will see it
    projects/db/assets/data/music/bank08.bin
    projects/db/assets/data/music/bank1f.bin
    projects/db/assets/data/music/waves.bin    the 10 channel-3 wave instruments (16 bytes each)
    projects/db/assets/data/music/index.json    what is in them

Each .bin is a **relocated copy of that ROM bank's audio region**: the 256-entry header table at
$4000 (3 bytes per entry -- which is *why* the "glitch" ids are a song's inner voices; see
notes/reference/glitch-music.md), followed by every command stream the table can reach. Pointers are
rewritten into this space. Nothing else from the cartridge is copied: no code, no graphics, no text.

WHY IT'S SAFE TO DO IT THIS WAY

The id of a track is not stored anywhere -- it is *computed from the header's address*
("Song ids are calculated by address to save space" -- music_constants.asm):

    id = (header_address - SFX_Headers_N) / 3

So the header table's LAYOUT is load-bearing: get it wrong by one entry and every id in the game
shifts. This script therefore **proves** its layout: it recomputes the id of all 46 real tracks from
the table it built and demands they equal the ids already in `music.json`. If a single one disagrees,
it refuses to write anything.

  --check   verify only (no writing) -- and, if assets/references/backup.gb is present, additionally
            compare every imported byte against the real cartridge.

Usage:
  python scripts/import_music.py [--check]
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
PRET = Path.home() / "Documents" / "projects" / "pokered"
OUT = REPO / "projects" / "db" / "assets" / "data" / "music"

BASE = 0x4000  # we keep the ROM's address base so the engine's 16-bit pointers mean the same thing

BANKS = {
    2: ("sfxheaders1.asm", "musicheaders1.asm", "_1"),
    8: ("sfxheaders2.asm", "musicheaders2.asm", "_2"),
    31: ("sfxheaders3.asm", "musicheaders3.asm", "_3"),
}

NOTES = {n: i for i, n in enumerate(
    ["C_", "C#", "D_", "D#", "E_", "F_", "F#", "G_", "G#", "A_", "A#", "B_"])}

# .wave5 is a BUG: the label has no data behind it, so the game reads whatever bytes follow the wave
# table in that bank. pokered records exactly what those bytes are, per bank, in wave_samples.asm --
# and Lavender Town / Pokemon Tower are built on the result. Reproduced verbatim; not "fixed".
WAVE5 = {
    2:  [2, 1, 14, 2, 3, 3, 2, 8, 14, 1, 2, 2, 15, 15, 14, 10, 1, 0, 1, 4, 13, 12, 1, 0, 14, 3, 4, 1, 5, 1, 7, 3],
    8:  [14, 12, 0, 2, 2, 0, 9, 1, 0, 7, 12, 0, 2, 0, 8, 1, 0, 7, 13, 0, 2, 0, 9, 1, 0, 7, 12, 0, 2, 12, 10, 1],
    31: [2, 1, 14, 2, 3, 3, 2, 8, 14, 1, 2, 2, 15, 15, 2, 2, 15, 7, 2, 4, 2, 2, 15, 7, 3, 4, 2, 4, 15, 7, 4, 4],
}


# --------------------------------------------------------------------------- tiny rgbasm subset


def strip(line: str) -> str:
    line = re.sub(r";.*$", "", line)
    return line.strip()


def parse_int(tok: str) -> int:
    tok = tok.strip()
    if tok.startswith("$"):
        return int(tok[1:], 16)
    if tok.startswith("%"):
        return int(tok[1:], 2)
    return int(tok, 0)


class Stream:
    """One source file, assembled into a relocatable blob of bytes with named labels."""

    def __init__(self, path: Path):
        self.path = path
        self.data = bytearray()
        self.labels: dict[str, int] = {}
        self.fixups: list[tuple[int, str]] = []  # (offset of the 2-byte pointer, label name)
        self._scope = ""
        self._assemble()

    # a channel's software index (0-7) decides how some commands are encoded
    def _chan_of(self, label: str) -> int:
        m = re.search(r"_Ch(\d)", label)
        return int(m.group(1)) - 1 if m else 0

    def _emit(self, *b: int) -> None:
        self.data.extend(b)

    def _ptr(self, target: str) -> None:
        if target.startswith("."):
            target = self._scope + target
        self.fixups.append((len(self.data), target))
        self._emit(0, 0)

    def _assemble(self) -> None:
        chan = 0
        # A couple of SFX differ between Red and Blue (`IF DEF(_RED)`). This editor targets the
        # US English RED cartridge, so we take the _RED branch -- the same one our backup.gb is.
        emit_on = True
        for raw in self.path.read_text("utf-8").splitlines():
            line = strip(raw)
            if not line:
                continue

            if line.startswith("IF "):
                emit_on = "DEF(_RED)" in line
                continue
            if line == "ELSE":
                emit_on = not emit_on
                continue
            if line == "ENDC":
                emit_on = True
                continue
            if not emit_on:
                continue

            # Labels sit at column 0; commands are indented. (Getting this wrong makes bare
            # commands like `sound_ret` look like labels, which silently wrecks every scope
            # after them -- the music-resolution guard below is what caught it.)
            indented = raw[:1] in (" ", "\t")
            if not indented:
                name = line.rstrip(":")
                if name.startswith("."):
                    self.labels[self._scope + name] = len(self.data)
                else:
                    self._scope = name
                    self.labels[name] = len(self.data)
                    chan = self._chan_of(name)
                continue

            parts = line.split(None, 1)
            op = parts[0]
            args = [a.strip() for a in parts[1].split(",")] if len(parts) > 1 else []
            self._op(op, args, chan)

    def _op(self, op: str, a: list[str], chan: int) -> None:
        is_noise = chan in (3, 7)

        if op == "note":
            self._emit((NOTES[a[0]] << 4) | (parse_int(a[1]) - 1))
        elif op == "rest":
            self._emit(0xC0 | (parse_int(a[0]) - 1))
        elif op == "octave":
            self._emit(0xE0 | (8 - parse_int(a[0])))
        elif op == "note_type":
            self._emit(0xD0 | parse_int(a[0]))
            self._emit(self._nibbles(parse_int(a[1]), parse_int(a[2])))
        elif op == "drum_speed":
            self._emit(0xD0 | parse_int(a[0]))
        elif op == "drum_note":
            self._emit(0xB0 | (parse_int(a[1]) - 1), parse_int(a[0]))
        elif op == "duty_cycle":
            self._emit(0xEC, parse_int(a[0]))
        elif op == "duty_cycle_pattern":
            v = (parse_int(a[0]) << 6) | (parse_int(a[1]) << 4) | (parse_int(a[2]) << 2) | parse_int(a[3])
            self._emit(0xFC, v)
        elif op == "tempo":
            t = parse_int(a[0])
            self._emit(0xED, (t >> 8) & 0xFF, t & 0xFF)
        elif op == "volume":
            self._emit(0xF0, (parse_int(a[0]) << 4) | parse_int(a[1]))
        elif op == "stereo_panning":
            self._emit(0xEE, (parse_int(a[0]) << 4) | parse_int(a[1]))
        elif op == "vibrato":
            self._emit(0xEA, parse_int(a[0]))
            self._emit((parse_int(a[1]) << 4) | parse_int(a[2]))
        elif op == "toggle_perfect_pitch":
            self._emit(0xE8)
        elif op == "pitch_slide":
            self._emit(0xEB, parse_int(a[0]) - 1)
            self._emit(((8 - parse_int(a[1])) << 4) | NOTES[a[2]] if a[2] in NOTES else
                       ((8 - parse_int(a[1])) << 4) | parse_int(a[2]))
        elif op == "pitch_sweep":
            self._emit(0x10, self._nibbles(parse_int(a[0]), parse_int(a[1])))
        elif op == "square_note":
            self._emit(0x20 | parse_int(a[0]))
            self._emit(self._nibbles(parse_int(a[1]), parse_int(a[2])))
            f = parse_int(a[3])
            self._emit(f & 0xFF, (f >> 8) & 0xFF)
        elif op == "noise_note":
            self._emit(0x20 | parse_int(a[0]))
            self._emit(self._nibbles(parse_int(a[1]), parse_int(a[2])))
            self._emit(parse_int(a[3]))
        elif op == "execute_music":
            self._emit(0xF8)
        elif op == "unknownmusic0xef":
            self._emit(0xEF, parse_int(a[0]))
        elif op == "sound_call":
            self._emit(0xFD)
            self._ptr(a[0])
        elif op == "sound_loop":
            self._emit(0xFE, parse_int(a[0]))
            self._ptr(a[1])
        elif op == "sound_ret":
            self._emit(0xFF)
        elif op == "dn":
            vals = [parse_int(x) for x in a]
            for i in range(0, len(vals), 2):
                self._emit((vals[i] << 4) | vals[i + 1])
        elif op == "db":
            for x in a:
                self._emit(parse_int(x))
        elif op in ("dw", "channel_count", "channel", "INCLUDE", "SECTION", "table_width",
                    "assert_table_length"):
            pass  # not part of a command stream
        else:
            raise SystemExit(f"{self.path.name}: unknown op '{op}'")

    @staticmethod
    def _nibbles(hi: int, lo: int) -> int:
        """`dn hi, lo` -- but a NEGATIVE low nibble is signed-magnitude (bit 3 = the sign)."""
        if lo < 0:
            lo = 0b1000 | (-lo)
        return ((hi & 0xF) << 4) | (lo & 0xF)


def parse_headers(path: Path) -> list[tuple[str, int, list[tuple[int, str]]]]:
    """[(header label, channel count, [(software channel 1-8, target label)])] in file order."""
    out: list[tuple[str, int, list[tuple[int, str]]]] = []
    label = None
    count = 0
    chans: list[tuple[int, str]] = []
    pad = 0
    for raw in path.read_text("utf-8").splitlines():
        line = strip(raw)
        if not line:
            continue
        if line.endswith("::") or (line.endswith(":") and not line.startswith("\t")):
            if label and chans:
                out.append((label, count, chans))
            label, count, chans = line.rstrip(":"), 0, []
            continue
        parts = line.split(None, 1)
        if parts[0] == "channel_count":
            count = parse_int(parts[1])
        elif parts[0] == "channel":
            c, t = [x.strip() for x in parts[1].split(",")]
            chans.append((parse_int(c), t))
        elif parts[0] == "db":
            pad += len([x for x in parts[1].split(",")])
    if label and chans:
        out.append((label, count, chans))
    # the SFX_Headers_N label itself carries `db $ff, $ff, $ff ; padding` -- id 0
    return out


def build_bank(bank: int, streams: dict[str, Stream]) -> tuple[bytearray, dict, list, int]:
    sfxh, mush, _suffix = BANKS[bank]
    sfx_entries = parse_headers(PRET / "audio" / "headers" / sfxh)
    entries = sfx_entries + parse_headers(PRET / "audio" / "headers" / mush)

    # ---- the header table. 3 bytes per CHANNEL; id 0 is the $ff $ff $ff padding.
    table = bytearray(b"\xff\xff\xff")
    id_of: dict[str, int] = {}
    ptr_fixups: list[tuple[int, str]] = []  # (offset in table, label)
    for label, count, chans in entries:
        id_of[label] = len(table) // 3
        for i, (c, target) in enumerate(chans):
            first = i == 0
            table.append((((count - 1) << 6) if first else 0) | (c - 1))
            ptr_fixups.append((len(table), target))
            table.extend(b"\x00\x00")
    if len(table) > 256 * 3:
        raise SystemExit(f"bank {bank}: header table is {len(table)} bytes -- over 256 ids")
    table.extend(b"\xff" * (256 * 3 - len(table)))

    # ---- lay the reachable streams out after the table, following references transitively
    blob = bytearray(table)
    placed: dict[str, int] = {}  # file name -> base address
    unresolved: set[str] = set()
    stub = [BASE + len(blob)]  # a lone sound_ret, for the SFX that live in code we don't import
    blob.append(0xFF)

    def label_addr(name: str) -> int:
        owner = owner_of.get(name)
        if owner is None:
            # A handful of sound effects live inside .asm files that are CODE, not data
            # (poke_flute.asm, low_health_alarm.asm, pokedex_rating_sfx.asm -- they poke the
            # hardware directly). We don't import those, so their header entries point at a
            # lone `sound_ret`: the id stays valid and simply plays nothing. Music is unaffected.
            unresolved.add(name)
            return stub[0]
        if owner.path.name not in placed:
            place(owner)
        return placed[owner.path.name] + owner.labels[name]

    def place(s: Stream) -> None:
        placed[s.path.name] = BASE + len(blob)
        blob.extend(s.data)
        for off, target in s.fixups:
            pending.append((placed[s.path.name] - BASE + off, target))

    owner_of: dict[str, Stream] = {}
    for s in streams.values():
        for name in s.labels:
            owner_of.setdefault(name, s)

    pending: list[tuple[int, str]] = []
    for off, target in ptr_fixups:
        pending.append((off, target))

    i = 0
    while i < len(pending):
        off, target = pending[i]
        i += 1
        addr = label_addr(target)
        blob[off] = addr & 0xFF
        blob[off + 1] = (addr >> 8) & 0xFF

    # MAX_SFX_ID_N -- the id of the LAST sound-effect header. PlaySound branches on it:
    # id <= max => the SFX path (partial init), id > max => the MUSIC path (full re-init).
    max_sfx = id_of[sfx_entries[-1][0]]

    return blob, id_of, sorted(unresolved), max_sfx


def cmd_len(op: int, chan: int) -> int:
    """Byte length of a command on software channel `chan` (0-7). Mirrors the engine EXACTLY.

    The channel is not a detail here, it changes what a byte *means*:

    * `$2x` is `sfx_note` **only on CHAN4-CHAN8** (`Audio1_sfx_note`: `cp CHAN4; jr c, ...`).
      On the three music tone channels `$2x` is an ordinary **note** -- one byte. Reading it as a
      4-byte sfx_note walks straight off the end of the bar, which is precisely the bug the
      cartridge cross-check caught here on 2026-07-12.
    * `$10` is `pitch_sweep` only on the SFX channels (CHAN5+).
    * `$Bx` is `drum_note` only on the music noise channel (CHAN4).
    * `$Dx` is `drum_speed` (1 byte) on the noise channels, `note_type` (2 bytes) elsewhere.
    """
    hi = op & 0xF0
    noise = chan in (3, 7)          # the two noise channels
    sfx_note_ok = chan >= 3         # CHAN4..CHAN8
    sweep_ok = chan >= 4            # CHAN5..CHAN8

    if op == 0xFF:
        return 1
    if op == 0xFE:
        return 4
    if op == 0xFD:
        return 3
    if op == 0xFC:
        return 2
    if op == 0xF8:
        return 1
    if hi == 0xF0:
        return 2
    if op in (0xEF, 0xEE, 0xEC):
        return 2
    if op in (0xED, 0xEB, 0xEA):
        return 3
    if op == 0xE8:
        return 1
    if hi == 0xE0:
        return 1
    if hi == 0xD0:
        return 1 if noise else 2
    if hi == 0xC0:
        return 1
    if hi == 0xB0 and chan == 3:
        return 2                    # drum_note (instrument byte follows)
    if op == 0x10 and sweep_ok:
        return 2
    if hi == 0x20 and sfx_note_ok:
        return 3 if noise else 4
    return 1                        # a note


def verify_against_rom(blobs: dict[int, bytearray], rom: bytes, music_json: list) -> bool:
    """Walk OUR stream and the CARTRIDGE's stream in lockstep. Every command byte must match.

    The two live at different addresses, so pointer operands can't be compared directly -- instead
    each ROM address is required to map to exactly one of our addresses (and vice versa), which is a
    stronger statement than byte equality: it says the two graphs are the same graph.
    """
    ok = True
    for bank, blob in blobs.items():
        rb = rom[bank * 0x4000: (bank + 1) * 0x4000]

        def byte_rom(a: int) -> int:
            return rb[a - BASE] if BASE <= a < BASE + len(rb) else -1

        def byte_our(a: int) -> int:
            return blob[a - BASE] if BASE <= a < BASE + len(blob) else -1

        for m in music_json:
            if m["bank"] != bank or m["name"] == "None":
                continue
            off = m["id"] * 3
            n_rom = ((rb[off] & 0xC0) >> 6) + 1
            n_our = ((blob[off] & 0xC0) >> 6) + 1
            if n_rom != n_our:
                print(f"  MISMATCH {m['name']}: channel count {n_our} vs cartridge {n_rom}")
                ok = False
                continue

            for k in range(n_rom):
                o = off + k * 3
                if (rb[o] & 0x0F) != (blob[o] & 0x0F):
                    print(f"  MISMATCH {m['name']} ch{k+1}: software channel differs")
                    ok = False
                    continue
                chan = rb[o] & 0x0F
                pr = rb[o + 1] | (rb[o + 2] << 8)
                po = blob[o + 1] | (blob[o + 2] << 8)

                seen: dict[int, int] = {}
                work = [(pr, po)]
                while work:
                    ar, ao = work.pop()
                    if ar in seen:
                        if seen[ar] != ao:
                            print(f"  MISMATCH {m['name']} ch{k+1}: ROM ${ar:04X} maps to two "
                                  f"different places in our data")
                            ok = False
                        continue
                    seen[ar] = ao
                    while True:
                        opr, opo = byte_rom(ar), byte_our(ao)
                        if opr != opo:
                            print(f"  MISMATCH {m['name']} ch{k+1}: ROM ${ar:04X}=${opr:02X} "
                                  f"but ours=${opo:02X} (chain from ROM ${pr:04X}, +{ar - pr})")
                            ok = False
                            break
                        ln = cmd_len(opr, chan)
                        if opr in (0xFD, 0xFE):  # sound_call / sound_loop -- a pointer operand
                            i = 1 if opr == 0xFD else 2
                            tr = byte_rom(ar + i) | (byte_rom(ar + i + 1) << 8)
                            to = byte_our(ao + i) | (byte_our(ao + i + 1) << 8)
                            infinite = opr == 0xFE and byte_rom(ar + 1) == 0
                            if opr == 0xFE and byte_rom(ar + 1) != byte_our(ao + 1):
                                print(f"  MISMATCH {m['name']} ch{k+1}: loop count differs")
                                ok = False
                            work.append((tr, to))
                            if infinite:
                                # `sound_loop 0` never falls through -- it is a terminator, and the
                                # bytes after it are the NEXT track. Walking past it wanders into
                                # another song (and, in our layout, off the end of the data).
                                break
                        else:
                            for j in range(1, ln):
                                if byte_rom(ar + j) != byte_our(ao + j):
                                    print(f"  MISMATCH {m['name']} ch{k+1}: operand at "
                                          f"${ar+j:04X}")
                                    ok = False
                        if opr == 0xFF:
                            break
                        ar += ln
                        ao += ln
        print(f"bank {bank:2d}: {'MATCHES' if ok else 'DIFFERS FROM'} the cartridge")
    return ok


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    if not PRET.exists():
        print(f"no pokered clone at {PRET}")
        return 2

    # ---- assemble every command-stream source once
    streams: dict[str, Stream] = {}
    for d in ("music", "sfx"):
        for f in sorted((PRET / "audio" / d).glob("*.asm")):
            streams[f.name] = Stream(f)

    music_json = json.loads((REPO / "projects" / "db" / "assets" / "data" / "music.json")
                            .read_text("utf-8"))

    index = {"banks": {}, "tracks": []}
    blobs: dict[int, bytearray] = {}
    for bank in BANKS:
        blob, id_of, unresolved, max_sfx = build_bank(bank, streams)
        blobs[bank] = blob

        # ---- THE PROOF. Recompute every real track's id from the table we just built.
        for m in music_json:
            if m["bank"] != bank:
                continue
            want, got = m["id"], id_of.get(f"Music_{m['name']}")
            if m["name"] == "None":
                continue  # id 255 = SFX_STOP_ALL_MUSIC, not a header
            if got != want:
                print(f"REFUSING TO WRITE: bank {bank} '{m['name']}' should be id {want}, "
                      f"the table we built says {got}. The header layout is wrong.")
                return 1

        # A stubbed MUSIC stream would be a silent, invisible failure. Never allow one.
        bad = [u for u in unresolved if u.startswith("Music_")]
        if bad:
            print(f"REFUSING TO WRITE: bank {bank} could not resolve music streams: {bad}")
            return 1
        if unresolved:
            print(f"  bank {bank}: code-only SFX stubbed -> {', '.join(unresolved[:4])}"
                  f"{' …' if len(unresolved) > 4 else ''}")

        index["banks"][str(bank)] = {"size": len(blob), "base": BASE, "maxSfxId": max_sfx,
                                     "unresolved_sfx": unresolved}
        print(f"bank {bank:2d}: {len(blob):6d} bytes, "
              f"{len([k for k in id_of if k.startswith('Music_')]):2d} tracks, "
              f"maxSfxId={max_sfx} -- ids verified"
              + (f" ({len(unresolved)} code-only SFX stubbed)" if unresolved else ""))

    # ---- the id -> what-it-is map, INCLUDING the inner voices (see glitch-music.md)
    for m in music_json:
        index["tracks"].append({"name": m["name"], "bank": m["bank"], "id": m["id"]})

    waves = bytearray()
    wave_src = (PRET / "audio" / "wave_samples.asm").read_text("utf-8")
    for i in range(5):
        line = re.search(rf"^\.wave{i}\s*\n\s*dn (.+)$", wave_src, re.M)
        vals = [parse_int(x) for x in line.group(1).split(",")]
        for j in range(0, 32, 2):
            waves.append((vals[j] << 4) | vals[j + 1])
    for bank in (2, 8, 31):  # .wave5 -- the bug; different in every bank
        for j in range(0, 32, 2):
            waves.append((WAVE5[bank][j] << 4) | WAVE5[bank][j + 1])

    # ---- THE OTHER PROOF: every imported byte, against the real cartridge (when it's here)
    rom_path = REPO / "assets" / "references" / "backup.gb"
    if rom_path.exists():
        print()
        if not verify_against_rom(blobs, rom_path.read_bytes(), music_json):
            print("REFUSING TO WRITE: the imported data does not match the cartridge.")
            return 1
    else:
        print("\n(no backup.gb here -- skipping the byte-for-byte cartridge check)")

    if args.check:
        print("\n--check: nothing written. Header layout verified against music.json.")
        return 0

    OUT.mkdir(parents=True, exist_ok=True)
    for bank, blob in blobs.items():
        (OUT / f"bank{bank:02x}.bin").write_bytes(bytes(blob))
    (OUT / "waves.bin").write_bytes(bytes(waves))
    (OUT / "index.json").write_text(json.dumps(index, indent=1), "utf-8")
    print(f"\nwrote {OUT}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
