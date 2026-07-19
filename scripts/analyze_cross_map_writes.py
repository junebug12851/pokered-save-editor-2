#!/usr/bin/env python3
"""Cross-map flag writes — the global-progression evidence base (2026-07-19).

The map-states pass (extract_map_states.py) modelled each map's own timeline.
This script answers the question that pass could not: WHO ELSE writes a map's
flags?  It mines pret/pokered for every event-flag write and every missable
(ShowObject/HideObject) toggle whose WRITER is a different map than the flag's
OWNER, plus the gate checks (badge bits, key items in bag) that join the maps
into one progression graph.

Classes it separates:
  event_set / event_reset   cross-block SetEvent*/ResetEvent* (writer file vs
                            the flag's event_constants.asm block)
  toggle                    cross-map Show/HideObject — BOTH the `ld a, TOGGLE_X`
                            form AND the `db TOGGLE_X` table form (Silph Co 11F's
                            Saffron liberation sweep lives in a db table and is
                            invisible to the ld-only regex — the bug this script
                            exists to avoid)
  badge_check               `bit BIT_*BADGE` reads (gates + field-move gating)
  item_check                `ld b, ITEM / call|jp IsItemInBag` (key-item gates)

Inputs:  the pret/pokered clone + tmp/event-flags/event_usage.json
         (run import_event_flags.py + analyze_event_usage.py first).
Output:  tmp/event-flags/cross_map_writes.json + a printed summary.
Reference note: notes/reference/game-progression.md
"""
from __future__ import annotations
import json
import os
import re
import sys
from collections import defaultdict

HOME = os.path.expanduser("~")
DEFAULT_PK = os.path.join(HOME, "Documents", "projects", "pokered")
REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(REPO, "tmp", "event-flags")
USAGE = os.path.join(OUT_DIR, "event_usage.json")


def norm(s: str) -> str:
    return re.sub(r"[^a-z0-9]", "", s.lower())


def same_block(section: str, file_base: str) -> bool:
    """Is this writer file part of the flag's own map block?

    Blocks span several maps (the 'Lavender Town events' block covers the
    Pokemon Tower floors), so the first word of the block name appearing in
    the file base counts as same-block — the interesting edges are the ones
    that survive even this generous test.
    """
    secn = norm(section.replace(" events", ""))
    basen = norm(file_base)
    base2 = re.sub(r"\d+$", "", basen)  # CeruleanCity_2 -> ceruleancity
    if secn and (secn in basen or basen in secn or secn in base2 or base2 in secn):
        return True
    words = [norm(w) for w in section.replace(" events", "").split()]
    return bool(words and words[0] and words[0] in basen)


def mine_events(usage):
    out = []
    for r in usage:
        sec = r.get("section", "")
        for x in r["refs"]:
            if x["kind"] not in ("set", "reset"):
                continue
            m = re.match(r"(?:scripts|engine|home)/(.+)\.asm$",
                         x["file"].replace("\\", "/"))
            base = m.group(1).split("/")[-1] if m else x["file"]
            if same_block(sec, base):
                continue
            out.append({
                "class": "event_" + x["kind"],
                "index": r["index"],
                "name": r["name"] or "#" + r["index_hex"],
                "owner": sec.replace(" events", ""),
                "writer": x["file"],
                "line": x["line"],
            })
    return out


def toggle_owners(pk):
    owner, cur = {}, None
    with open(os.path.join(pk, "constants", "toggle_constants.asm"),
              encoding="utf-8") as fh:
        for line in fh:
            m = re.match(r"\s*toggle_consts_for\s+(\S+)", line)
            if m:
                cur = m.group(1)
                continue
            m = re.match(r"\s*const\s+(TOGGLE_\S+)", line)
            if m:
                owner[m.group(1)] = cur
    return owner


def mine_toggles(pk, owner):
    """Every Show/HideObject toggle write, ld-form AND db-table form."""
    out = []
    for root, _dirs, files in os.walk(pk):
        if os.sep + ".git" in root:
            continue
        for fn in files:
            if not fn.endswith(".asm"):
                continue
            rel = os.path.relpath(os.path.join(root, fn), pk).replace("\\", "/")
            if rel.startswith(("constants/", "data/maps/toggleable_objects",)):
                continue
            lines = open(os.path.join(root, fn), encoding="utf-8",
                         errors="replace").readlines()
            label = None
            for i, l in enumerate(lines):
                lm = re.match(r"(\.?\w+):", l)
                if lm:
                    label = lm.group(1)
                m = re.search(r"\bld a, (TOGGLE_[A-Z0-9_]+)", l)
                verb, tog = None, None
                if m:
                    tog = m.group(1)
                    for j in range(i, min(i + 6, len(lines))):
                        if "ShowObject" in lines[j]:
                            verb = "show"
                            break
                        if "HideObject" in lines[j]:
                            verb = "hide"
                            break
                    verb = verb or "indirect"
                else:
                    m = re.search(r"\bdb (TOGGLE_[A-Z0-9_]+)", l)
                    if m:
                        tog = m.group(1)
                        lab = label or ""
                        verb = ("hide" if "Hide" in lab
                                else "show" if "Show" in lab else "table")
                if not tog:
                    continue
                own = owner.get(tog, "?")
                base = re.sub(r"\.asm$", "", rel.split("/")[-1])
                if norm(own) in norm(base) or norm(base) in norm(own):
                    continue
                out.append({"class": "toggle", "toggle": tog, "verb": verb,
                            "owner": own, "writer": rel, "line_no": i + 1})
    return out


def mine_gates(pk):
    out = []
    for root, _dirs, files in os.walk(pk):
        if os.sep + ".git" in root:
            continue
        for fn in files:
            if not fn.endswith(".asm"):
                continue
            rel = os.path.relpath(os.path.join(root, fn), pk).replace("\\", "/")
            lines = open(os.path.join(root, fn), encoding="utf-8",
                         errors="replace").readlines()
            for i, l in enumerate(lines):
                m = re.search(r"bit (BIT_\w*BADGE)", l)
                if m:
                    out.append({"class": "badge_check", "badge": m.group(1),
                                "file": rel, "line_no": i + 1})
                m = re.search(r"ld b, ([A-Z0-9_]+)\s*$", l)
                if m and i + 1 < len(lines) and "IsItemInBag" in lines[i + 1]:
                    out.append({"class": "item_check", "item": m.group(1),
                                "file": rel, "line_no": i + 1})
    return out


def main() -> int:
    pk = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PK
    if not os.path.isdir(pk):
        sys.exit(f"pokered tree not found: {pk}")
    if not os.path.exists(USAGE):
        sys.exit("run import_event_flags.py + analyze_event_usage.py first")
    usage = json.load(open(USAGE, encoding="utf-8"))

    events = mine_events(usage)
    toggles = mine_toggles(pk, toggle_owners(pk))
    gates = mine_gates(pk)

    result = {"events": events, "toggles": toggles, "gates": gates}
    os.makedirs(OUT_DIR, exist_ok=True)
    out_path = os.path.join(OUT_DIR, "cross_map_writes.json")
    with open(out_path, "w", encoding="utf-8") as fh:
        json.dump(result, fh, indent=1)

    by = defaultdict(int)
    for e in events:
        by[e["class"]] += 1
    print(f"cross-block event writes: {len(events)} "
          f"(set={by['event_set']} reset={by['event_reset']})")
    print(f"cross-map toggles: {len(toggles)} "
          f"(db-table form: {sum(1 for t in toggles if t['verb'] in ('show','hide','table') and 'db' not in t)} incl.)")
    print(f"gate checks: {len(gates)} "
          f"(badge={sum(1 for g in gates if g['class']=='badge_check')} "
          f"item={sum(1 for g in gates if g['class']=='item_check')})")
    print(f"out -> {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
