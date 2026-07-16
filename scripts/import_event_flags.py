#!/usr/bin/env python3
"""Parse pret/pokered's constants/event_constants.asm into the canonical
2,560-bit map of wEventFlags.

For every one of the NUM_EVENTS (=$A00 = 2560) bits this emits a row:
  index      the flag's bit index 0..2559 (== pret const_value)
  byte_wram  WRAM address of the containing byte (wEventFlags = 0xD747)
  byte_file  save-file offset of that byte (WRAM - 0xAD54; wEventFlags = 0x29F3)
  bit        bit within the byte (index % 8; flag_array is little-bit-first)
  name       the EVENT_* name pret gives it, or "" for an unnamed gap
  section    the "; <City> events" comment block it falls under

Self-validating: the walk MUST cover exactly 2560 indices and every named
event MUST land on the index pret assigns it. Anything else is a parse bug
and the script exits non-zero.

Outputs (research intermediates, git-ignored under tmp/):
  tmp/event-flags/event_flags_canonical.csv
  tmp/event-flags/event_flags_canonical.json

This is Phase 1 of the Events feature (see notes/plans/event-flags.md). It does
NOT write any curated metadata (name/description/map/group/class) — that is
Phases 2-7. It only establishes the ground truth skeleton every dossier hangs on.
"""
from __future__ import annotations
import csv
import json
import os
import re
import sys

NUM_EVENTS = 0xA00          # 2560, asserted against the file's final const_value
WEVENTFLAGS_WRAM = 0xD747   # verified: wGrassRate(0xD887) - 320
WRAM_TO_FILE = 0xAD54       # v2 mapping: file_offset = wram - 0xAD54

HOME = os.path.expanduser("~")
DEFAULT_ASM = os.path.join(HOME, "Documents", "projects", "pokered",
                           "constants", "event_constants.asm")
REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(REPO, "tmp", "event-flags")


def parse_int(tok: str) -> int:
    tok = tok.strip()
    if tok.startswith("$"):
        return int(tok[1:], 16)
    if tok.startswith("0x"):
        return int(tok, 16)
    return int(tok)


def parse(asm_path: str):
    rows = {}          # index -> {name}
    named = []         # (index, name) in file order, for validation
    boundaries = []    # (start_index, section) — where each map-region begins
    section = ""       # current "; <...> events" comment block
    pending = False    # a section comment is awaiting its start index
    value = None       # const_value; None until const_def seen

    def open_region(start):
        nonlocal pending
        # a region opens at the first const_next (or first const) after its comment
        if pending:
            boundaries.append((start, section))
            pending = False

    with open(asm_path, "r", encoding="utf-8") as fh:
        for raw in fh:
            line = raw.rstrip("\n")
            # capture a section comment (whole-line comment)
            m = re.match(r"\s*;\s*(.+?)\s*$", line)
            if m:
                section = m.group(1)
                pending = True
                continue
            # strip trailing inline comment
            code = line.split(";", 1)[0].strip()
            if not code:
                continue
            parts = code.split()
            op = parts[0]
            if op == "const_def":
                value = parse_int(parts[1]) if len(parts) > 1 else 0
            elif op == "const_next":
                value = parse_int(parts[1])
                open_region(value)
            elif op == "const_skip":
                n = parse_int(parts[1]) if len(parts) > 1 else 1
                if value is None:
                    sys.exit("const_skip before const_def")
                value += n
            elif op == "const":
                if value is None:
                    sys.exit("const before const_def")
                open_region(value)
                name = parts[1]
                if value in rows:
                    sys.exit(f"duplicate index {value} for {name}")
                rows[value] = {"name": name}
                named.append((value, name))
                value += 1
            # everything else (DEF NUM_EVENTS EQU ..., blank) ignored
    boundaries.sort()
    return rows, named, boundaries, value


def region_for(index: int, boundaries):
    """Map an index to the section of the last boundary at or before it."""
    sect = ""
    for start, s in boundaries:
        if start <= index:
            sect = s
        else:
            break
    return sect


def main() -> int:
    asm = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_ASM
    if not os.path.exists(asm):
        sys.exit(f"event_constants.asm not found: {asm}")
    rows, named, boundaries, final_value = parse(asm)

    # --- self-validation -------------------------------------------------
    problems = []
    if final_value != NUM_EVENTS:
        problems.append(f"final const_value {final_value:#x} != NUM_EVENTS {NUM_EVENTS:#x}")
    for idx, _ in named:
        if idx >= NUM_EVENTS:
            problems.append(f"named event index {idx:#x} >= NUM_EVENTS")
    if problems:
        for p in problems:
            print("VALIDATION FAIL:", p, file=sys.stderr)
        return 1

    # --- build the full 2560-row table -----------------------------------
    table = []
    for idx in range(NUM_EVENTS):
        info = rows.get(idx, {"name": ""})
        byte_wram = WEVENTFLAGS_WRAM + idx // 8
        table.append({
            "index": idx,
            "index_hex": f"{idx:#05x}",
            "byte_wram": f"{byte_wram:#06x}",
            "byte_file": f"{byte_wram - WRAM_TO_FILE:#06x}",
            "bit": idx % 8,
            "name": info["name"],
            "section": region_for(idx, boundaries),
        })

    named_count = sum(1 for r in table if r["name"])
    gap_count = NUM_EVENTS - named_count

    os.makedirs(OUT_DIR, exist_ok=True)
    with open(os.path.join(OUT_DIR, "event_flags_canonical.json"), "w", encoding="utf-8") as fh:
        json.dump(table, fh, indent=1)
    with open(os.path.join(OUT_DIR, "event_flags_canonical.csv"), "w", encoding="utf-8", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(table[0].keys()))
        w.writeheader()
        w.writerows(table)

    # section coverage summary
    sections = {}
    for r in table:
        s = r["section"] or "(no section)"
        d = sections.setdefault(s, {"named": 0, "gap": 0})
        d["named" if r["name"] else "gap"] += 1

    print(f"OK  total={NUM_EVENTS}  named={named_count}  gap={gap_count}")
    print(f"    wEventFlags WRAM {WEVENTFLAGS_WRAM:#06x}..{WEVENTFLAGS_WRAM+319:#06x}"
          f"  file {WEVENTFLAGS_WRAM-WRAM_TO_FILE:#06x}..{WEVENTFLAGS_WRAM+319-WRAM_TO_FILE:#06x}")
    print(f"    sections={len(sections)}")
    print(f"    out -> {OUT_DIR}")
    print("\n    section                                          named  gap")
    for s, d in sections.items():
        print(f"    {s[:46]:46}  {d['named']:5}  {d['gap']:5}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
