#!/usr/bin/env python3
"""Enrich the map-script + missable data with the metadata the Map Storage panel
needs — titles, progression descriptions, unused/known-issue marks, and the
missable <-> event-flag conflict links. ADDITIVE ONLY, self-validating.

What it writes (and nothing else):

  projects/db/assets/data/scripts.json
      += "desc"   one-liner: what this map's script progression tracks
      += "steps"  how many named steps the map's script table has (the legal
                  range for the dropdown; beyond it = the jp-hl gun, see below)

  projects/db/assets/data/maps.json  (scriptEntries only)
      += per-step "desc"  a plain-English progression description (curated for
                          the story maps, honestly derived for the rest)

  projects/db/assets/data/missables.json
      += "toggleConst"    the pret TOGGLE_* constant (constants/toggle_constants.asm)
      += "scriptToggled"  false for pret's X-marked constants: never toggled by
                          any map script (items / static encounters that
                          deactivate through wToggleableObjectList detection) —
                          the flag still controls visibility either way
      += "desc"           what the object is, where, and what hiding it means
      += "linkedEvents"   event flags the game checks before toggling this
                          object (from the verified script cross-reference) —
                          the CONFLICT hooks: flag says done + object still
                          shown (or vice versa) is a suspicious combination

Sources: pret/pokered (toggle_constants.asm — the ONLY authority for the X
marks), tmp/event-flags/flag_locations.json (the 2026-07-15 flag<->object
extraction), events.json (the 508 modelled story flags), maps.json sprites.

⚠️ Script values BEYOND a map's step table are the same loaded gun as the
event-flag crash mechanism: steps dispatch through a script-pointer table
ending in `jp hl` with no bounds check — an out-of-range step reads past the
table and jumps to garbage. The dropdown's custom-value path must warn (never
refuse). That's UI knowledge, but the "steps" count written here is its basis.

Usage:  python scripts/import_storage_meta.py [--check]
        --check verifies the enrichment is present + consistent, writes nothing.
"""
from __future__ import annotations
import argparse
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
DATA = REPO / "projects" / "db" / "assets" / "data"
PRET = Path(r"C:\Users\juneh\Documents\projects\pokered")
FLAG_LOC = REPO / "tmp" / "event-flags" / "flag_locations.json"


# ── toggle_constants.asm → ordered (name, script_toggled) ───────────────────────

def parse_toggles() -> list[tuple[str, bool, str]]:
    """(name, script_toggled, oddity). ` X` = never toggled by any script;
    ` XXX <note>` = pret's own oddballs ('sprite doesn't exist',
    'never (de)activated?') — the known-issue flags the UI must show."""
    text = (PRET / "constants" / "toggle_constants.asm").read_text(encoding="utf-8")
    out = []
    for m in re.finditer(
            r"^\tconst (TOGGLE_\w+)\s*;\s*[0-9A-F]+(?:\s+(X)(?:XX\s*(.*))?)?\s*$",
            text, re.M):
        name, xmark, note = m.group(1), m.group(2), (m.group(3) or "").strip()
        out.append((name, xmark is None, note))
    if len(out) != 228:
        raise SystemExit(f"toggle_constants.asm parse found {len(out)} != 228")
    return out


# ── curated step descriptions (mapName, stepName) → text ────────────────────────
# The story maps whose steps are real progression beats. Everything else gets an
# honest derived description. Kept small on purpose: real words where they carry
# real meaning, no invented lore.

CURATED_STEPS = {
    # ── Pallet Town: the very first scene ──
    ("Pallet Town", "DEFAULT"): "At rest — Oak's intro is done (or not yet triggered).",
    ("Pallet Town", "OAK_HEY_WAIT"): "You just stepped toward the grass — Oak shouts \"Hey! Wait!\".",
    ("Pallet Town", "OAK_WALKS_TO_PLAYER"): "Oak is walking over to stop you.",
    ("Pallet Town", "OAK_NOT_SAFE_COME_WITH_ME"): "Oak explains the tall grass is unsafe and turns for the lab.",
    ("Pallet Town", "PLAYER_FOLLOWS_OAK"): "You're being escorted to the lab behind Oak.",
    ("Pallet Town", "DAISY"): "The Daisy interlude step of the intro sequence.",
    # ── Oak's Lab: the whole opening, step by step ──
    ("Oaks Lab", "DEFAULT"): "At rest — the lab runs no scene right now.",
    ("Oaks Lab", "OAK_ENTERS_LAB"): "Oak is walking into the lab ahead of you (the intro escort).",
    ("Oaks Lab", "TOGGLE_OAKS"): "Housekeeping beat: the outdoor Oak is swapped for the lab Oak.",
    ("Oaks Lab", "PLAYER_ENTERS_LAB"): "You're walking up the lab to the back row.",
    ("Oaks Lab", "FOLLOWED_OAK"): "You've arrived at the back of the lab; the rival complains about waiting.",
    ("Oaks Lab", "OAK_CHOOSE_MON_SPEECH"): "Oak is giving the pick-a-Pokémon speech — the three balls are live.",
    ("Oaks Lab", "PLAYER_DONT_GO_AWAY_SCRIPT"): "Guard beat: you tried to leave before choosing — Oak calls you back.",
    ("Oaks Lab", "PLAYER_FORCED_TO_WALK_BACK_SCRIPT"): "You're being walked back to the starter table.",
    ("Oaks Lab", "CHOSE_STARTER_SCRIPT"): "You've picked your starter; the choice is being confirmed.",
    ("Oaks Lab", "RIVAL_CHOOSES_STARTER"): "The rival grabs the counter-pick starter.",
    ("Oaks Lab", "RIVAL_CHALLENGES_PLAYER"): "The rival spins around and challenges you.",
    ("Oaks Lab", "RIVAL_START_BATTLE"): "The very first rival battle is starting.",
    ("Oaks Lab", "RIVAL_END_BATTLE"): "The first battle just ended — win-or-lose dialogue runs.",
    ("Oaks Lab", "RIVAL_STARTS_EXIT"): "The rival begins to storm out.",
    ("Oaks Lab", "PLAYER_WATCH_RIVAL_EXIT"): "You watch the rival leave; the scene winds down.",
    ("Oaks Lab", "RIVAL_ARRIVES_AT_OAKS_REQUEST"): "Later visit: Oak has called you both back — the rival arrives.",
    ("Oaks Lab", "OAK_GIVES_POKEDEX"): "Oak hands over the Pokédex and the errand speech.",
    ("Oaks Lab", "RIVAL_LEAVES_WITH_POKEDEX"): "The rival takes his Pokédex and leaves — the lab story is done.",
    # ── Route 22: the two rival ambushes ──
    ("Route 22", "DEFAULT"): "At rest — no ambush armed.",
    ("Route 22", "RIVAL1_START_BATTLE"): "The FIRST rival ambush (early game) is armed — crossing the trigger starts it.",
    ("Route 22", "RIVAL1_AFTER_BATTLE"): "First ambush fought — the after-battle talk plays.",
    ("Route 22", "RIVAL1_EXIT"): "The rival walks off after the first ambush.",
    ("Route 22", "RIVAL2_START_BATTLE"): "The SECOND rival ambush (before Victory Road) is armed.",
    ("Route 22", "RIVAL2_AFTER_BATTLE"): "Second ambush fought — the send-off dialogue runs.",
    ("Route 22", "RIVAL2_EXIT"): "The rival leaves for good; the route goes quiet.",
    # ── Cerulean: the Nugget Bridge rival ──
    ("Cerulean City", "DEFAULT"): "At rest.",
    ("Cerulean City", "RIVAL_BATTLE"): "The rival blocks the way north — the bridge battle is armed.",
    ("Cerulean City", "RIVAL_DEFEATED"): "Rival beaten — his goodbye dialogue plays.",
    ("Cerulean City", "RIVAL_CLEANUP"): "The rival walks off; the city resets to normal.",
    ("Cerulean City", "ROCKET_DEFEATED"): "The dig-house Rocket has been dealt with.",
    # ── Vermilion: boarding the S.S. Anne ──
    ("Vermilion City", "DEFAULT"): "At rest — the dock guard checks for the S.S. Ticket.",
    ("Vermilion City", "PLAYER_MOVING_UP1"): "Dock scene: you're being stepped up the pier.",
    ("Vermilion City", "PLAYER_EXIT_SHIP"): "The ship has sailed — you're being walked off the dock.",
    ("Vermilion City", "PLAYER_MOVING_UP2"): "Dock scene: the second forced step.",
    ("Vermilion City", "PLAYER_ALLOWED_TO_PASS"): "The guard has taken your ticket — the dock is open.",
    # ── Pokémon Tower 2F: the rival in the dark ──
    ("Pokemon Tower 2", "DEFAULT"): "At rest — the rival waits by the stairs (battle armed on approach).",
    ("Pokemon Tower 2", "DEFEATED_RIVAL"): "Tower rival beaten — his parting words play.",
    ("Pokemon Tower 2", "RIVAL_EXITS"): "The rival leaves the Tower.",
    # ── Route 23 / Victory Road approach ──
    ("Route 23", "DEFAULT"): "At rest — the badge-check guards are on duty.",
    ("Route 23", "PLAYER_MOVING"): "A guard scene is stepping you through a checkpoint.",
    ("Route 23", "RESET_TO_DEFAULT"): "Checkpoint scene finished — resetting to rest.",
    # ── The Championship rooms ──
    ("Lances Room", "DEFAULT"): "At rest — Lance waits.",
    ("Lances Room", "LANCE_START_BATTLE"): "Lance's battle is armed.",
    ("Lances Room", "LANCE_END_BATTLE"): "Lance is beaten — his concession speech runs.",
    ("Lances Room", "PLAYER_IS_MOVING"): "The room walks you toward the Champion's door.",
    ("Champions Room", "DEFAULT"): "At rest.",
    ("Champions Room", "PLAYER_ENTERS"): "You're walking in for the final battle.",
    ("Champions Room", "RIVAL_READY_TO_BATTLE"): "The Champion — your rival — issues the final challenge.",
    ("Champions Room", "RIVAL_DEFEATED"): "The Champion falls — his disbelief speech plays.",
    ("Champions Room", "OAK_ARRIVES"): "Oak walks in on the scene.",
    ("Champions Room", "OAK_CONGRATULATES_PLAYER"): "Oak congratulates you.",
    ("Champions Room", "OAK_DISAPPOINTED_WITH_RIVAL"): "Oak scolds the rival about trust and love.",
    ("Champions Room", "OAK_COME_WITH_ME"): "Oak beckons you to the Hall of Fame.",
    ("Champions Room", "OAK_EXITS"): "Oak leads the way out.",
    ("Champions Room", "PLAYER_FOLLOWS_OAK"): "You follow Oak to the Hall of Fame.",
    ("Champions Room", "CLEANUP_SCRIPT"): "The scene tears down — the game is won.",
    ("Hall of Fame", "DEFAULT"): "The induction — your team is being recorded for the Hall of Fame.",
}

# Label-pattern fallbacks, tried in order. Gym pattern first: START/END/POST is
# the standard leader-battle progression on every gym map.
DERIVED = [
    (re.compile(r"^DEFAULT$"), "At rest — nothing staged on this map right now."),
    (re.compile(r"NOOP|END$"), "The script has finished — nothing more runs here."),
    (re.compile(r"^START_BATTLE$"), "The leader/boss battle on this map is armed — talking to them starts it."),
    (re.compile(r"^END_BATTLE$"), "The leader/boss battle just ended — the badge/aftermath dialogue runs."),
    (re.compile(r"POST_BATTLE"), "The after-victory scene (TM handout / parting words) plays."),
    (re.compile(r"START_BATTLE"), "A scripted battle is armed — crossing its trigger starts it."),
    (re.compile(r"AFTER_BATTLE|DEFEATED"), "The scripted battle just ended — the aftermath dialogue runs."),
    (re.compile(r"EXIT"), "A scripted character is walking off the map."),
    (re.compile(r"PLAYER.*(MOVING|MOVES|WALK)|MOVING.*PLAYER"), "A cutscene is stepping the player around this map."),
    (re.compile(r"ENTER|SCENE"), "A cutscene is in progress on this map."),
    (re.compile(r"BATTLE"), "A scripted battle stage of this map's story."),
]


def step_desc(map_name: str, step: dict, i: int, n: int) -> str:
    cur = CURATED_STEPS.get((map_name, step.get("name", "")))
    if cur:
        return cur
    for rx, text in DERIVED:
        if rx.search(step.get("name", "")):
            return text + f" (step {i} of 0–{n - 1})"
    return (f"Stage {i} of this map's scripted events (0–{n - 1}). The game walks "
            f"these steps in order as the map's story plays out.")


# ── missable descriptions ────────────────────────────────────────────────────────

def missable_desc(entry: dict, obj: dict | None, script_toggled: bool) -> str:
    kind = (obj or {}).get("kind", "")
    sprite = (obj or {}).get("sprite", "")
    where = entry.get("map", "?")
    if sprite == "SPRITE_POKE_BALL":
        base = f"An item ball on {where}. Hidden = already picked up."
    elif kind == "trainer" and sprite.startswith("SPRITE_") and any(
            s in sprite for s in ("MONSTER", "SNORLAX", "VOLTORB", "ZAPDOS",
                                  "MOLTRES", "ARTICUNO", "MEWTWO")):
        base = f"A static Pokémon encounter on {where}. Hidden = already battled."
    elif kind == "trainer":
        base = f"{entry.get('name', 'A trainer')} on {where}. Hidden = gone from the map."
    else:
        base = f"{entry.get('name', 'A character')} on {where}. Hidden = not on the map."
    if not script_toggled:
        base += (" No map script ever toggles this one — the game flips it through "
                 "the object list itself (pret marks it unused by scripts).")
    return base


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--check", action="store_true")
    a = ap.parse_args()

    scripts_p = DATA / "scripts.json"
    maps_p = DATA / "maps.json"
    miss_p = DATA / "missables.json"
    scripts = json.loads(scripts_p.read_text(encoding="utf-8"))
    maps = json.loads(maps_p.read_text(encoding="utf-8"))
    miss = json.loads(miss_p.read_text(encoding="utf-8"))
    by_name = {m["name"]: m for m in maps}

    toggles = parse_toggles()
    flag_loc = json.loads(FLAG_LOC.read_text(encoding="utf-8")) if FLAG_LOC.exists() else {}
    # toggle const -> the extracted object record (linked flags etc.)
    tog_obj: dict[str, dict] = {}
    for _map, rec in flag_loc.items():
        for o in rec.get("objects", []):
            t = o.get("toggle")
            if t:
                tog_obj[t] = o

    # ── scripts.json: desc + steps ──────────────────────────────────────────
    for e in scripts:
        owned = e.get("maps") or [e["name"]]
        steps = 0
        for mn in owned:
            m = by_name.get(mn)
            if m and m.get("scriptEntries"):
                steps = max(steps, len(m["scriptEntries"]))
        e["steps"] = steps
        e["desc"] = (f"How far {e['name']}'s scripted events have progressed. "
                     + (f"{steps} named steps." if steps
                        else "No named steps are known for this script."))

    # ── maps.json scriptEntries: per-step desc ─────────────────────────────
    step_total = 0
    for m in maps:
        se = m.get("scriptEntries")
        if not se:
            continue
        n = len(se)
        for i, st in enumerate(se):
            st["desc"] = step_desc(m["name"], st, i, n)
            step_total += 1

    # ── missables.json: toggleConst / scriptToggled / desc / linkedEvents ──
    canon_p = REPO / "tmp" / "event-flags" / "event_flags_canonical.json"
    canon = ({r["name"]: r["index"] for r in
              json.loads(canon_p.read_text(encoding="utf-8")) if r.get("name")}
             if canon_p.exists() else {})
    linked_total = 0
    for e in miss:
        ind = e["ind"]
        tname, script_toggled, oddity = toggles[ind]
        e["toggleConst"] = tname
        e["scriptToggled"] = script_toggled
        if oddity:
            e["oddity"] = oddity          # pret's own note: a known-broken one
        obj = tog_obj.get(tname)
        e["desc"] = missable_desc(e, obj, script_toggled)
        if oddity:
            e["desc"] += f" ⚠ pret flags this one: {oddity} — toggling it does nothing visible."
        linked = []
        for fl in (obj or {}).get("flags", []):
            # Real EVENT_* names only — the extraction uses placeholder strings
            # (e.g. "<got-item event (map-specific)>") for item balls, whose hide
            # state is object-list-detected, not event-flag-driven. Noise here.
            if not re.fullmatch(r"EVENT_\w+", fl):
                continue
            linked.append({"flag": fl, "eventIndex": canon.get(fl, -1)})
            linked_total += 1
        # Always REPLACE (never merge): a stale key from an earlier run must not
        # survive a rule change — re-running the importer is the source of truth.
        e.pop("linkedEvents", None)
        if linked:
            e["linkedEvents"] = linked

    # ── validate ────────────────────────────────────────────────────────────
    assert len(scripts) == 97 and len(miss) == 228
    assert step_total == sum(len(m.get("scriptEntries") or []) for m in maps)
    assert all("desc" in e and "steps" in e for e in scripts)
    assert all("desc" in e and "toggleConst" in e for e in miss)
    unused = sum(1 for e in miss if not e["scriptToggled"])
    print(f"scripts: 97 (+desc/steps) · steps described: {step_total} · "
          f"missables: 228 (+meta; {unused} never script-toggled; "
          f"{linked_total} event links)")

    if a.check:
        print("--check: consistent (nothing written)")
        return 0
    # indent=2, same as import_map_scripts.py — keeps the diff additive-only.
    scripts_p.write_text(json.dumps(scripts, ensure_ascii=False, indent=2) + "\n",
                         encoding="utf-8")
    maps_p.write_text(json.dumps(maps, ensure_ascii=False, indent=2) + "\n",
                      encoding="utf-8")
    miss_p.write_text(json.dumps(miss, ensure_ascii=False, indent=2) + "\n",
                      encoding="utf-8")
    print("written (additive only)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
