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
    name+modernName, ZERO are. So the curated map lists are 100% GOOD and are
    preserved verbatim.

    And there is NO bug here: `MapsDB` indexes `name`, the id, AND `modernName`
    (`ind.insert(entry->modernName, entry)`), so `getIndAt()` — which
    EventDBEntry::deepLink() uses — resolves all 119 today. This validator simply
    mirrors that same index, so it must accept modernName too or it would throw
    away good data. (An earlier draft of this file claimed deepLink resolved by
    `name` only and that those 42 qCritical — that was WRONG, asserted without
    reading MapsDB. Verify before asserting.)
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


# The 9 pret regions that are NOT a single map but a multi-floor LOCATION. A flag
# living in one of these spans the whole location, so per leadership it is assigned
# to EVERY map in it and marked `shared` -- the UI shows it on each map's page in a
# labelled shared group that lets you see/select the other maps it is in. We do NOT
# invent a merged "Silph Co." page: pret's region is a ROM allocation block, and we
# care about the save file, not the ROM's storage layout.
LOCATION_PREFIXES = {
    "Silph Co.":        ["Silph Co"],
    "S.S. Anne":        ["S.S. Anne", "SS Anne"],
    "Pokémon Mansion":  ["Mansion", "Pokemon Mansion"],
    "Rocket Hideout":   ["Rocket Hideout"],
    "Mt. Moon":         ["Mt. Moon", "Mt Moon"],
    "Safari Zone":      ["Safari Zone"],
    "Seafoam Islands":  ["Seafoam Islands"],
    "Cerulean Cave":    ["Cerulean Cave"],
    "Rock Tunnel":      ["Rock Tunnel"],
}


# Prefix matching over-reaches: "Mt. Moon Pokecenter" is a separate building out on
# Route 4, NOT part of the Mt. Moon cave whose block these bits sit in -- filing the
# cave's padding there would be plainly wrong. Excluded by name.
LOCATION_EXCLUDE = ("pokecenter", "pokémon center", "pokemon center", "mart")


def location_members(region, map_rows):
    """Every real map making up a multi-floor location (Silph Co. -> 1F..11F + lift)."""
    prefixes = LOCATION_PREFIXES.get(region)
    if not prefixes:
        return []
    out = []
    for name, mod in map_rows:
        for cand in (name, mod):
            if not cand:
                continue
            if any(cand.lower().startswith(p.lower()) for p in prefixes):
                if not any(x in cand.lower() for x in LOCATION_EXCLUDE) and name not in out:
                    out.append(name)
                break
    return out


def script_file_to_map(fname, valid):
    """'scripts/OaksLab.asm' -> 'Oaks Lab' if that is a real map name."""
    base = os.path.basename(fname)
    if not base.endswith(".asm"):
        return None
    stem = base[:-4]
    # CamelCase -> spaced: OaksLab -> 'Oaks Lab', RocketHideoutB1F -> 'Rocket Hideout B1F'.
    camel = re.sub(r"(?<=[a-z])(?=[A-Z])", " ", stem)
    # Only SOME maps want a space before digits (Route22 -> 'Route 22'); it MUST be tried
    # after `camel`, because it mangles floor suffixes ('B1F' -> 'B 1F') -- which is why
    # EVENT_ENTERED_ROCKET_HIDEOUT lost its map until 2026-07-16.
    digits = re.sub(r"(?<=[A-Za-z])(?=\d)", " ", camel)
    for cand in (camel, digits, stem):
        if cand in valid:
            return cand
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--check", action="store_true")
    a = ap.parse_args()

    valid = valid_map_names()
    _m = load(MAPS_JSON)
    _items = _m if isinstance(_m, list) else (_m.get("maps") or list(_m.values())[0])
    map_rows = [(x.get("name"), x.get("modernName")) for x in _items
                if isinstance(x, dict) and x.get("name")]
    canon = load(CANON)
    dossiers = {d["index"]: d for d in load(DOSSIERS)}
    old = load(EVENTS_JSON)
    old_by_ind = {e["ind"]: e for e in old}

    rows, stats = [], {"named": 0, "placeholder": 0, "maps_kept": 0,
                       "maps_derived": 0, "maps_dropped": 0, "no_map": 0,
                       "maps_from_region": 0, "maps_from_location": 0, "general": 0}

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
        # REGION FALLBACK (leadership 2026-07-16: "if anything can be mapped to a
        # region do that, prefer to not use General when possible"). Every flag has
        # a region (pret's allocation block) -- and 37 of the 46 regions ARE real map
        # names, so file those on that map's page: putting the region into maps[]
        # makes EventDBEntry::deepLink() -> MapDBEntry::toEvents carry them for free.
        # The other 9 are multi-floor LOCATIONS (Silph Co., Mt. Moon, S.S. Anne...) --
        # still real places, so they get a location page off `region`, never General.
        region = d.get("map") or ""
        if not maps and region in valid:
            maps.append(region)
            stats["maps_from_region"] += 1
        if not maps and region:
            # A multi-floor LOCATION: assign the flag to EVERY map in it and mark it
            # shared (leadership 2026-07-16) -- "if something spans multiple maps I
            # like to keep everything on its own map, so introduce shared groups
            # between maps... it's labelled as such and lets you select the other
            # maps it's in." No merged location page.
            members = location_members(region, map_rows)
            if members:
                maps.extend(members)
                stats["maps_from_location"] += 1
        if not maps:
            # GENERAL: retained (leadership: "more stuff I have to add in"), though
            # today nothing lands here -- every flag has a region that resolves.
            stats["general"] += 1

        row = {
            "name": d.get("name") or (pret or f"Placeholder Flag #{ind:03X}"),
            "ind": ind,
            "byte": byte,
            "bit": bit,
            "maps": maps,
            # --- additive: the research payload -----------------------------
            # `region` is pret's allocation block and EVERY flag has one (0 without),
            # so there is no General bucket at all. When it names a real map it is
            # also in maps[] above; when it is a multi-floor location it is the
            # location page's key.
            "region": region or None,
            # SHARED: this flag lives on more than one map, so it renders on EACH of
            # their pages inside a labelled shared group that lists/selects the others.
            "shared": len(maps) > 1,
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
    shared = sum(1 for r in rows if r["shared"])
    homeless = sum(1 for r in rows if not r["maps"])
    print(f"maps: kept={stats['maps_kept']} derived={stats['maps_derived']} "
          f"from-region={stats['maps_from_region']} "
          f"from-location(shared)={stats['maps_from_location']} "
          f"DROPPED-as-invalid={stats['maps_dropped']}")
    print(f"SHARED across >1 map: {shared}    on NO map (General): {homeless}")
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
