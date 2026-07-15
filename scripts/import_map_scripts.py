#!/usr/bin/env python3
"""
import_map_scripts.py -- each map's SCRIPT STEPS, imported from pret/pokered.

`wCurMapScript` (the save's "Current Script" byte, 0x2CE5) is an INDEX into the current map's
list of scripted-event steps. The list is the map's `<Map>_ScriptPointers` table in
`pret/pokered`, whose entries are named `SCRIPT_<MAP>_<NAME>` (auto-numbered 0, 1, 2, ...). Today
maps.json ships only the raw index; a person editing "Current Script" has just a number. This
imports the NAMES, so the editor can show a descriptive ComboBox ("Default", "Start Battle",
"Marowak Battle", ...) with a "Something else" escape for raw/hack values.

Scope (Twilight, 2026-07-15): the CURRENT map's script steps only -- NOT the separate per-map
progress block (`w<Map>CurScript`, ~90 bytes), which is a later, un-briefed topic.

None of it is invented. Per the standing file-format rule this EXTENDS the map data we already
ship: it adds one field, `scriptEntries`, to each scripted map in maps.json, and NEVER touches an
existing field -- run it and diff, every other byte of maps.json is identical. Same additive shape
as import_sign_text.py's `textEntries`.

Source:  <pokered>/data/maps/map_header_pointers.asm   (map id -> Map base name, in order)
         <pokered>/scripts/<Map>.asm                   (def_script_pointers: dw_const routine, SCRIPT_<MAP>_<NAME>)
Output:  projects/db/assets/data/maps.json  (adds "scriptEntries": [ {id, name, label}, ... ])

Each map's `scriptEntries` is ordered; `id` is the 0-based index `wCurMapScript` holds (the
`def_script_pointers` macro numbers them from 0). `name` is the constant suffix
(`SCRIPT_<MAP>_` stripped), `label` is that prettified ("MAROWAK_BATTLE" -> "Marowak Battle").

Self-checking, because a wrong list would mislabel a save's script state:
  1. Structural: every map with a def_script_pointers table must yield a non-empty scriptEntries.
  2. Additive: only `scriptEntries` is ever set; no other key is added, changed, or removed.

Usage:
    python scripts/import_map_scripts.py [--pokered <path>] [--check]
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


def read(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (FileNotFoundError, UnicodeDecodeError):
        try:
            return path.read_text(encoding="latin-1")
        except FileNotFoundError:
            return ""


# ── ind -> Map file base name (same source/shape as import_sign_text.py) ─────────────────────
def map_names(pokered: Path) -> list[str]:
    txt = read(pokered / "data" / "maps" / "map_header_pointers.asm")
    return [m.group(1) for m in re.finditer(r"^\s*dw\s+(\w+)_h\b", txt, re.M)]


# ── Per-map: the def_script_pointers table (ordered SCRIPT_* const suffixes) ──────────────────
def script_steps(pokered: Path, name: str) -> list[dict]:
    txt = read(pokered / "scripts" / f"{name}.asm")
    if not txt:
        return []
    m = re.search(r"def_script_pointers\b(.*?)(?:^\S|\Z)", txt, re.S | re.M)
    if not m:
        return []
    prefix = f"SCRIPT_{name.upper()}_"
    steps: list[dict] = []
    for line in m.group(1).splitlines():
        line = line.strip()
        md = re.match(r"^dw_const\s+\w+\s*,\s*(\w+)", line)
        if not md:
            # a bare `dw <routine>` with no const still occupies an index; keep alignment.
            if re.match(r"^dw\s+\w+\s*$", line):
                idx = len(steps)
                steps.append({"id": idx, "name": f"UNNAMED_{idx}", "label": f"Step {idx}"})
            continue
        const = md.group(1)
        suffix = const[len(prefix):] if const.startswith(prefix) else \
            (const[len("SCRIPT_"):] if const.startswith("SCRIPT_") else const)
        label = suffix.replace("_", " ").title()
        steps.append({"id": len(steps), "name": suffix, "label": label})
    return steps


def build(pokered: Path) -> tuple[dict[int, list[dict]], dict]:
    names = map_names(pokered)
    tables: dict[int, list[dict]] = {}
    stats = {"maps_with_scripts": 0, "steps": 0}
    for ind, name in enumerate(names):
        steps = script_steps(pokered, name)
        if not steps:
            continue
        tables[ind] = steps
        stats["maps_with_scripts"] += 1
        stats["steps"] += len(steps)
    return tables, stats


# ── Merge into maps.json (additive only) + validate ──────────────────────────────────────────
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

    # Validate: every produced table must be non-empty and every step id sequential from 0.
    problems = []
    for ind, table in tables.items():
        if not table:
            problems.append(f"  map ind {ind}: empty script table")
        for i, step in enumerate(table):
            if step["id"] != i:
                problems.append(f"  map ind {ind}: step {i} has non-sequential id {step['id']}")

    print(f"maps with script tables : {stats['maps_with_scripts']}")
    print(f"script steps total       : {stats['steps']}")
    if problems:
        print(f"\n!! {len(problems)} structural problem(s):")
        for p in problems[:40]:
            print(p)
        print("\nFAIL: fix the parser.", file=sys.stderr)
        return 1

    # Merge additively -- set ONLY scriptEntries, touch nothing else.
    changed = 0
    for entry in maps:
        table = tables.get(entry.get("ind"))
        if table is None:
            continue
        if entry.get("scriptEntries") != table:
            changed += 1
        entry["scriptEntries"] = table

    if args.check:
        current = json.loads(read(MAPS_JSON))
        cur_by_ind = {e.get("ind"): e for e in current}
        drift = sum(
            1 for e in maps
            if cur_by_ind.get(e["ind"], {}).get("scriptEntries") != e.get("scriptEntries")
        )
        if drift:
            print(f"\n--check: {drift} map(s) would change. Run without --check to write.")
            return 1
        print("\n--check: maps.json is up to date.")
        return 0

    MAPS_JSON.write_text(json.dumps(maps, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"\nwrote {MAPS_JSON} ({changed} map(s) gained/changed scriptEntries)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
