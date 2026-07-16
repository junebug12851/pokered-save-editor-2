#!/usr/bin/env python3
"""Phase 4 (contradictions) — find flag SETS that must not be on together.

Leadership's archetype: the two Route 22 rival battles (1ST + 2ND) share one
NPC/script, so both on collides on battle. This surfaces such contradiction
candidates from two signals:

1. Same-subject multi-state flags — several named flags for one subject that are
   mutually-exclusive story states: 1ST/2ND versions, WANTS_BATTLE + BEAT,
   FIGHT + BEAT of the same trainer/event.
2. Same-object governors — a single map object driven by >1 event flag
   (from tmp/event-flags/flag_locations.json).

Output: tmp/event-flags/contradictions.json + a printed summary. These feed the
UI's per-combo warnings and the bulk-set crash guard.
"""
from __future__ import annotations
import json
import os
import re
from collections import defaultdict

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(REPO, "tmp", "event-flags")
USAGE = os.path.join(OUT, "event_usage.json")
LOCS = os.path.join(OUT, "flag_locations.json")

# tokens that mark a *state* of a shared subject (strip to get the subject)
STATE_TOK = re.compile(
    r"_(1ST|2ND|3RD)?_?(RIVAL_)?(BATTLE|WANTS_BATTLE|FIGHT|BEAT|IN_PROGRESS)$")


def subject(name: str) -> str:
    b = name[len("EVENT_"):] if name.startswith("EVENT_") else name
    b = re.sub(r"^(BEAT_|FIGHT_|GOT_)", "", b)
    b = re.sub(r"_(1ST|2ND|3RD)_(ROUTE\d+_)?RIVAL_BATTLE$", "_RIVAL", b)
    b = re.sub(r"_(1ST|2ND|3RD)_BATTLE$", "", b)
    b = re.sub(r"_WANTS_BATTLE$", "", b)
    b = re.sub(r"_(BATTLE|IN_PROGRESS)$", "", b)
    return b


def main():
    usage = json.load(open(USAGE, encoding="utf-8"))
    named = [r for r in usage if r["name"]]

    # 1) same-subject multi-state
    by_subject = defaultdict(list)
    for r in named:
        n = r["name"]
        if re.search(r"(1ST|2ND|3RD)_.*BATTLE|WANTS_BATTLE|FIGHT_|_IN_PROGRESS", n):
            by_subject[subject(n)].append(n)
    subject_sets = {s: sorted(set(v)) for s, v in by_subject.items() if len(set(v)) > 1}

    # 2) same-object governors (from flag_locations)
    object_sets = []
    if os.path.exists(LOCS):
        locs = json.load(open(LOCS, encoding="utf-8"))
        for mapname, m in locs.items():
            for o in m["objects"]:
                fl = [f for f in o.get("flags", []) if f.startswith("EVENT_")]
                if len(fl) > 1:
                    object_sets.append({
                        "map": mapname, "object": o.get("const"),
                        "coord": [o.get("x"), o.get("y")], "flags": sorted(set(fl)),
                    })

    out = {
        "same_subject_multistate": subject_sets,
        "same_object_governors": object_sets,
    }
    json.dump(out, open(os.path.join(OUT, "contradictions.json"), "w",
                        encoding="utf-8"), indent=1)

    print(f"same-subject multi-state contradiction sets: {len(subject_sets)}")
    for s, v in sorted(subject_sets.items())[:12]:
        print(f"  {s:34} {v}")
    print(f"\nsame-object governor sets (>1 flag on one object): {len(object_sets)}")
    for e in object_sets[:12]:
        print(f"  {e['map']:14} {e['object']:24} {e['flags']}")
    print(f"\nout -> {os.path.join(OUT, 'contradictions.json')}")


if __name__ == "__main__":
    main()
