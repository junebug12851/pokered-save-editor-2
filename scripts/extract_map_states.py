#!/usr/bin/env python3
"""
extract_map_states.py -- per-map PROGRESSION BLUEPRINTS (the "map states"), from pret/pokered.

The brief (Fairy Fox, 2026-07-17): each map has one or more default states -- where the NPCs
stand, which filter flags are on/off, which event flags are set, what the map's script byte
holds -- and the map's script largely determines its PROGRESS. This builds, per map, a JSON
blueprint of every progression state so the app can:

  * roll a map backward/forward one stage at a time,
  * construct a proper destination map when the player changes maps (pick a state from the
    list, write its save-file facts, land on the entry spot -- the first warp),
  * feed the randomizer legal whole-states instead of raw bytes.

── The model (the research finding this encodes) ─────────────────────────────────────────────

A map's SCRIPT_<MAP>_* values are NOT all states of the world. They split into:

  * RESTING values -- what the byte sits at between play sessions (DEFAULT, NOOP, the
    post-cutscene watchers). A save is normally found in one of these.
  * TRANSIENT values -- mid-cutscene steps the engine passes through frame-by-frame (Oak
    walking to the player). Real, storable, never refused -- but not what you roll to.

And a STAGE (a progression state a person rolls through) is MORE than the script byte: it is
script byte + the event flags the story has set + missable visibility + (sometimes) badges.
Two stages can share a script value (every gym rests at 0 both before and after its leader).
So the blueprint's `states` are stages; the raw byte's own table ships as `scriptValues`.

Save-file-only doctrine: effects are expressed ONLY as save-file facts (script byte, event
flag inds, missable inds, badge bits). ROM/WRAM scratch is out of scope on purpose.

── How it works ──────────────────────────────────────────────────────────────────────────────

Parses pret's scripts/<Map>.asm through import_map_storage_spots' proven ScriptFile parser
(their macro names are the grammar), then adds what stages need:

  * register-`a` line simulation per routine -> every `ld [w*CurScript], a` edge with its
    VALUE (catches `xor a` and numeric forms the const-only regex misses),
  * FALLTHROUGH closure -- a routine that doesn't end in ret/jp/jr keeps executing into the
    next label (PewterGymBrockPostBattle's world-writes all live in the ReceiveTM34 body it
    falls into; without this the gym "does nothing"),
  * badge writes (`ld hl, wObtainedBadges` + `set BIT_xBADGE, [hl]`), trainer-header events,
  * guard classification: story guards (CheckEvent / coord tests / talk gating) mean a value
    RESTS there; pure animation waits (scripted-NPC movement, walk counters, battle-end
    polls) mean it is TRANSIENT.

The hand-researched story layer lives in scripts/data/map_states_curated.json and is merged
over the derived skeleton; every name it uses must resolve against the shipped vocabularies
(events.json pretName, missables.json toggleConst, the map's own script table) or generation
FAILS -- curation cannot invent a flag.

Source:  <pokered>/scripts/<Map>.asm, constants/{map,event}_constants.asm
Reads:   projects/db/assets/data/{maps,events,missables,scripts}.json
         scripts/data/map_states_curated.json      (the story layer)
Output:  projects/db/assets/data/map-states/<Map>.json  (+ _index.json)

Usage:
    python scripts/extract_map_states.py [--pokered <path>] [--check]
    --check   parse + verify + diff against the committed files, write nothing. Exit 1 on drift.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

for _s in (sys.stdout, sys.stderr):
    try:
        _s.reconfigure(encoding="utf-8")
    except AttributeError:
        pass

SCRIPTS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPTS_DIR))
import import_map_storage_spots as base  # noqa: E402  (the proven parser is the foundation)

REPO = SCRIPTS_DIR.parent
DEFAULT_POKERED = Path.home() / "Documents" / "projects" / "pokered"
DATA = REPO / "projects" / "db" / "assets" / "data"
OUT_DIR = DATA / "map-states"
CURATED = SCRIPTS_DIR / "data" / "map_states_curated.json"

# Shared trainer-engine routines a map's pointer table may name without defining -- their
# bodies live in the engine, so classification is knowledge, not parse: DEFAULT's watcher
# rests; the battle pair is pure machinery that hands back to 0 when the fight resolves.
ENGINE_ROUTINES = {
    "CheckFightingMapTrainers": ("resting", []),
    "DisplayEnemyTrainerTextAndStartBattle": ("transient", [0]),
    "EndTrainerBattle": ("transient", [0]),
}

# scripts.json names that own a GROUP of maps (one save byte, several maps).
SLOT_GROUPS = {
    "Route 16 Gate": ["Route 16 Gate 1F", "Route 16 Gate 2F"],
    "Route 18 Gate": ["Route 18 Gate 1F", "Route 18 Gate 2F"],
    "Power Plant / Route 7 Gate": ["Power Plant", "Route 7 Gate"],
}

BADGE_HL_RE = re.compile(r"^\s*ld\s+hl\s*,\s*(wObtainedBadges|wBeatGymFlags)\b")
BADGE_SET_RE = re.compile(r"^\s*set\s+BIT_([A-Z]+BADGE)\s*,\s*\[hl\]")
TRAINER_HDR_RE = re.compile(r"^\s*trainer\s+(EVENT_[A-Z0-9_]+)", re.MULTILINE)
LD_A_RE = re.compile(r"^\s*ld\s+a\s*,\s*([^;\[\n]+?)\s*(?:;.*)?$")
XOR_A_RE = re.compile(r"^\s*xor\s+a\b")
STORE_CUR_RE = re.compile(r"^\s*ld\s+\[(w\w*CurScript)\]\s*,\s*a")
NUM_RE = re.compile(r"^\$?([0-9A-Fa-f]+)$")
# Unconditional control transfer = the body does NOT fall through past this line.
UNCOND_END_RE = re.compile(
    r"^\s*(ret\b(?!\s*[cznv])|reti\b|jp\s+(?!c\b|nc\b|z\b|nz\b)\S+$|jr\s+(?!c\b|nc\b|z\b|nz\b)\S+$"
    r"|predef_jump\b|jp\s+hl)"
)
STORY_GUARD_RE = re.compile(
    r"\bCheck(?:Both)?Events?(?:Set)?\w*\s|\bArePlayerCoordsInArray\b"
    r"|ld\s+a\s*,\s*\[w[XY]Coord\]|ldh?\s+a\s*,\s*\[hJoy(?:Held|Pressed)\]"
)
WAIT_GUARD_RE = re.compile(
    r"wSimulatedJoypadStatesIndex|wNPCMovementScriptPointerTableNum|BIT_SCRIPTED_NPC_MOVEMENT"
    r"|wWalkCounter|wIsInBattle|wMovingSpriteData|wJoyIgnore"
)
BADGE_BITS = {"BOULDERBADGE", "CASCADEBADGE", "THUNDERBADGE", "RAINBOWBADGE",
              "SOULBADGE", "MARSHBADGE", "VOLCANOBADGE", "EARTHBADGE"}


def read(p: Path) -> str:
    return p.read_text(encoding="utf-8")


# ──────────────────────────────────────────────────────────────────────────────────────────────
class StateFile(base.ScriptFile):
    """base.ScriptFile + the stage-level analysis (edges with values, fallthrough, badges)."""

    def __init__(self, text: str, consts: dict[str, int]):
        super().__init__(text, consts)
        # ordered global labels, for fallthrough
        self.order = [name for name, _ in base.split_routines(text)]
        self.at = {n: i for i, n in enumerate(self.order)}
        # SCRIPT const -> index (pointer-table position IS the byte value)
        self.const_val = {c: i for i, c in enumerate(self.ptrs)}
        self.val_const = {i: c for c, i in self.const_val.items()}
        self.val_routine = {i: r for i, (c, r) in enumerate(self.ptrs.items())}
        self.trainer_events = TRAINER_HDR_RE.findall(text)
        # SETTER helpers: routines that store the caller's register-a into a CurScript var
        # before loading a themselves (`ld a, SCRIPT_X / jp SilphCo7FSetCurScript` is a real
        # idiom -- without this, whole maps' edges vanish).
        self.setters: set[str] = set()
        for name, body in self.routines.items():
            for ln in body.split("\n"):
                if XOR_A_RE.match(ln) or LD_A_RE.match(ln):
                    break
                if STORE_CUR_RE.match(ln):
                    self.setters.add(name)
                    break

    def falls_through(self, label: str) -> bool:
        body = self.routines.get(label, "")
        last = ""
        for ln in body.split("\n"):
            s = ln.split(";", 1)[0].rstrip()
            if s.strip():
                last = s
        if not last:
            return False
        return not UNCOND_END_RE.match(last)

    def flat_body(self, label: str) -> str:
        """The routine's body + every body it falls through into (cycle-guarded)."""
        out, cur, seen = [], label, set()
        while cur is not None and cur not in seen:
            seen.add(cur)
            out.append(self.routines.get(cur, ""))
            if not self.falls_through(cur):
                break
            idx = self.at.get(cur)
            cur = self.order[idx + 1] if idx is not None and idx + 1 < len(self.order) else None
        return "\n".join(out)

    def script_edges(self, body: str) -> list[int]:
        """Every VALUE this body stores into a *CurScript variable (register-a simulation)."""
        vals, a = [], None
        for ln in body.split("\n"):
            if XOR_A_RE.match(ln):
                a = 0
                continue
            m = LD_A_RE.match(ln)
            if m:
                arg = m.group(1).strip()
                if arg in self.const_val:
                    a = self.const_val[arg]
                else:
                    n = NUM_RE.match(arg)
                    if n:
                        try:
                            a = int(n.group(1), 16 if arg.startswith("$") else 10)
                        except ValueError:
                            a = None
                    else:
                        a = None
                continue
            if STORE_CUR_RE.match(ln) and a is not None:
                vals.append(a)
                continue
            m = re.match(r"^\s*(?:jp|call)\s+(?:c\s*,|nc\s*,|z\s*,|nz\s*,)?\s*(\w+)\s*$", ln)
            if m and m.group(1) in self.setters and a is not None:
                vals.append(a)
        return vals

    def badges(self, body: str) -> list[str]:
        out, armed = [], False
        for ln in body.split("\n"):
            if BADGE_HL_RE.match(ln):
                armed = True
                continue
            if armed:
                m = BADGE_SET_RE.match(ln)
                if m and m.group(1) in BADGE_BITS:
                    out.append(m.group(1))
                elif ln.strip() and not ln.strip().startswith(";") and not BADGE_SET_RE.match(ln):
                    armed = False
        return sorted(set(out))


# ──────────────────────────────────────────────────────────────────────────────────────────────
def classify(sf: StateFile, val: int) -> tuple[str, bool]:
    """-> (kind, autoRunsOnEntry). Value 0 is the init value every save starts at; if it
    advances unguarded (a cutscene plays the moment you walk in) it still must be listable,
    so it stays 'resting' and wears autoRunsOnEntry instead."""
    routine = sf.val_routine.get(val)
    if routine is None:
        return "resting", False
    if routine in ENGINE_ROUTINES:
        return ENGINE_ROUTINES[routine][0], False
    body = sf.flat_body(routine)
    edges = [v for v in sf.script_edges(body) if v != val]
    if not edges:
        return "resting", False
    if STORY_GUARD_RE.search(body):
        return "resting", False
    if val == 0:
        return "resting", True
    return "transient", False


def reach_order(sf: StateFile) -> list[int]:
    """BFS over next-script edges from 0 -- the order the story visits values."""
    seen, queue = [], [0]
    while queue:
        v = queue.pop(0)
        if v in seen or v not in sf.val_routine:
            continue
        seen.append(v)
        for nxt in sf.script_edges(sf.flat_body(sf.val_routine[v])):
            if nxt not in seen:
                queue.append(nxt)
    for v in sf.val_routine:            # unreachable table entries still exist
        if v not in seen:
            seen.append(v)
    return seen


def stage_missables(map_missables: list[dict], overrides: dict[str, str]) -> list[dict]:
    """ABSOLUTE visibility for every missable of the map: defVal + curated overrides."""
    out = []
    for m in map_missables:
        state = overrides.get(m["toggleConst"], "show" if m["defVal"] == "Show" else "hide")
        out.append({"ind": m["ind"], "name": m["name"], "toggleConst": m["toggleConst"],
                    "state": state})
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--pokered", type=Path, default=DEFAULT_POKERED)
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--digest", action="store_true",
                    help="also write tmp/map-states-digest.json (raw facts, for curation work)")
    args = ap.parse_args()

    sdir = args.pokered / "scripts"
    mconst = args.pokered / "constants" / "map_constants.asm"
    econst = args.pokered / "constants" / "event_constants.asm"
    if not sdir.is_dir() or not mconst.is_file() or not econst.is_file():
        print(f"FAIL: pokered clone not usable at {args.pokered}", file=sys.stderr)
        return 1

    event_consts = base.parse_event_constants(read(econst))
    map_consts = base.MAPCONST_RE.findall(read(mconst))
    file_to_id = {base.norm(c): i for i, c in enumerate(map_consts)}

    maps = json.loads(read(DATA / "maps.json"))
    by_ind = {m["ind"]: m for m in maps}
    events = json.loads(read(DATA / "events.json"))
    event_by_name = {e["pretName"]: e for e in events if e.get("pretName")}
    missables = json.loads(read(DATA / "missables.json"))
    miss_by_const = {m["toggleConst"]: m for m in missables if m.get("toggleConst")}
    miss_by_map: dict[str, list[dict]] = {}
    for m in missables:
        miss_by_map.setdefault(m["map"], []).append(m)
    scripts_layout = json.loads(read(DATA / "scripts.json"))
    slot_of_map: dict[str, int] = {}
    for s in scripts_layout:
        for mapname in SLOT_GROUPS.get(s["name"], [s["name"]]):
            slot_of_map[mapname] = s["ind"]

    curated_all = json.loads(read(CURATED)) if CURATED.exists() else {}

    problems: list[str] = []
    out_files: dict[str, dict] = {}
    digest: dict[str, dict] = {}
    stats = {"maps": 0, "stages": 0, "curatedMaps": 0, "transient": 0, "resting": 0}

    for path in sorted(sdir.glob("*.asm")):
        text = read(path)
        if "def_script_pointers" not in text:
            continue
        key = base.norm(path.name)
        map_id = file_to_id.get(key)
        if map_id is None or map_id not in by_ind:
            problems.append(f"{path.name}: no maps.json entry (key '{key}')")
            continue
        entry = by_ind[map_id]
        sf = StateFile(text, event_consts)
        if not sf.ptrs:
            continue
        for p in sorted(set(sf.problems)):
            problems.append(f"{path.name}: {p}")

        # ── raw script-value table (the dropdown's truth) ─────────────────────────────
        script_values = []
        per_val: dict[int, dict] = {}
        for val, routine in sf.val_routine.items():
            kind, auto_runs = classify(sf, val)
            body = sf.flat_body(routine)
            evs, tgs = sf.own_writes(body)
            nxt = (ENGINE_ROUTINES[routine][1] if routine in ENGINE_ROUTINES
                   else sorted({v for v in sf.script_edges(body) if v != val}))
            info = {
                "value": val,
                "name": sf.val_const.get(val, f"UNNAMED_{val}"),
                "routine": routine,
                "kind": kind,
                "next": nxt,
                "sets": sorted({n for a, n in evs if a == "SetEvent"}),
                "resets": sorted({n for a, n in evs if a == "ResetEvent"}),
                "shows": sorted({n for n, v in tgs if v == "Show"}),
                "hides": sorted({n for n, v in tgs if v == "Hide"}),
                "badges": sf.badges(body),
            }
            if auto_runs:
                info["autoRunsOnEntry"] = True
            per_val[val] = info
            stats[kind] += 1
            script_values.append({k: info[k] for k in
                                  ("value", "name", "routine", "kind", "next")}
                                 | ({"autoRunsOnEntry": True} if auto_runs else {}))

        order = reach_order(sf)
        resting_order = [v for v in order if per_val[v]["kind"] == "resting"]
        settle = resting_order[-1] if resting_order else 0

        # owned universe: every event this FILE touches -- script routines AND text handlers
        # (Bill's SS-ticket, Blue's Town Map and every gym TM live in text_asm bodies, not in
        # the pointer table's routines) -- plus the trainer headers.
        file_evs, file_tgs = sf.own_writes(text)
        owned: set[str] = set(sf.trainer_events)
        owned |= {n for _a, n in file_evs}
        owned = {n for n in owned if n in event_by_name}
        file_sets = {n for a, n in file_evs if a == "SetEvent"}
        file_shows = {n for n, v in file_tgs if v == "Show"}
        file_hides = {n for n, v in file_tgs if v == "Hide"}
        file_badges = sf.badges(text)
        for n in set(sf.trainer_events) - set(event_by_name):
            problems.append(f"{path.name}: unknown trainer event {n}")

        # missables.json's `map` strings mix legacy and modern spellings ("Mt Moon B2F" vs
        # maps.json's "Mt. Moon 3") -- accept either of this entry's names.
        own_names = {entry["name"], entry.get("modernName") or entry["name"]}
        map_missables = [m for nm in own_names for m in miss_by_map.get(nm, [])]
        curated = curated_all.get(path.stem)

        # ── stages ────────────────────────────────────────────────────────────────────
        def resolve_events(set_names: list[str], context: dict[str, bool],
                           where: str) -> tuple[list[dict], list[dict]]:
            """ABSOLUTE set-list -> (set, cleared) with inds; cleared = owned - set."""
            es, seen = [], set()
            for n in set_names:
                if n not in event_by_name:
                    problems.append(f"{where}: unknown event {n}")
                    continue
                if n in seen:
                    continue
                seen.add(n)
                es.append({"ind": event_by_name[n]["ind"], "name": n,
                           "owned": n in owned and not context.get(n, False)})
            cleared = [{"ind": event_by_name[n]["ind"], "name": n, "owned": True}
                       for n in sorted(owned - seen)]
            for n, _ in context.items():
                if n not in seen and n in event_by_name and n not in owned:
                    cleared.append({"ind": event_by_name[n]["ind"], "name": n, "owned": False})
            return es, sorted(cleared, key=lambda e: e["ind"])

        states: list[dict] = []
        if curated:
            stats["curatedMaps"] += 1
            for st in curated["stages"]:
                where = f"{path.stem}[stage {st['id']}]"
                if st["script"] not in per_val:
                    problems.append(f"{where}: script value {st['script']} not in table")
                ctx = {n: True for n in st.get("context", [])}
                ev_set, ev_clr = resolve_events(st.get("events", []), ctx, where)
                overrides = {}
                for tconst in st.get("missables", {}).get("show", []):
                    overrides[tconst] = "show"
                for tconst in st.get("missables", {}).get("hide", []):
                    overrides[tconst] = "hide"
                for tconst in overrides:
                    if tconst not in miss_by_const:
                        problems.append(f"{where}: unknown toggle {tconst}")
                    elif miss_by_const[tconst]["map"] not in own_names:
                        problems.append(f"{where}: {tconst} belongs to "
                                        f"{miss_by_const[tconst]['map']}, not this map")
                for b in st.get("badges", []):
                    if b not in BADGE_BITS:
                        problems.append(f"{where}: unknown badge {b}")
                states.append({
                    "id": st["id"], "kind": "resting",
                    "script": st["script"],
                    "scriptName": sf.val_const.get(st["script"], f"UNNAMED_{st['script']}"),
                    "name": st["name"], "desc": st["desc"],
                    "timeline": st.get("timeline", ""),
                    "trigger": st.get("trigger", {}),
                    "advance": st.get("advance", []),
                    "save": {
                        "script": st["script"],
                        "events": {"set": ev_set, "cleared": ev_clr},
                        "missables": stage_missables(map_missables, overrides),
                        "badges": st.get("badges", []),
                        "notes": st.get("saveNotes", []),
                    },
                })
                # transient states attached to (leaving) this stage
                for k, tval in enumerate(st.get("transients", []), start=1):
                    if tval not in per_val:
                        problems.append(f"{where}: transient value {tval} not in table")
                        continue
                    tinfo = per_val[tval]
                    states.append({
                        "id": f"{st['id']}.{k}", "kind": "transient",
                        "script": tval, "scriptName": tinfo["name"],
                        "name": f"Cutscene step: {tinfo['name']}",
                        "desc": st.get("transientDesc",
                                       "Mid-cutscene machinery between stages; a save loaded "
                                       "here resumes inside the cutscene."),
                        "trigger": {}, "advance": [
                            {"to": str(v), "text": "automatic", "auto": True}
                            for v in tinfo["next"]],
                    })
            progression = {
                "start": curated["stages"][0]["id"],
                "end": curated.get("end", curated["stages"][-1]["id"]),
                "order": [s["id"] for s in curated["stages"]],
                "branches": curated.get("branches", {}),
                "messy": curated.get("messy", False),
                "notes": curated.get("notes", ""),
            }
        else:
            # ── auto skeleton: initial stage + (if the map writes anything) a settled one ──
            ev_set0, ev_clr0 = resolve_events([], {}, path.stem)
            states.append({
                "id": "1", "kind": "resting", "script": 0,
                "scriptName": sf.val_const.get(0, "UNNAMED_0"),
                "name": "Initial state",
                "desc": "The map as a fresh save finds it: script byte 0, none of this map's "
                        "own story flags set, missables at their defaults.",
                "timeline": "", "trigger": {"type": "start", "text": "New game."},
                "advance": [], "derived": True,
                "save": {"script": 0, "events": {"set": ev_set0, "cleared": ev_clr0},
                         "missables": stage_missables(map_missables, {}),
                         "badges": [], "notes": []},
            })
            final_sets = sorted((owned & file_sets) | (owned & set(sf.trainer_events)))
            shows = set(file_shows)
            hides = set(file_hides)
            badges_all = sorted(file_badges)
            if final_sets or shows or hides or badges_all:
                overrides = {n: "show" for n in shows if n in miss_by_const}
                overrides |= {n: "hide" for n in hides if n in miss_by_const}
                ev_set1, ev_clr1 = resolve_events(final_sets, {}, path.stem)
                states[0]["advance"] = [{"to": "2",
                                         "text": "This map's scripted events all run to "
                                                 "completion (derived summary).",
                                         "auto": False}]
                states.append({
                    "id": "2", "kind": "resting", "script": settle,
                    "scriptName": sf.val_const.get(settle, f"UNNAMED_{settle}"),
                    "name": "Scripted events completed (derived)",
                    "desc": "Every event flag this map's own scripts set is on, every missable "
                            "they toggle is toggled, badges they award are held. Derived "
                            "mechanically from the script source, not yet hand-curated.",
                    "timeline": "", "trigger": {
                        "type": "event",
                        "text": "The map's scripted story has fully played out."},
                    "advance": [], "derived": True,
                    "save": {"script": settle,
                             "events": {"set": ev_set1, "cleared": ev_clr1},
                             "missables": stage_missables(map_missables, overrides),
                             "badges": badges_all, "notes": []},
                })
            progression = {
                "start": "1", "end": states[-1]["id"],
                "order": [s["id"] for s in states if s["kind"] == "resting"],
                "branches": {}, "messy": False,
                "notes": "Auto-derived skeleton (not hand-curated): the initial state and, "
                         "where the map's scripts write durable facts, a completed state.",
            }

        # deltas between consecutive resting stages (the roll-forward increments)
        resting = [s for s in states if s["kind"] == "resting" and "save" in s]
        for prev, cur in zip(resting, resting[1:]):
            pset = {e["ind"] for e in prev["save"]["events"]["set"]}
            cset = {e["ind"] for e in cur["save"]["events"]["set"]}
            pmis = {m["ind"]: m["state"] for m in prev["save"]["missables"]}
            cmis = {m["ind"]: m["state"] for m in cur["save"]["missables"]}
            cur["delta"] = {
                "from": prev["id"],
                "script": {"from": prev["save"]["script"], "to": cur["save"]["script"]},
                "events": {
                    "set": sorted(cset - pset),
                    "cleared": sorted(pset - cset),
                },
                "missables": {
                    "show": sorted(i for i, s2 in cmis.items()
                                   if s2 == "show" and pmis.get(i) != "show"),
                    "hide": sorted(i for i, s2 in cmis.items()
                                   if s2 == "hide" and pmis.get(i) != "hide"),
                },
                "badges": sorted(set(cur["save"]["badges"]) - set(prev["save"]["badges"])),
            }

        warp_in = entry.get("warpIn") or []
        entry_spot = ({"kind": "warp", "warp": 0, "x": warp_in[0]["x"], "y": warp_in[0]["y"]}
                      if warp_in else
                      {"kind": "center", "x": entry.get("width", 0),
                       "y": entry.get("height", 0)})

        blueprint = {
            "map": entry["name"],
            "mapInd": map_id,
            "scriptSlot": slot_of_map.get(entry["name"]),
            "source": f"scripts/{path.name}",
            "curated": bool(curated),
            "entry": entry_spot,
            "progression": progression,
            "scriptValues": script_values,
            "states": states,
        }
        out_files[path.stem] = blueprint
        digest[path.stem] = {"map": entry["name"], "values": list(per_val.values()),
                             "order": order, "restingOrder": resting_order,
                             "owned": sorted(owned), "trainerEvents": sf.trainer_events}
        stats["maps"] += 1
        stats["stages"] += len([s for s in states if s["kind"] == "resting"])

    # curated entries that matched no file are typos -- fail loudly ("_"-keys are comments)
    for name in curated_all:
        if not name.startswith("_") and name not in out_files:
            problems.append(f"curated overlay: no script file produced a blueprint for '{name}'")

    index = [{"file": f"{k}.json", "map": v["map"], "mapInd": v["mapInd"],
              "scriptSlot": v["scriptSlot"], "curated": v["curated"],
              "stages": len([s for s in v["states"] if s["kind"] == "resting"]),
              "messy": v["progression"]["messy"]}
             for k, v in sorted(out_files.items(), key=lambda kv: kv[1]["mapInd"])]

    print(f"maps with blueprints    : {stats['maps']}")
    print(f"  resting stages        : {stats['stages']}")
    print(f"  curated maps          : {stats['curatedMaps']}")
    print(f"script values: resting {stats['resting']} / transient {stats['transient']}")

    if problems:
        print(f"\n!! {len(problems)} problem(s):")
        for p in problems[:60]:
            print(f"  {p}")
        print("\nFAIL: every reference must resolve.", file=sys.stderr)
        return 1

    if args.digest:
        tmp = REPO / "tmp"
        tmp.mkdir(exist_ok=True)
        (tmp / "map-states-digest.json").write_text(
            json.dumps(digest, indent=1, ensure_ascii=False), encoding="utf-8")
        print(f"digest -> {tmp / 'map-states-digest.json'}")

    if args.check:
        drift = []
        for k, v in out_files.items():
            f = OUT_DIR / f"{k}.json"
            if not f.exists() or json.loads(read(f)) != v:
                drift.append(k)
        fidx = OUT_DIR / "_index.json"
        if not fidx.exists() or json.loads(read(fidx)) != index:
            drift.append("_index")
        existing = {p.stem for p in OUT_DIR.glob("*.json")} if OUT_DIR.exists() else set()
        stale = existing - set(out_files) - {"_index"}
        if stale:
            drift.extend(sorted(stale))
        if drift:
            print(f"\n--check: {len(drift)} file(s) would change: {', '.join(sorted(drift)[:12])}")
            return 1
        print("\n--check: map-states are up to date.")
        return 0

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for p in OUT_DIR.glob("*.json"):
        if p.stem not in out_files and p.name != "_index.json":
            p.unlink()
    for k, v in out_files.items():
        (OUT_DIR / f"{k}.json").write_text(
            json.dumps(v, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    (OUT_DIR / "_index.json").write_text(
        json.dumps(index, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"\nwrote {stats['maps']} blueprint(s) + _index.json -> {OUT_DIR}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
