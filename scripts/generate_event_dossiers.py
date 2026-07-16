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

# ---------------------------------------------------------------------------
# CURATED OVERRIDES — hand-authored research, keyed by pret EVENT_ name (or, for
# unnamed gaps, by index hex like "#1CC"). This is the layer where deep per-flag
# research lands: a real name + description that supersedes the auto-derived text.
# pret marks some used events "; ???"; where their usage discloses meaning we
# name them here instead of leaving pret's hex placeholder.
# Extend this as authoring proceeds (Phase 5); every entry is source-grounded.
# ---------------------------------------------------------------------------
CURATED = {
    "EVENT_1B8": {
        "name": "Celadon unused reset flag (1B8)",
        "description": "Cleared at the top of CeladonCity_Script every time you "
        "enter Celadon City (ResetEvents EVENT_1B8, EVENT_1BF), but the shipped "
        "game never sets or checks it anywhere. A vestigial / dead flag — pret "
        "marks it '; ???'. Harmless dev leftover.",
        "classification_add": ["vestigial", "temporary"],
    },
    "EVENT_1BF": {
        "name": "Celadon unused reset flag (1BF)",
        "description": "Cleared with EVENT_1B8 at the top of CeladonCity_Script on "
        "every Celadon entry, but never set or checked anywhere. Vestigial / dead "
        "flag (pret '; ???').",
        "classification_add": ["vestigial", "temporary"],
    },
    "EVENT_67F": {
        "name": "Celadon unused reset flag (67F)",
        "description": "ResetEvent'd at the top of CeladonCity_Script on every "
        "Celadon entry, but never set or checked. Vestigial / dead flag (pret "
        "'; ???'); note it lives in the Rocket Hideout allocation block yet is "
        "reset by Celadon's script.",
        "classification_add": ["vestigial", "temporary"],
    },
    "EVENT_2A7": {
        "name": "On Cinnabar Gym map (gate-tile refresh)",
        "description": "Set by the overworld loop (home/overworld.asm) while the "
        "current map is the Cinnabar Gym, and cleared by CinnabarGym_Script. A "
        "transient marker that drives UpdateCinnabarGymGateTileBlocks (the "
        "quiz-gate tiles). pret marks it '; ???'; its usage discloses the meaning. "
        "Temporary — not durable story state.",
        "classification_add": ["temporary"],
    },
}

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


def neighbors_phrase(cx):
    a = f"'{cx['prev'][1]}'" if cx["prev"] else "the start of the block"
    b = f"'{cx['next'][1]}'" if cx["next"] else "the end of the block"
    return f"It sits between {a} and {b} in flag order"


PROGRESSION_RE = re.compile(
    r"EVENT_GOT_(POKEDEX|OAKS_PARCEL|HM0\d|MASTER_BALL|BIKE_VOUCHER|TOWN_MAP|"
    r"ITEMFINDER|SILPH_SCOPE|CARD_KEY|LIFT_KEY|SS_TICKET|GOLD_TEETH|SECRET_KEY|"
    r"POKE_FLUTE|COIN_CASE)")


def caution(r, cls):
    """Per-flag caution note (Phase 4). Bulk 'all on' crash is a panel-level
    warning, not per-flag."""
    name = r.get("name") or ""
    if PROGRESSION_RE.match(name):
        return ("progression: clearing this after the item/gate was used can strand "
                "progress (may be un-re-obtainable).")
    if "block-swept" in cls:
        return ("part of a range set/clear sequence (e.g. Elite-Four reset) — "
                "editing one bit mid-sequence can desync it.")
    if "temporary" in cls or "vestigial" in cls:
        return "temporary/scratch — the game rewrites it during play, so an edit may not stick."
    return None


def describe(r, fname, mapname, cls, cx):
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
    nb = neighbors_phrase(cx)
    if r["block_swept"]:
        rg = (r.get("range_groups") or ["a block operation"])[0]
        return (f"Unnamed bit in the {mapname} event block that has no individual "
                f"name, but is swept as a member of the range group \"{rg}\": a "
                f"Set/ResetEventRange byte-fills the whole span, toggling this bit "
                f"together with the group. {nb}. Save {loc}.")
    return (f"Placeholder flag in the {mapname} event block — {cx['role']}. "
            f"{nb}. It has no reference anywhere in the disassembly (verified by "
            f"name, by range macro, and by raw index); the game rounds each map's "
            f"event block up to whole bytes and leaves headroom, so this bit is "
            f"unallocated space. Named 'Placeholder Flag #{idx[2:].upper()}' and "
            f"filed in this map's Placeholder Flags group. Save {loc}.")


def build_context(usage):
    """Per-bit research context: named neighbors + role within the map block."""
    import bisect
    named = [(r["index"], friendly(r["name"])) for r in usage if r["name"]]
    named_idx = [i for i, _ in named]
    # highest 'live' (named or block-swept) index within each map region
    region_max = {}
    for r in usage:
        if r["name"] or r["block_swept"]:
            sec = r["section"]
            region_max[sec] = max(region_max.get(sec, -1), r["index"])
    ctx = {}
    for r in usage:
        i = r["index"]
        p = bisect.bisect_left(named_idx, i) - 1
        n = bisect.bisect_right(named_idx, i)
        prev_n = named[p] if p >= 0 else None
        next_n = named[n] if n < len(named) else None
        rmax = region_max.get(r["section"], -1)
        role = "trailing reserve (byte-aligned block padding)" if i > rmax \
            else "interior reserved bit (between used flags)"
        ctx[i] = {"prev": prev_n, "next": next_n, "role": role}
    return ctx


def main() -> int:
    with open(USAGE, encoding="utf-8") as fh:
        usage = json.load(fh)
    ctx = build_context(usage)

    dossiers = []
    placeholders = []
    counts = {"named": 0, "block_swept_gap": 0, "placeholder": 0}

    for r in usage:
        mapname = region_to_map(r["section"])
        cls = classify(r)
        placeholder = False
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
            # No reference ANYWHERE (name, range, raw) -> Placeholder Flag
            # (project leadership 2026-07-15): named, tied to its map, in that
            # map's own "Placeholder Flags" group at the bottom.
            fname = f"Placeholder Flag #{r['index_hex'][2:].upper()}"
            unknown = False
            placeholder = True
            counts["placeholder"] += 1

        # A range sweep IS a flag group (project leadership): prefer it, since it
        # is evidence-based (these bits are literally toggled together).
        rgroups = r.get("range_groups", [])
        group = rgroups[0] if rgroups else group_of(r["name"], mapname)
        if placeholder:
            group = f"{mapname} — Placeholder Flags"
            cls = ["placeholder"]

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
            "placeholder": placeholder,
            "unknown_candidate": unknown,
            "name_provisional": unknown,   # unused; kept for schema stability
            "prev_named": ctx[r["index"]]["prev"][1] if ctx[r["index"]]["prev"] else None,
            "next_named": ctx[r["index"]]["next"][1] if ctx[r["index"]]["next"] else None,
            "block_role": ctx[r["index"]]["role"],
            "crash": None,                 # no single-flag crash gun (Phase 4)
            "caution": caution(r, cls),    # softlock/progression note, else null
            "description": describe(r, fname, mapname, cls, ctx[r["index"]]),
            "evidence": {
                "used": r["used"], "n_set": r["n_set"], "n_check": r["n_check"],
                "n_reset": r["n_reset"], "temporary": r["temporary"],
                "block_swept": r["block_swept"], "multi_map": r["multi_map"],
                "files": r["files"],
            },
            "curated": False,
        }
        # apply hand-authored research override (by pret name or #hex)
        key = r["name"] or f"#{r['index_hex'][2:].upper()}"
        ov = CURATED.get(key)
        if ov:
            if "name" in ov:
                entry["name"] = ov["name"]
                entry["name_provisional"] = False
                entry["unknown_candidate"] = False
            if "description" in ov:
                entry["description"] = ov["description"]
            for c in ov.get("classification_add", []):
                if c not in entry["classification"]:
                    entry["classification"].append(c)
            entry["curated"] = True
        dossiers.append(entry)
        if entry["placeholder"]:
            placeholders.append({
                "index_hex": r["index_hex"], "name": entry["name"],
                "map": mapname, "group": group,
                "byte_file": r["byte_file"], "bit": r["bit"],
            })

    with open(os.path.join(OUT_DIR, "events_dossiers.json"), "w", encoding="utf-8") as fh:
        json.dump(dossiers, fh, indent=1)
    with open(os.path.join(OUT_DIR, "placeholder_flags.json"), "w", encoding="utf-8") as fh:
        json.dump(placeholders, fh, indent=1)

    # group summary
    groups = {}
    for e in dossiers:
        if e["group"]:
            groups[e["group"]] = groups.get(e["group"], 0) + 1

    curated = sum(1 for e in dossiers if e["curated"])
    print(f"dossiers written: {len(dossiers)} / 2560")
    print(f"  named (pret):            {counts['named']}")
    print(f"  block-swept (range grp): {counts['block_swept_gap']}")
    print(f"  Placeholder Flags:       {counts['placeholder']}  (no code presence at all)")
    print(f"  hand-curated overrides:  {curated}")
    print(f"flag-groups derived: {len(groups)}")
    top = sorted(groups.items(), key=lambda kv: -kv[1])[:12]
    for g, n in top:
        print(f"    {g:42} {n}")
    print(f"out -> {OUT_DIR}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
