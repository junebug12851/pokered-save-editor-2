#!/usr/bin/env python3
"""Phases 3 + 5 — turn the usage evidence into a curated dossier for ALL 2,560
event flags: friendly name, description, map, flag-group, and classification.

Input:  tmp/event-flags/event_usage.json  (Phase 2)
Output: tmp/event-flags/events_dossiers.json   (the full 2,560-entry dataset)
        tmp/event-flags/unknown_candidates.json (gaps with no discoverable
                                                  identity — need leadership
                                                  sign-off to name "Unknown #<hex>")

Naming rule (project leadership, 2026-07-15): a real, discerned name wherever
findable; "Unknown #<hex id>" ONLY after exhausting every file (Phase 2 did the
whole tree, by name + range + raw index) AND with explicit leadership sign-off,
per-flag or per-group. So this generator does NOT finalize any "Unknown" name —
it lists the candidates and marks them provisional until sign-off.

This is a derivation, re-runnable from source; the descriptions are grounded in
the pret name + the disassembly evidence, ready for editorial refinement.
"""
from __future__ import annotations
import json
import os
import re

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(REPO, "tmp", "event-flags")
USAGE = os.path.join(OUT_DIR, "event_usage.json")

ACRONYMS = {"TM", "HM", "SS", "PC", "HP", "OT", "NPC", "2F", "1F", "3F", "4F",
            "5F", "6F", "7F", "8F", "9F", "10F", "11F", "B1F", "B2F", "B3F",
            "B4F"}


def friendly(name: str) -> str:
    """EVENT_BEAT_CERULEAN_GYM_TRAINER_0 -> 'Beat Cerulean Gym Trainer 0'."""
    body = name[len("EVENT_"):] if name.startswith("EVENT_") else name
    out = []
    for tok in body.split("_"):
        if not tok:
            continue
        if tok in ACRONYMS or re.fullmatch(r"[A-Z]{1,3}\d+", tok) or tok.isdigit():
            out.append(tok)
        else:
            out.append(tok.capitalize())
    return " ".join(out)


def region_to_map(section: str) -> str:
    """'Pallet Town events' -> 'Pallet Town'."""
    return re.sub(r"\s*events?$", "", section).strip() or "General"


def group_of(name: str, mapname: str):
    """Derive a descriptive flag-group for co-toggled sets."""
    if not name:
        return None
    b = name[len("EVENT_"):]
    m = re.match(r"BEAT_(.+)_GYM_TRAINER_\d+$", b)
    if m:
        return f"{mapname} Gym — trainers"
    if re.match(r"BEAT_.+_GYM_(LEADER|GIOVANNI|BROCK|MISTY|SURGE|ERIKA|KOGA|"
                r"SABRINA|BLAINE)$", b) or b.startswith("BEAT_") and "GYM" in b:
        return f"{mapname} Gym"
    if re.search(r"_TRAINER_\d+$", b):
        return f"{mapname} — trainers"
    if b.startswith("GOT_") or "_ITEM" in b or b.startswith("PICKED_UP") or "HIDDEN" in b:
        return f"{mapname} — items"
    if "BOULDER" in b or "SWITCH" in b or "STRENGTH" in b:
        return f"{mapname} — boulder/switch puzzle"
    if "ROCKET" in b or "GRUNT" in b:
        return f"{mapname} — Team Rocket"
    if "RIVAL" in b:
        return f"{mapname} — rival"
    if "SNORLAX" in b:
        return "Snorlax (Route 12 / 16)"
    if "SS_ANNE" in b or "SSANNE" in b:
        return "S.S. Anne storyline"
    return f"{mapname} — story"


def classify(r):
    cls = []
    if not r["used"]:
        cls.append("unused")
    else:
        cls.append("used")
    if r["temporary"]:
        cls.append("temporary")
    if r["block_swept"]:
        cls.append("block-swept")
    if r["multi_map"]:
        cls.append("multi-map")
    if r["name"] and not r["used"]:
        cls.append("defined-unused")
    return cls


def describe(r, fname, mapname, cls):
    idx = r["index_hex"]
    loc = f"byte {r['byte_file']} bit {r['bit']} (index {idx})"
    if r["name"]:
        if r["used"]:
            bits = []
            if r["n_set"]:
                bits.append(f"set {r['n_set']}x")
            if r["n_check"]:
                bits.append(f"checked {r['n_check']}x")
            if r["n_reset"]:
                bits.append(f"cleared {r['n_reset']}x")
            ev = ", ".join(bits) if bits else "referenced in data"
            desc = (f"{fname}. Progress/story flag for {mapname}; the game {ev}. "
                    f"Save {loc}.")
            if r["temporary"]:
                desc += " Temporary — the game clears it again during play."
            if r["multi_map"]:
                desc += " Referenced from more than one area."
            return desc
        # named but never referenced
        return (f"{fname}. Named in pret/pokered but the shipped game never sets "
                f"or checks it — a defined-but-unused flag. Save {loc}.")
    # unnamed gap
    if r["block_swept"]:
        return (f"Unnamed bit in the {mapname} event block. It has no individual "
                f"name, but a range operation (Set/ResetEventRange) sweeps it as "
                f"part of a block set/clear — e.g. clearing a whole area's events. "
                f"Save {loc}.")
    return (f"Unused event bit in the {mapname} event block. No reference anywhere "
            f"in the disassembly (checked by name, range, and raw index). Identity "
            f"undiscoverable — an 'Unknown #{idx[2:].upper()}' candidate pending "
            f"project-leadership sign-off. Save {loc}.")


def main() -> int:
    with open(USAGE, encoding="utf-8") as fh:
        usage = json.load(fh)

    dossiers = []
    unknown_candidates = []
    counts = {"named": 0, "block_swept_gap": 0, "unknown_candidate": 0}

    for r in usage:
        mapname = region_to_map(r["section"])
        cls = classify(r)
        if r["name"]:
            fname = friendly(r["name"])
            unknown = False
            counts["named"] += 1
        elif r["block_swept"]:
            rg = (r.get("range_groups") or [f"{mapname} block"])[0]
            fname = f"{rg} — member bit"
            unknown = False
            counts["block_swept_gap"] += 1
        else:
            # undiscoverable — provisional Unknown, pending sign-off
            fname = f"Unknown #{r['index_hex'][2:].upper()}"
            unknown = True
            counts["unknown_candidate"] += 1

        # A range sweep IS a flag group (project leadership): prefer it, since it
        # is evidence-based (these bits are literally toggled together).
        rgroups = r.get("range_groups", [])
        group = rgroups[0] if rgroups else group_of(r["name"], mapname)

        entry = {
            "index": r["index"],
            "index_hex": r["index_hex"],
            "byte_file": r["byte_file"],
            "byte_wram": r["byte_wram"],
            "bit": r["bit"],
            "pret_name": r["name"],
            "name": fname,
            "map": mapname,
            "group": group,
            "range_groups": rgroups,
            "classification": cls,
            "unknown_candidate": unknown,
            "name_provisional": unknown,   # name not final until sign-off
            "crash": None,                 # Phase 4 (console-probed) fills this
            "description": describe(r, fname, mapname, cls),
            "evidence": {
                "used": r["used"], "n_set": r["n_set"], "n_check": r["n_check"],
                "n_reset": r["n_reset"], "temporary": r["temporary"],
                "block_swept": r["block_swept"], "multi_map": r["multi_map"],
                "files": r["files"],
            },
        }
        dossiers.append(entry)
        if unknown:
            unknown_candidates.append({
                "index_hex": r["index_hex"], "map": mapname,
                "byte_file": r["byte_file"], "bit": r["bit"],
            })

    with open(os.path.join(OUT_DIR, "events_dossiers.json"), "w", encoding="utf-8") as fh:
        json.dump(dossiers, fh, indent=1)
    with open(os.path.join(OUT_DIR, "unknown_candidates.json"), "w", encoding="utf-8") as fh:
        json.dump(unknown_candidates, fh, indent=1)

    # group summary
    groups = {}
    for e in dossiers:
        if e["group"]:
            groups[e["group"]] = groups.get(e["group"], 0) + 1

    print(f"dossiers written: {len(dossiers)} / 2560")
    print(f"  named (pret):            {counts['named']}")
    print(f"  block-swept gaps:        {counts['block_swept_gap']}")
    print(f"  Unknown candidates:      {counts['unknown_candidate']}  (need sign-off)")
    print(f"flag-groups derived: {len(groups)}")
    top = sorted(groups.items(), key=lambda kv: -kv[1])[:12]
    for g, n in top:
        print(f"    {g:42} {n}")
    print(f"out -> {OUT_DIR}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
