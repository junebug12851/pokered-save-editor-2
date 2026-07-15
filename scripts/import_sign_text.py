#!/usr/bin/env python3
"""
import_sign_text.py -- the map's TEXT, imported from pret/pokered.

A sign in Gen 1 does not store words -- it stores a **text id**, a 1-based index into the
map's `def_text_pointers` table. maps.json already ships every sign's id; what it has never
had is the actual English behind that id, so a person editing a sign has only a number to go
on. This imports the words.

None of it is invented. Per the standing file-format rule, this EXTENDS the map data we
already ship (it adds one field, `textEntries`, to each map in maps.json) rather than inventing
a parallel file, and it NEVER touches an existing field -- run it and diff, every other byte of
maps.json is identical.

Source:  <pokered>/data/maps/objects/<Map>.asm   (which text ids are signs vs people)
         <pokered>/scripts/<Map>.asm             (def_text_pointers: id -> label)
         <pokered>/text/**/*.asm                  (label -> the actual string)
Output:  projects/db/assets/data/maps.json  (adds "textEntries": [ {id, string, category}, ... ])

Each map's `textEntries` is an ordered list, index 0 == text id 1. Every entry is
`{ "id": N, "string": "...", "category": "sign" | "person" | "other" }`. A `text_asm` entry
that is a script with no single literal gets `"string": null` (shown in-app as "(scripted text)").

`category` is decided by what references the text id on that map: a `bg_event` -> "sign",
an `object_event` -> "person", neither -> "other". The picker groups on it.

Self-checking, because a silently-wrong parse would put the wrong words on a placard:
  1. Structural: every map with signs must produce a textEntries table, and every one of that
     map's `signs[].text` ids must land inside it.
  2. Coverage: report how many text ids resolved to real strings vs stayed scripted/blank.

Usage:
    python scripts/import_sign_text.py [--pokered <path>] [--check]
    --check   parse + verify + diff against the committed maps.json, write nothing. Exit 1 on drift.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

for _s in (sys.stdout, sys.stderr):
    try:
        _s.reconfigure(encoding="utf-8")
    except AttributeError:
        pass

REPO = Path(__file__).resolve().parent.parent
DEFAULT_POKERED = Path.home() / "Documents" / "projects" / "pokered"
MAPS_JSON = REPO / "projects" / "db" / "assets" / "data" / "maps.json"


# ── Text control codes -> readable English ──────────────────────────────────────────────────
# We assemble the human string the game prints. The game's text macros are turned into plain
# text; the few substitution tokens are kept as readable placeholders (a save editor showing
# "<PLAYER>" is more honest than guessing a name). `#` is the charmap's "POKé".
SUBS = [
    ("#MON", "POKéMON"),      # '#' == "POKé"; the very common "#MON" reads POKéMON
    ("#", "POKé"),
    ("<PLAYER>", "<PLAYER>"),
    ("<RIVAL>", "<RIVAL>"),
    ("<USER>", "<USER>"),
    ("<TARGET>", "<TARGET>"),
    ("<PC>", "<PC>"),
    ("<TM>", "TM"),
    ("<PKMN>", "POKéMON"),
    ("<POKE>", "POKé"),
    ("¥", "¥"),
    ("<DEXEND>", ""),
    ("<DOT>", "."),
    ("<LNBRK>", " "),
]

# The text macros that carry a quoted literal, and how they join to the previous line.
#   text/line/cont/next -> newline (cont/next continue a box; line a new line)
#   para                 -> blank line (new text box)
LINE_MACROS = {"text", "line", "cont", "next", "para", "page", "prompt", "done", "text_end"}


def clean_literal(s: str) -> str:
    for a, b in SUBS:
        s = s.replace(a, b)
    return s


def read(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (FileNotFoundError, UnicodeDecodeError):
        try:
            return path.read_text(encoding="latin-1")
        except FileNotFoundError:
            return ""


# ── 1. ind -> Map file base name, from data/maps/map_header_pointers.asm ─────────────────────
def map_names(pokered: Path) -> list[str]:
    txt = read(pokered / "data" / "maps" / "map_header_pointers.asm")
    # Every `dw <Map>_h` is one map id, in order -- INCLUDING the unused slots, which point at
    # a real map with a comment (`dw SaffronCity_h ; UNUSED_MAP_0B`). Match `_h` on a word
    # boundary (not end-of-line) or those commented slots drop out and shift every id after them.
    names = []
    for m in re.finditer(r"^\s*dw\s+(\w+)_h\b", txt, re.M):
        names.append(m.group(1))
    return names


# ── 2. A global index: label -> assembled string, from every text/**/*.asm ───────────────────
# Labels can chain via `text_far` (a label whose only body is `text_far _Other`), so we first
# capture every label's raw body, then resolve.
LABEL_RE = re.compile(r"^(\w+)::?\s*$", re.M)
QUOTED_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')


def index_text_labels(pokered: Path) -> dict[str, str | None]:
    """Return {label: assembled string, or None if it's a script with no literal}."""
    bodies: dict[str, list[str]] = {}
    files = list((pokered / "text").rglob("*.asm"))
    # scripts/*.asm also hold the wrapper labels (Label: text_far _X / text_asm), so include them.
    files += list((pokered / "scripts").rglob("*.asm"))
    # Shared signs live outside the per-map tree: the wrappers (MartSignText, PokeCenterSignText)
    # are in home/overworld_text.asm, and their strings (_MartSignText, ...) in data/text/*.asm.
    files += list((pokered / "home").glob("*.asm"))
    files += list((pokered / "data" / "text").rglob("*.asm"))

    for f in files:
        txt = read(f)
        lines = txt.splitlines()
        cur: str | None = None
        for raw in lines:
            line = raw.strip()
            mlbl = re.match(r"^(\w+)::?\s*$", line)
            if mlbl:
                cur = mlbl.group(1)
                bodies.setdefault(cur, [])
                continue
            if cur is None:
                continue
            bodies[cur].append(line)

    resolved: dict[str, str | None] = {}

    def assemble(label: str, seen: set[str]) -> str | None:
        if label in resolved:
            return resolved[label]
        if label in seen:  # cycle guard
            return None
        seen.add(label)
        body = bodies.get(label)
        if body is None:
            return None
        out: list[str] = []
        is_script = False
        got_literal = False
        for line in body:
            # stop at the next label (already split) -- bodies are per-label already.
            if line.startswith("text_asm"):
                is_script = True
                continue
            mfar = re.match(r"^text_far\s+(\w+)", line)
            if mfar:
                sub = assemble(mfar.group(1), seen)
                if sub:
                    out.append(sub)
                    got_literal = True
                continue
            mmac = re.match(r"^(\w+)\b(.*)$", line)
            if not mmac:
                continue
            macro, rest = mmac.group(1), mmac.group(2)
            if macro in ("done", "prompt", "text_end", "text_promptbutton", "text_waitbutton"):
                # end of this text
                break
            if macro in ("text", "line", "cont", "next", "para", "page"):
                q = QUOTED_RE.search(rest)
                if q:
                    seg = clean_literal(q.group(1))
                    if macro == "para" or macro == "page":
                        out.append("")  # blank line -> new box
                    out.append(seg)
                    got_literal = True
                continue
            # ignore db/other assembler noise
        if got_literal:
            s = "\n".join(out).strip("\n")
            resolved[label] = s
            return s
        if is_script:
            resolved[label] = None
            return None
        resolved[label] = None
        return None

    for label in list(bodies.keys()):
        assemble(label, set())
    return resolved


# ── 3. Per-map: the def_text_pointers table (id -> label) ────────────────────────────────────
def text_pointer_table(pokered: Path, name: str) -> list[str]:
    """Ordered list of the labels in <Map>'s def_text_pointers (index 0 == text id 1)."""
    txt = read(pokered / "scripts" / f"{name}.asm")
    if not txt:
        return []
    m = re.search(r"def_text_pointers\b(.*?)(?:^\S|\Z)", txt, re.S | re.M)
    if not m:
        return []
    block = m.group(1)
    labels: list[str] = []
    for line in block.splitlines():
        line = line.strip()
        md = re.match(r"^dw_const\s+(\w+)\s*,", line)
        if md:
            labels.append(md.group(1))
            continue
        md = re.match(r"^dw\s+(\w+)\s*$", line)
        if md:
            labels.append(md.group(1))
    return labels


# ── 4. Per-map: which TEXT_* consts are signs (bg_event) vs people (object_event) ────────────
def categorize(pokered: Path, name: str) -> tuple[set[str], set[str]]:
    txt = read(pokered / "data" / "maps" / "objects" / f"{name}.asm")
    signs, people = set(), set()
    for m in re.finditer(r"^\s*bg_event\s+[^,]+,[^,]+,\s*(\w+)", txt, re.M):
        signs.add(m.group(1))
    for m in re.finditer(r"^\s*object_event\s+.*,\s*(\w+)\s*$", txt, re.M):
        people.add(m.group(1))
    return signs, people


# def_text_pointers entries also carry the CONST as the 2nd arg of dw_const; capture it so we
# can categorize by the const (robust even if label naming is irregular).
def text_pointer_consts(pokered: Path, name: str) -> list[str | None]:
    txt = read(pokered / "scripts" / f"{name}.asm")
    if not txt:
        return []
    m = re.search(r"def_text_pointers\b(.*?)(?:^\S|\Z)", txt, re.S | re.M)
    if not m:
        return []
    consts: list[str | None] = []
    for line in m.group(1).splitlines():
        line = line.strip()
        md = re.match(r"^dw_const\s+\w+\s*,\s*(\w+)", line)
        if md:
            consts.append(md.group(1))
        elif re.match(r"^dw\s+\w+\s*$", line):
            consts.append(None)
    return consts


def build(pokered: Path) -> tuple[dict[int, list[dict]], dict]:
    names = map_names(pokered)
    labels_idx = index_text_labels(pokered)
    tables: dict[int, list[dict]] = {}
    stats = {"maps_with_text": 0, "entries": 0, "resolved": 0, "scripted": 0}

    for ind, name in enumerate(names):
        labels = text_pointer_table(pokered, name)
        if not labels:
            continue
        consts = text_pointer_consts(pokered, name)
        signs, people = categorize(pokered, name)

        entries = []
        for i, label in enumerate(labels):
            const = consts[i] if i < len(consts) else None
            string = labels_idx.get(label)
            if const and const in signs:
                cat = "sign"
            elif const and const in people:
                cat = "person"
            else:
                cat = "other"
            entries.append({"id": i + 1, "string": string, "category": cat})
            stats["entries"] += 1
            if string is None:
                stats["scripted"] += 1
            else:
                stats["resolved"] += 1
        tables[ind] = entries
        stats["maps_with_text"] += 1
    return tables, stats


# ── 5. Merge into maps.json (additive only) + validate ──────────────────────────────────────
def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--pokered", type=Path, default=DEFAULT_POKERED)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    if not args.pokered.exists():
        print(f"ERROR: pokered clone not found at {args.pokered}", file=sys.stderr)
        return 2

    tables, stats = build(args.pokered)

    maps = json.loads(read(MAPS_JSON))

    # Validate: every sign's text id must land inside its map's table.
    problems = []
    for entry in maps:
        ind = entry.get("ind")
        table = tables.get(ind, [])
        for s in entry.get("signs", []):
            tid = s.get("text")
            if tid is None:
                continue
            if tid < 1 or tid > len(table):
                problems.append(
                    f"  map {ind} ({entry.get('name')}): sign text id {tid} outside table of {len(table)}")

    print(f"maps with text tables : {stats['maps_with_text']}")
    print(f"text entries total     : {stats['entries']}")
    print(f"  resolved to a string : {stats['resolved']}")
    print(f"  scripted / no literal: {stats['scripted']}")
    if problems:
        print(f"\n!! {len(problems)} sign(s) point outside their map's text table:")
        for p in problems[:40]:
            print(p)
        # Not necessarily fatal (hack saves can too), but for the SHIPPED maps.json every
        # sign is a real one, so any miss here is a parser bug -> fail loudly.
        print("\nFAIL: shipped signs must all resolve. Fix the parser.", file=sys.stderr)
        return 1

    # Merge additively.
    changed = 0
    for entry in maps:
        ind = entry.get("ind")
        table = tables.get(ind)
        if table is None:
            continue
        if entry.get("textEntries") != table:
            changed += 1
        entry["textEntries"] = table

    if args.check:
        # Diff against committed file (does it already match what we'd write?)
        current = json.loads(read(MAPS_JSON))
        drift = 0
        cur_by_ind = {e.get("ind"): e for e in current}
        for entry in maps:
            if cur_by_ind.get(entry["ind"], {}).get("textEntries") != entry.get("textEntries"):
                drift += 1
        if drift:
            print(f"\n--check: {drift} map(s) would change. Run without --check to write.")
            return 1
        print("\n--check: maps.json is up to date.")
        return 0

    MAPS_JSON.write_text(json.dumps(maps, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"\nwrote {MAPS_JSON} ({changed} map(s) gained/changed textEntries)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
