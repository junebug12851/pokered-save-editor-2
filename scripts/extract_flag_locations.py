#!/usr/bin/env python3
"""Phase 9 — extract the flag <-> map location / object association.

Two products, both from pret/pokered source (no ROM):

1. OBJECT INVENTORY (high confidence). Every map's `def_object_events`: each
   object's tile (X, Y), sprite, movement, and KIND — npc (plain text id),
   trainer (OPP_*), or item (ITEM ball). Parallel `object_const_def` gives each
   object its const name (Nth const <-> Nth object).

2. FLAG <-> OBJECT LINKAGE (best-effort). In each map script, the pattern is
       CheckEvent <flag> ... ld a, <OBJCONST> ... predef ShowObject|HideObject
   so within a routine (between labels) we pair the CheckEvent flags with the
   object consts toggled there, resolving the const to its object's coords via
   the parallel list. Item balls attach to their got-item flag. This is the data
   that colours a hotspot (flag-attached vs plain) and focuses the panel on click.

Linkage is heuristic (assembly control-flow, not perfect); every link carries a
`confidence`. The inventory is exact.

Output (git-ignored): tmp/event-flags/flag_locations.json
                      tmp/event-flags/flag_locations_summary.txt
"""
from __future__ import annotations
import json
import os
import re
import sys

HOME = os.path.expanduser("~")
DEFAULT_PK = os.path.join(HOME, "Documents", "projects", "pokered")
REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(REPO, "tmp", "event-flags")

OBJ_RE = re.compile(r"^\s*object_event\s+(.+)$")
CONST_RE = re.compile(r"^\s*(?:const|const_export)\s+(\w+)")
LABEL_RE = re.compile(r"^([A-Za-z_.][\w.]*):")
LDA_RE = re.compile(r"^\s*ld\s+a,\s*([A-Za-z_]\w*)\s*$")
SHOWHIDE_RE = re.compile(r"predef\s+(ShowObject|HideObject)")
CHECKEVENT_RE = re.compile(r"CheckEvent\w*\s+(EVENT_\w+)")
EVENTOP_RE = re.compile(r"(?:Set|Reset|Check)Event\w*\s+(EVENT_\w+)")


def parse_object_file(path):
    """Return (object_consts[list], objects[list of dict]) in parallel order."""
    consts, objects = [], []
    in_consts = in_objs = False
    with open(path, encoding="utf-8") as fh:
        for raw in fh:
            line = raw.split(";", 1)[0].rstrip()
            s = line.strip()
            if s == "object_const_def":
                in_consts, in_objs = True, False
                continue
            if s == "def_object_events":
                in_consts, in_objs = False, True
                continue
            if s.startswith("def_warps_to") or s.startswith("def_bg_events") \
                    or s.startswith("def_warp_events"):
                in_consts = in_objs = False
            if in_consts:
                m = CONST_RE.match(line)
                if m:
                    consts.append(m.group(1))
            elif in_objs:
                m = OBJ_RE.match(line)
                if m:
                    args = [a.strip() for a in m.group(1).split(",")]
                    if len(args) < 6:
                        continue
                    x, y, sprite = args[0], args[1], args[2]
                    rest = " ".join(args[5:])
                    if "OPP_" in rest:
                        kind = "trainer"
                    elif "ITEM" in " ".join(args[5:6]) or (len(args) == 7):
                        kind = "item"
                    else:
                        kind = "npc"
                    objects.append({
                        "x": _num(x), "y": _num(y), "sprite": sprite,
                        "movement": args[3], "facing": args[4],
                        "kind": kind, "target": args[5],
                    })
    return consts, objects


def _num(t):
    t = t.strip()
    try:
        return int(t)
    except ValueError:
        return t  # a constant expression; keep as text


TOGGLEIDX_RE = re.compile(r"ld\s+\[wToggleableObjectIndex\],\s*a")


def parse_script_links(path, const_to_obj):
    """Best-effort flag<->object links + the set of ever-toggled (conditional)
    object consts. Pattern:
        [CheckEvent <flag> ...] ld a, <CONST> ; ld [wToggleableObjectIndex], a
        ; predef ShowObject|HideObject
    Returns (links[list], toggled[set of const])."""
    links, toggled = [], set()
    if not os.path.exists(path):
        return links, toggled
    routine_flags = []          # flags seen since the last label
    pending_lda = None          # last 'ld a, <CONST>'
    cur_obj = None              # const stored to wToggleableObjectIndex
    with open(path, encoding="utf-8") as fh:
        for raw in fh:
            line = raw.split(";", 1)[0]
            if LABEL_RE.match(line) and not line.startswith((" ", "\t")):
                routine_flags = []      # new routine
                pending_lda = cur_obj = None
            m = CHECKEVENT_RE.search(line)
            if m:
                routine_flags.append(m.group(1))
            lm = LDA_RE.match(line)
            if lm:
                pending_lda = lm.group(1)
            if TOGGLEIDX_RE.search(line):
                cur_obj = pending_lda
            sm = SHOWHIDE_RE.search(line)
            if sm and cur_obj and cur_obj in const_to_obj:
                toggled.add(cur_obj)
                for fl in routine_flags[-2:]:   # nearest checked flags
                    links.append({
                        "flag": fl, "object_const": cur_obj,
                        "kind": "show" if sm.group(1) == "ShowObject" else "hide",
                        "confidence": "medium",
                    })
                cur_obj = None
    return links, toggled


def main() -> int:
    pk = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PK
    obj_dir = os.path.join(pk, "data", "maps", "objects")
    scr_dir = os.path.join(pk, "scripts")
    if not os.path.isdir(obj_dir):
        sys.exit(f"objects dir not found: {obj_dir}")

    maps = {}
    n_obj = n_item = n_trainer = n_link = 0
    for fn in sorted(os.listdir(obj_dir)):
        if not fn.endswith(".asm"):
            continue
        mapname = fn[:-4]
        consts, objects = parse_object_file(os.path.join(obj_dir, fn))
        # parallel: const[i] <-> object[i]
        const_to_obj = {}
        for i, o in enumerate(objects):
            o["const"] = consts[i] if i < len(consts) else None
            if o["const"]:
                const_to_obj[o["const"]] = o
        links, toggled = parse_script_links(os.path.join(scr_dir, fn), const_to_obj)
        # attach flags to objects
        for o in objects:
            o["flags"] = sorted({l["flag"] for l in links
                                 if l["object_const"] == o.get("const")})
            o["conditional"] = o.get("const") in toggled   # appears/disappears
            if o["kind"] == "item":
                o["flags"] = o["flags"] or ["<got-item event (map-specific)>"]
            # flag-governed = has a flag, OR is toggled (missable), OR item/trainer
            o["flag_attached"] = bool(o["flags"]) or o["conditional"] \
                or o["kind"] in ("item", "trainer")
        n_obj += len(objects)
        n_item += sum(1 for o in objects if o["kind"] == "item")
        n_trainer += sum(1 for o in objects if o["kind"] == "trainer")
        n_link += len(links)
        maps[mapname] = {"objects": objects, "links": links}

    os.makedirs(OUT_DIR, exist_ok=True)
    with open(os.path.join(OUT_DIR, "flag_locations.json"), "w", encoding="utf-8") as fh:
        json.dump(maps, fh, indent=1)

    linked_objs = sum(1 for m in maps.values() for o in m["objects"] if o["flag_attached"])
    summary = [
        f"maps parsed:            {len(maps)}",
        f"objects (total):        {n_obj}",
        f"  trainers:             {n_trainer}",
        f"  item balls:           {n_item}",
        f"  npc/other:            {n_obj - n_trainer - n_item}",
        f"flag<->object links:    {n_link} (best-effort)",
        f"objects flag-attached:  {linked_objs}",
    ]
    with open(os.path.join(OUT_DIR, "flag_locations_summary.txt"), "w", encoding="utf-8") as fh:
        fh.write("\n".join(summary) + "\n")
    print("\n".join(summary))
    print(f"out -> {os.path.join(OUT_DIR, 'flag_locations.json')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
