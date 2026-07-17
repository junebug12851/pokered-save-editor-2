#!/usr/bin/env python3
"""Phase 6b/7 — regenerate the DB's events.json from pret (the canonical truth).

WHY: v1's hand-made 508-entry list is byte-correct (`byte == 0x29F3 + ind/8`,
`bit == ind%8`, 0 mismatches) but it MISLABELS 14 bits and invents 14 phantoms in
the Pokémon Tower block (shifted ~2 bits), so the editor writes the wrong flag —
e.g. "Beat Pokemontower 7 Trainer 0" actually sets EVENT_BEAT_GHOST_MAROWAK. It
also reaches only 508 of the 2,560 flags. `ind` IS pret's event index, so the fix
is to regenerate from pret rather than nudge the list.

INPUTS (all produced by the earlier research pipeline):
  tmp/event-flags/event_flags_canonical.json  (ind -> byte/bit/pret name/region)
  tmp/event-flags/events_dossiers.json        (name/description/map/group/class)
  projects/db/assets/data/events.json         (v1 — for its CURATED map lists)
  projects/db/assets/data/maps.json           (the ONLY valid map names)

SCHEMA — strictly ADDITIVE, so existing readers keep working. EventDBEntry parses
name/ind/byte/bit/maps[]; we keep those exactly and add description/group/
classification/placeholder/pretName.

⚠️ MAP NAMES ARE DEEP-LINKED. EventDBEntry resolves every `maps[]` entry against
MapDBEntry and qCritical()s if it can't. v1's file already ships 42 names that do
NOT exist in maps.json (a pre-existing bug — silent qCritical spam). So every name
we emit is VALIDATED against maps.json; anything unresolvable is dropped rather
than shipped broken. Curated v1 lists are PRESERVED where valid (leadership's
"preserve, don't correct" rule) and only supplemented.

Usage:  python scripts/import_events_db.py [--check] [--write]
  --check  report what would change, touch nothing (default)
  --write  rewrite projects/db/assets/data/events.json
"""
from __future__ import annotations
import argparse
import json
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DB = os.path.join(REPO, "projects", "db", "assets", "data")
TMP = os.path.join(REPO, "tmp", "event-flags")
EVENTS_JSON = os.path.join(DB, "events.json")
MAPS_JSON = os.path.join(DB, "maps.json")
CANON = os.path.join(TMP, "event_flags_canonical.json")
DOSSIERS = os.path.join(TMP, "events_dossiers.json")

EV_START = 0x29F3
NUM_EVENTS = 2560


def load(p):
    with open(p, encoding="utf-8") as fh:
        return json.load(fh)


def valid_map_names():
    """Every name a map can legitimately be called: `name` AND `modernName`.

    ⚠️ v1's events.json uses the MODERN names ('Mt Moon 1F', 'Pokemon Tower 2F')
    while maps.json's `name` is the old short form ('Mt. Moon 1', 'Celadon Mart 3').
    Checked against `name` alone, 42 of its 119 look invalid — against
    name+modernName, ZERO are. So the curated map lists are 100% GOOD and must be
    preserved verbatim; the real fault is that EventDBEntry::deepLink() resolves
    only by `name`, which is why those 42 qCritical today. Fix the CODE, not the
    data (leadership: "fix code that uses the data").
    """
    m = load(MAPS_JSON)
    items = m if isinstance(m, list) else (m.get("maps") or list(m.values())[0])
    ok = set()
    for x in items:
        if not isinstance(x, dict):
            continue
        if x.get("name"):
            ok.add(x["name"])
        if x.get("modernName"):
            ok.add(x["modernName"])
    return ok


def script_file_to_map(fname, valid):
    """'scripts/OaksLab.asm' -> 'Oaks Lab' if that is a real map name."""
    base = os.path.basename(fname)
    if not base.endswith(".asm"):
        return None
    stem = base[:-4]
    # CamelCase / digits -> spaced: OaksLab -> Oaks Lab, Route22 -> Route 22
    s = re.sub(r"(?<=[a-z])(?=[A-Z])", " ", stem)
    s = re.sub(r"(?<=[A-Za-z])(?=\d)", " ", s)
    for cand in (s, stem):
        if cand in valid:
            return cand
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--check", action="store_true")
    a = ap.parse_args()

    valid = valid_map_names()
    canon = load(CANON)
    dossiers = {d["index"]: d for d in load(DOSSIERS)}
    old = load(EVENTS_JSON)
    old_by_ind = {e["ind"]: e for e in old}

    rows, stats = [], {"named": 0, "placeholder": 0, "maps_kept": 0,
                       "maps_derived": 0, "maps_dropped": 0, "no_map": 0}

    for r in canon:
        ind = r["index"]
        d = dossiers.get(ind, {})
        pret = r["name"]
        byte = EV_START + ind // 8
        bit = ind % 8
        assert byte == int(d.get("byte_file", hex(byte)), 16) if d.get("byte_file") else True

        # --- map list: preserve v1's curation where VALID, else derive -------
        maps, prev = [], old_by_ind.get(ind)
        if prev and pret and _same(prev.get("name", ""), pret):
            for mn in prev.get("maps", []):
                if mn in valid:
                    maps.append(mn)
                    stats["maps_kept"] += 1
                else:
                    stats["maps_dropped"] += 1          # v1 shipped a bad name
        if not maps:
            for f in (d.get("evidence", {}) or {}).get("files", []):
                mn = script_file_to_map(f, valid)
                if mn and mn not in maps:
                    maps.append(mn)
                    stats["maps_derived"] += 1
        if not maps:
            stats["no_map"] += 1

        row = {
            "name": d.get("name") or (pret or f"Placeholder Flag #{ind:03X}"),
            "ind": ind,
            "byte": byte,
            "bit": bit,
            "maps": maps,
            # --- additive: the research payload -----------------------------
            "pretName": pret or None,
            "description": d.get("description"),
            "group": d.get("group"),
            "classification": d.get("classification", []),
            "placeholder": bool(d.get("placeholder")),
        }
        if d.get("caution"):
            row["caution"] = d["caution"]
        rows.append(row)
        stats["placeholder" if row["placeholder"] else "named"] += 1

    # --- self-validation --------------------------------------------------
    assert len(rows) == NUM_EVENTS, f"expected {NUM_EVENTS}, got {len(rows)}"
    for r in rows:
        assert r["byte"] == EV_START + r["ind"] // 8 and r["bit"] == r["ind"] % 8
        for mn in r["maps"]:
            assert mn in valid, f"invalid map {mn}"

    fixed = [r for r in rows
             if (p := old_by_ind.get(r["ind"])) and r["pretName"]
             and not _same(p.get("name", ""), r["pretName"])]
    phantom = [e for e in old if not any(c["index"] == e["ind"] and c["name"]
                                         for c in canon)]

    print(f"entries: {len(rows)}  (was {len(old)})   named={stats['named']} "
          f"placeholder={stats['placeholder']}")
    print(f"maps: kept={stats['maps_kept']} derived={stats['maps_derived']} "
          f"DROPPED-as-invalid={stats['maps_dropped']}  entries-with-no-map={stats['no_map']}")
    print(f"MISLABELS corrected: {len(fixed)}   phantom entries removed: {len(phantom)}")
    for r in fixed[:12]:
        print(f"   {r['ind']:#05x}  '{old_by_ind[r['ind']]['name']}' -> '{r['name']}'")

    if a.write:
        with open(EVENTS_JSON, "w", encoding="utf-8") as fh:
            json.dump(rows, fh, indent=1, ensure_ascii=False)
        print(f"\nWROTE {EVENTS_JSON}")
    else:
        print("\n(--check: nothing written; pass --write to apply)")
    return 0


def _same(a, b):
    n = lambda s: re.sub(r"[^a-z0-9]", "", s.lower())
    return n(a) == n(b[len("EVENT_"):] if b.startswith("EVENT_") else b)


if __name__ == "__main__":
    raise SystemExit(main())
