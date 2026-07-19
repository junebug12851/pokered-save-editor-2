#!/usr/bin/env python3
"""Filter-flag (missable) lifecycle dossiers — the 2026-07-19 deep dive.

For every toggleable object (the save's 228 missable bits, v1's "filter flags"),
answer the questions the per-map view can't: what IS the thing (sprite/item),
what is its DEFAULT state at new game, WHO shows/hides it (every call site,
including the `db TOGGLE_*` table form and the engine), and WHEN in the game
that happens (joined against the map-state blueprints' per-stage visibility).

Classes derived per flag:
  static      never script-toggled (pret's X marks): items + one-shot static
              encounters the engine deactivates via wToggleableObjectList
  one_shot    shown or hidden once by story scripts and left there
  cutscene    shown then re-hidden inside one map's own cutscene (Oak's shape)
  rearmed     rewritten from OUTSIDE per approach (Seafoam / Victory Road)
  sweep       flipped in a bulk liberation table (Silph 11F's Saffron sweep)

Inputs: the pret/pokered clone + projects/db/assets/data/missables.json +
        projects/db/assets/data/map-states/*.json
Output: tmp/filter-flags/filter_flag_dossiers.json + a printed summary.
Reference note: notes/reference/filter-flags.md
"""
from __future__ import annotations
import glob
import json
import os
import re
import sys
from collections import defaultdict

HOME = os.path.expanduser("~")
DEFAULT_PK = os.path.join(HOME, "Documents", "projects", "pokered")
REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(REPO, "projects", "db", "assets", "data")
OUT_DIR = os.path.join(REPO, "tmp", "filter-flags")

# The known bulk-sweep tables (writer -> the moment). Route20's reconciliation is
# keyed on the boulder hole events; Silph's on beating Giovanni.
SWEEP_FILES = {
    "scripts/SilphCo11F.asm": "Silph liberation (EVENT_BEAT_SILPH_CO_GIOVANNI)",
    "scripts/Route20.asm": "Seafoam reconciliation on leaving the islands",
}
REARM_WRITERS = ("scripts/Route23.asm", "scripts/Route20.asm",
                 "scripts/IndigoPlateauLobby.asm")


def parse_toggles(pk):
    order = []
    owner = {}
    cur = None
    for line in open(os.path.join(pk, "constants", "toggle_constants.asm"),
                     encoding="utf-8"):
        m = re.match(r"\s*toggle_consts_for\s+(\S+)", line)
        if m:
            cur = m.group(1)
            continue
        m = re.match(r"\s*const\s+(TOGGLE_\S+)", line)
        if m:
            owner[m.group(1)] = cur
            order.append(m.group(1))
    return order, owner


def parse_defaults(pk):
    """toggle name -> ('ON'|'OFF', object_const) from toggleable_objects.asm."""
    out = {}
    idx_in_map = defaultdict(int)
    for line in open(os.path.join(pk, "data", "maps", "toggleable_objects.asm"),
                     encoding="utf-8"):
        m = re.match(r"\s*toggle_object_state\s+(\S+?)\s*,\s*(ON|OFF)", line)
        if m:
            out[len(out)] = (m.group(2), m.group(1))
    return out  # keyed by toggle INDEX (file order == const order)


def scan_sites(pk, owner):
    """Every Show/Hide site: (toggle, verb, file, line, routine_label, form)."""
    sites = defaultdict(list)
    for root, _dirs, files in os.walk(pk):
        if os.sep + ".git" in root:
            continue
        for fn in files:
            if not fn.endswith(".asm"):
                continue
            rel = os.path.relpath(os.path.join(root, fn), pk).replace("\\", "/")
            if rel.startswith("constants/") or rel.endswith("toggleable_objects.asm"):
                continue
            lines = open(os.path.join(root, fn), encoding="utf-8",
                         errors="replace").readlines()
            label = ""
            for i, l in enumerate(lines):
                lm = re.match(r"(\.?\w+):", l)
                if lm:
                    label = lm.group(1)
                m = re.search(r"\bld a, (TOGGLE_[A-Z0-9_]+)", l)
                if m:
                    tog = m.group(1)
                    verb = "indirect"
                    for j in range(i, min(i + 6, len(lines))):
                        if "ShowObject" in lines[j]:
                            verb = "show"
                            break
                        if "HideObject" in lines[j]:
                            verb = "hide"
                            break
                    sites[tog].append({"verb": verb, "file": rel, "line": i + 1,
                                       "routine": label, "form": "ld"})
                    continue
                m = re.search(r"\bdb (TOGGLE_[A-Z0-9_]+)", l)
                if m:
                    tog = m.group(1)
                    verb = ("hide" if "Hide" in label
                            else "show" if "Show" in label else "table")
                    sites[tog].append({"verb": verb, "file": rel, "line": i + 1,
                                       "routine": label, "form": "db-table"})
    return sites


def stage_flips(order):
    """toggle missable ind -> [(map, stage id, hide bool)] from the blueprints —
    which researched stages state a visibility for it (join by missables.json ind)."""
    flips = defaultdict(list)
    for f in sorted(glob.glob(os.path.join(DATA, "map-states", "*.json"))):
        if f.endswith("_index.json"):
            continue
        j = json.load(open(f, encoding="utf-8"))
        prev = {}
        for st in j.get("states", []):
            save = st.get("save") or {}
            mis = save.get("missables") or []
            cur = {}
            for e in mis:  # flat list: {ind, name, toggleConst, state: "hide"|"show"}
                cur[e["ind"]] = (e.get("state") == "hide")
            for ind, hide in cur.items():
                if prev.get(ind) != hide:
                    flips[ind].append({"map": j.get("map"), "stage": st["id"],
                                       "hidden": hide})
            if st.get("kind") == "resting":
                prev = cur
    return flips


def classify(d):
    files = {s["file"] for s in d["sites"]}
    if not d["sites"]:
        return "static"
    if any(f in SWEEP_FILES for f in files) and len(d["sites"]) == 1:
        return "sweep"
    if any(f in REARM_WRITERS for f in files) or "BOULDER" in d["const"]:
        return "rearmed"
    verbs = {s["verb"] for s in d["sites"]}
    if "show" in verbs and "hide" in verbs:
        # Shown and hidden by scripts: a cutscene visitor (Oak) or a swap pair.
        return "cutscene_or_swap"
    return "one_shot"


def main() -> int:
    pk = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PK
    if not os.path.isdir(pk):
        sys.exit(f"pokered tree not found: {pk}")

    order, owner = parse_toggles(pk)
    defaults = parse_defaults(pk)
    sites = scan_sites(pk, owner)
    missables = json.load(open(os.path.join(DATA, "missables.json"),
                               encoding="utf-8"))
    by_ind = {m.get("ind"): m for m in missables}
    flips = stage_flips(order)

    dossiers = []
    counts = defaultdict(int)
    for idx, const in enumerate(order):
        dj = by_ind.get(idx, {})
        d = {
            "ind": idx,
            "const": const,
            "map": owner.get(const, "?"),
            "default": defaults.get(idx, ("?", "?"))[0],
            "object": defaults.get(idx, ("?", "?"))[1],
            "name": dj.get("name", ""),
            "desc": dj.get("desc", ""),
            "sites": sites.get(const, []),
            "stageFlips": flips.get(idx, []),
        }
        d["class"] = classify(d)
        counts[d["class"]] += 1
        dossiers.append(d)

    os.makedirs(OUT_DIR, exist_ok=True)
    out = os.path.join(OUT_DIR, "filter_flag_dossiers.json")
    with open(out, "w", encoding="utf-8") as fh:
        json.dump(dossiers, fh, indent=1)

    print(f"toggles: {len(order)}  defaults parsed: {len(defaults)}")
    for k in sorted(counts):
        print(f"  {k:16s} {counts[k]}")
    ex = next(d for d in dossiers if d["const"] == "TOGGLE_PALLET_TOWN_OAK")
    print("\nExemplar TOGGLE_PALLET_TOWN_OAK:")
    print(json.dumps(ex, indent=1))
    print(f"\nout -> {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
