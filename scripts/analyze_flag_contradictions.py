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

    # --- the conflicting-flags dataset (Phase 11 schema) ----------------
    # condition: not-both-on | not-both-off | at-most-one | exactly-one | ...
    # status:    suspected (static) | confirmed (console-reproduced)
    # Subjects whose suspicion has been ADJUDICATED and REFUTED — the static
    # heuristic keeps re-raising them, so it must defer to the verdict. (Negative
    # knowledge is knowledge: never re-suspect a settled question.)
    REFUTED_SUBJECTS = {"ROUTE22_RIVAL"}

    conflicts = []
    for subj, flags in subject_sets.items():
        if subj in REFUTED_SUBJECTS:
            continue   # settled below as an explicit 'refuted' entry
        conflicts.append({
            "id": f"subject:{subj}",
            "flags": flags,
            "condition": "at-most-one-on",
            # SUSPECTED ONLY — a same-subject cluster is a lead, NOT evidence. The
            # Route 22 case proved script dispatch order can mask one flag entirely,
            # so this never renders as a crash warning until console-confirmed.
            "status": "suspected",
            "severity": "unknown-pending-adjudication",
            "map": None,
            "reason": (f"{subj}: multiple mutually-exclusive battle/state flags for one "
                       f"subject. LEAD ONLY — must be adjudicated on the console before "
                       f"being shown as a risk (cf. ROUTE22_RIVAL, refuted)."),
            "evidence": None,
        })
    for e in object_sets:
        conflicts.append({
            "id": f"object:{e['map']}:{e['object']}",
            "flags": e["flags"],
            "condition": "not-both-on",
            "status": "suspected",
            "severity": "softlock",
            "map": e["map"],
            "reason": (f"one object ({e['object']} @ {e['coord']}) is governed by "
                       f"multiple flags; both on can show/hide it inconsistently."),
            "evidence": None,
        })
    # The Route 22 rival — the archetype, and the system's first REFUTATION.
    # Suspicion: two SPRITE_BLUE at the same tile (25,5), one per battle, so both
    # battle flags on "must" collide. WRONG. Route22DefaultScript is an ordered
    # if/else: `CheckEvent 1ST; jr nz -> FirstRivalBattle` — with 1ST set the 2ND
    # is NEVER consulted, so it is masked, not conflicting. Console agrees: both
    # flags + both sprites shown -> a normal trainer battle (mode=2), no crash.
    # Kept as NEGATIVE KNOWLEDGE so it is never re-raised.
    conflicts.append({
        "id": "route22-rival-overlap",
        "flags": ["EVENT_1ST_ROUTE22_RIVAL_BATTLE", "EVENT_2ND_ROUTE22_RIVAL_BATTLE",
                  "EVENT_ROUTE22_RIVAL_WANTS_BATTLE"],
        "condition": "at-most-one-on",
        "status": "refuted",
        "severity": "none",
        "map": "Route22",
        "reason": ("REFUTED 2026-07-16 (console + source). Route22DefaultScript checks "
                   "1ST before 2ND in an ordered if/else, so with 1ST set the 2ND flag is "
                   "never read — masked, not conflicting. The two stacked SPRITE_BLUE "
                   "objects merely overlap; only one is ever driven. Both flags on + both "
                   "sprites shown engages a NORMAL trainer battle."),
        "evidence": ("scripts/emu/probe_route22_conflict.py -> 'conflict -> HEALTHY trainer "
                     "battle (mode=2, script=2)'; source: scripts/Route22.asm"),
    })
    # …but the probe surfaced a REAL candidate: armed WITHOUT the rival sprite.
    conflicts.append({
        "id": "route22-rival-armed-but-hidden",
        "flags": ["EVENT_ROUTE22_RIVAL_WANTS_BATTLE", "EVENT_1ST_ROUTE22_RIVAL_BATTLE"],
        "condition": "flags-armed-while-object-hidden",
        "status": "suspected",
        "severity": "softlock",
        "map": "Route22",
        "reason": ("Flag ↔ missable inconsistency: the flags arm the ambush but the rival "
                   "object is hidden, so the coord trigger fires and the script advances to "
                   "1 yet no battle ever engages (observed: 'ghost -> NO BATTLE (script=1)' "
                   "— stalled). Needs its own probe to confirm whether the player is truly "
                   "stuck."),
        "evidence": "scripts/emu/probe_route22_conflict.py (ghost scenario)",
    })
    json.dump({"conflicts": conflicts}, open(os.path.join(OUT, "conflicts.json"), "w",
              encoding="utf-8"), indent=1)
    print(f"\nconflicts dataset: {len(conflicts)} rules -> conflicts.json")

    print(f"same-subject multi-state contradiction sets: {len(subject_sets)}")
    for s, v in sorted(subject_sets.items())[:12]:
        print(f"  {s:34} {v}")
    print(f"\nsame-object governor sets (>1 flag on one object): {len(object_sets)}")
    for e in object_sets[:12]:
        print(f"  {e['map']:14} {e['object']:24} {e['flags']}")
    print(f"\nout -> {os.path.join(OUT, 'contradictions.json')}")


if __name__ == "__main__":
    main()
