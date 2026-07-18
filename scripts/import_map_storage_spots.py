#!/usr/bin/env python3
"""
import_map_storage_spots.py -- WHERE a script happens, and WHICH event flags it writes there.

The 16f-b importer. Turns the 16f-a research (extract_map_storage_locations.py, which writes a
git-ignored report) into shipped data: a `storageSpots` list on each map in maps.json.

── Why this exists, in leadership's own words (2026-07-17) ───────────────────────────────────

    "what i have previously discussed was giving event flags potentially multiple locations, any
     script at any point on the map that sets or changes event flags, well that looks like a good
     location to me x/y. Event flags dont have a location because they can be in multiple places
     there tied to scripts so show them where they happen on the map."

That is NOT in tension with the 16f-a finding that "event flags have NO location". Both are true,
and together they are the whole model:

    An event flag has no location OF ITS OWN. It has as many locations as there are located
    scripts that write it. So a flag is shown wherever it HAPPENS -- one spot per writing site,
    zero spots if nothing located writes it.

Nothing here is invented. A flag with no located writer gets NO spot; that is the rule, not a gap.

── The two ways a script gets a location ─────────────────────────────────────────────────────

  1. `ld hl, .Coords` + `call ArePlayerCoordsInArray`, with a `dbmapcoord` table -> real TILES.
  2. raw `ld a, [wYCoord]` / `[wXCoord]` + `cp N` -> a whole ROW / COLUMN.

⚠️ A range is NOT a box (leadership's settled call): it is HIGHLIGHT geometry, drawn at its own
truthful extent, while hover/click happen on the 32x32 block. This importer therefore records the
range as a range (`span: "row"|"col"`) and does not fan it out or invent a w/h.

── THE CHAIN, and it is the point of this importer ───────────────────────────────────────────

A trigger routine rarely writes the interesting flags itself. It writes the SCRIPT STEP, and the
step it hands off to does the work:

    PalletTownDefaultScript:                    ; <- the LOCATION (cp 1 on wYCoord)
        SetEvent EVENT_OAK_APPEARED_IN_PALLET   ;    one flag, here
        ld a, SCRIPT_PALLETTOWN_OAK_HEY_WAIT    ; <- the CHAIN
        ld [wPalletTownCurScript], a
    PalletTownOakHeyWaitScript:                 ;    ... and the rest of the cutscene's flags
        ld a, TOGGLE_PALLET_TOWN_OAK            ;    live in the routines it leads to
        predef ShowObject

Attributing only a routine's OWN writes finds just 13 located spots that touch events. Following
`ld [w<Map>CurScript], a` through `<Map>_ScriptPointers` unions the chain, which is what makes a
trigger's tabs complete -- and it is exactly what leadership described: the flags happen *there*,
because standing on that tile is what causes every one of them.

⚠️ This is a PROVEN mechanical link (constant -> pointer table -> routine), NOT proximity.
"The flag is near the object" is the static co-location reasoning that produced the Route 22 false
positive and got the conflicts system shelved. Routine boundaries + the pointer table are the unit.
Anything the source does not prove gets nothing.

Source:  <pokered>/scripts/<Map>.asm            (routines, coord tables, ScriptPointers, the chain)
         <pokered>/constants/map_constants.asm  (map const -> map id)
Reads:   projects/db/assets/data/events.json    (EVENT_x  -> flag index, via `pretName`)
         projects/db/assets/data/missables.json (TOGGLE_x -> filter index, via `toggleConst`)
Output:  projects/db/assets/data/maps.json      (adds "storageSpots": [...] per map)

Self-checking:
  1. Every map const must resolve to a maps.json entry (id identity), or we are attaching a
     script to the wrong town.
  2. Every EVENT_x / TOGGLE_x named by a located routine must resolve. An unresolved constant is
     a parse bug, not a fallback -- fail loudly.
  3. Chain walks are cycle-guarded and depth-capped; a cycle is reported, never silently trimmed.

Usage:
    python scripts/import_map_storage_spots.py [--pokered <path>] [--check]
    --check   parse + verify + diff against the committed maps.json, write nothing. Exit 1 on drift.
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

REPO = Path(__file__).resolve().parent.parent
DEFAULT_POKERED = Path.home() / "Documents" / "projects" / "pokered"
DATA = REPO / "projects" / "db" / "assets" / "data"
MAPS_JSON = DATA / "maps.json"
EVENTS_JSON = DATA / "events.json"
MISSABLES_JSON = DATA / "missables.json"

MAX_CHAIN_DEPTH = 24  # the longest real cutscene is far shorter; a hit here means a cycle

# ⚠️ Anchored at column 0 -- instructions are always indented. A GLOBAL label starts a routine.
LABEL_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_.]*):")
# ⚠️ Coord tables are usually LOCAL labels (`.PlayerCoordsArray:`) -- a leading dot is not a
# letter, and a [A-Za-z_]-anchored regex misses 31 of 53 files.
# ⚠️ The colon is OPTIONAL in RGBDS (`.Route22RivalBattleCoords`). Requiring it loses Route 22.
ANY_LABEL_RE = re.compile(r"^(\.?[A-Za-z_][A-Za-z0-9_.]*):?\s*(?:;.*)?$")
COORD_RE = re.compile(r"^\s*dbmapcoord\s+(\d+)\s*,\s*(\d+)")
ARRAY_RE = re.compile(r"ld\s+hl\s*,\s*(\.?[A-Za-z_][A-Za-z0-9_.]*)\s*\n\s*call\s+ArePlayerCoordsInArray")
CARDKEY_RE = re.compile(r"ld\s+hl\s*,\s*(\.?[A-Za-z_][A-Za-z0-9_.]*)\s*\n\s*call\s+\w*SetCardKeyDoor\w*")
# ⚠️ `SetEvent` has EIGHT SIBLINGS, and matching only the two obvious ones silently drops a THIRD
# of every event write in the game. Counted in pret's scripts/:
#     SetEvent 105 · SetEventReuseHL 17 · SetEventAfterBranchReuseHL 14 · SetEventForceReuseHL 1
#     ResetEvent 12 · ResetEventReuseHL 8 · ResetEvents 8 · SetEvents 4
#     SetEventRange 7 · ResetEventRange 2
# The `...ReuseHL` variants are pure codegen (reuse the HL register) -- semantically identical to
# the plain form. The plural and Range forms write SEVERAL flags at once. A naive
# `\b(SetEvent|ResetEvent)\s` matches none of the suffixed ones (the `\s` fails against the "R" of
# "ReuseHL"), so 61 of 178 sites vanish without a word.
SETEV_RE = re.compile(
    r"\b(Set|Reset)Event(s|Range|ReuseHL|AfterBranchReuseHL|ForceReuseHL)?\s+([^;\n]+)"
)
# `const_def` / `const NAME` / `const_skip N` / `DEF NAME EQU const_value[ - 1]`
CONST_DEF_RE = re.compile(r"^\s*const_def\b", re.MULTILINE)
CONST_RE = re.compile(r"^\s*const\s+([A-Z0-9_]+)", re.MULTILINE)
CONST_SKIP_RE = re.compile(r"^\s*const_skip(?:\s+(\d+))?", re.MULTILINE)
CONST_ALIAS_RE = re.compile(r"^\s*DEF\s+([A-Z0-9_]+)\s+EQU\s+const_value\s*(-\s*1)?", re.MULTILINE)
# ⚠️ The toggle and its VERB must be captured together:
#     ld a, TOGGLE_BILL_POKEMON / ld [wToggleableObjectIndex], a / predef HideObject
# Matching `ld a, TOGGLE_x` alone and separately asking "does this routine contain a Show/Hide?"
# cannot tell you WHICH was shown and which was hidden -- and a routine that does one of each
# reports both as the same thing. Leadership: "i need them to be aware of which are on and off and
# where and by whom".
#
# ⚠️⚠️ `predef_jump` IS a Show/Hide and requiring `predef ` alone silently drops it. It is
# `predef` + `ret` -- the tail-call form the game uses when the toggle is the routine's last act,
# which is exactly when it is the CLIMAX of a phase. Pallet Town proves the point in one routine:
#     ld a, TOGGLE_DAISY_SITTING  / predef      HideObject   <- matched
#     ld a, TOGGLE_DAISY_WALKING  / predef_jump ShowObject   <- MISSED without `(?:_jump)?`
# Daisy would have been recorded as hidden and never shown again.
TOGGLE_RE = re.compile(
    r"ld\s+a\s*,\s*(TOGGLE_[A-Z0-9_]+)[\s\S]{0,80}?predef(?:_jump)?\s+(Show|Hide)Object"
)
RAWY_RE = re.compile(r"ld\s+a\s*,\s*\[wYCoord\][\s\S]{0,120}?cp\s+(\d+)")
RAWX_RE = re.compile(r"ld\s+a\s*,\s*\[wXCoord\][\s\S]{0,120}?cp\s+(\d+)")
# `dw_const PalletTownOakHeyWaitScript, SCRIPT_PALLETTOWN_OAK_HEY_WAIT`
PTR_RE = re.compile(r"^\s*dw_const\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*(SCRIPT_[A-Z0-9_]+)", re.MULTILINE)
# The chain: `ld a, SCRIPT_x` ... `ld [w<Map>CurScript], a`. The write must actually be to a
# CurScript variable -- plenty of routines load a SCRIPT_ constant for other reasons.
CHAIN_RE = re.compile(r"ld\s+a\s*,\s*(SCRIPT_[A-Z0-9_]+)[\s\S]{0,80}?ld\s+\[w\w*CurScript\]\s*,\s*a")
MAPCONST_RE = re.compile(r"^\s*map_const\s+([A-Z0-9_]+)\s*,", re.MULTILINE)


def read(p: Path) -> str:
    return p.read_text(encoding="utf-8")


def norm(s: str) -> str:
    """Normalise a map const or a script filename to a comparable key.

    PALLET_TOWN -> 'pallettown'; 'PalletTown.asm' -> 'pallettown'. Underscores and case are the
    only differences between pret's two spellings of the same map, so this is a safe join --
    and every match is asserted, so a miss is reported rather than guessed at.
    """
    return s.replace("_", "").replace(".asm", "").lower()


def parse_event_constants(text: str) -> dict[str, int]:
    """Every event constant -> its flag index, straight out of `event_constants.asm`.

    Needed because `SetEventRange` / `ResetEventRange` do NOT always name EVENT_ constants:
    `ResetEventRange INDIGO_PLATEAU_EVENTS_START, INDIGO_PLATEAU_EVENTS_END, 1` names two
    block markers, which events.json has never heard of (it only carries `pretName`s). Resolving a
    range through this table is the difference between importing it and dropping it.

    RGBDS semantics, and all four matter:
      const_def          -> the counter restarts at 0
      const NAME         -> NAME = counter; counter += 1
      const_skip N       -> counter += N   (N defaults to 1) -- the padding gaps
      DEF X EQU const_value[ - 1] -> an ALIAS for wherever the counter currently is
    """
    out: dict[str, int] = {}
    counter = 0
    for ln in text.split("\n"):
        if CONST_DEF_RE.match(ln):
            counter = 0
            continue
        m = CONST_RE.match(ln)
        if m:
            out[m.group(1)] = counter
            counter += 1
            continue
        m = CONST_SKIP_RE.match(ln)
        if m:
            counter += int(m.group(1)) if m.group(1) else 1
            continue
        m = CONST_ALIAS_RE.match(ln)
        if m:
            out[m.group(1)] = counter - 1 if m.group(2) else counter
    return out


def expand_event_write(verb: str, suffix: str, args: str,
                       consts: dict[str, int]) -> tuple[str, list[str], bool]:
    """One Set/Reset macro -> (action, [constant names], ok).

    `ok` is False when an argument cannot be resolved -- reported by the caller, never dropped
    quietly. A missing tab is indistinguishable from "nothing happens here", which is the one
    thing this whole feature must not say by accident.
    """
    action = "reset" if verb == "Reset" else "set"
    names = [a.strip() for a in args.split(",")]
    # Trailing numeric args are macro options (`ResetEventRange A, B, 1`), not flags.
    names = [n for n in names if n and not n.isdigit()]
    if not names:
        return action, [], False

    if suffix == "Range":
        # A RANGE writes every flag between its two endpoints, inclusive. Expanding it is the only
        # way "which flags does this phase touch" can be true for the 9 range sites.
        if len(names) < 2 or names[0] not in consts or names[1] not in consts:
            return action, [], False
        lo, hi = consts[names[0]], consts[names[1]]
        if hi < lo:
            return action, [], False
        return action, [n for n, v in consts.items() if lo <= v <= hi and n.startswith("EVENT_")], True

    # Single, plural, and every ...ReuseHL variant: each named constant, as written.
    bad = [n for n in names if n not in consts]
    return action, [n for n in names if n in consts], not bad


def split_routines(text: str) -> list[tuple[str, str]]:
    """Split a script file into (global label, body). The ROUTINE is the unit of truth: a flag
    written in the same routine as a coord test belongs to that trigger. Proximity across a
    label boundary proves nothing at all."""
    routines: list[tuple[str, str]] = []
    cur, body = None, []
    for ln in text.split("\n"):
        m = LABEL_RE.match(ln)
        if m and not ln.startswith((" ", "\t")):
            if cur is not None:
                routines.append((cur, "\n".join(body)))
            cur, body = m.group(1), []
        else:
            body.append(ln)
    if cur is not None:
        routines.append((cur, "\n".join(body)))
    return routines


def coord_tables(text: str) -> dict[str, list[tuple[int, int]]]:
    """label -> [(x, y)]. `dbmapcoord y, x` -- the macro's arg order is y first."""
    tables: dict[str, list[tuple[int, int]]] = {}
    cur = None
    for ln in text.split("\n"):
        m = ANY_LABEL_RE.match(ln)
        if m:
            cur = m.group(1)
            continue
        c = COORD_RE.match(ln)
        if c and cur is not None:
            tables.setdefault(cur, []).append((int(c.group(2)), int(c.group(1))))
    return tables


class ScriptFile:
    """One map's script file, parsed once."""

    def __init__(self, text: str, consts: dict[str, int] | None = None):
        self.text = text
        self.consts = consts or {}
        self.problems: list[str] = []
        self.tables = coord_tables(text)
        self.routines = dict(split_routines(text))
        # SCRIPT_x -> routine label, straight out of the map's own pointer table.
        self.ptrs = {m.group(2): m.group(1) for m in PTR_RE.finditer(text)}
        # routine label -> its PHASE (the wCurMapScript value that runs it). The pointer table's
        # ORDER is the step number -- the same value `scriptEntries`/WorldScripts already use, so
        # a flag's phase and the map's "Current script" dropdown speak the same language.
        self.step_of = {}
        for i, (const, label) in enumerate(self.ptrs.items()):
            self.step_of[label] = i
        self.step_name = {label: const for const, label in self.ptrs.items()}

    def own_writes(self, body: str) -> tuple[list[tuple[str, str]], list[tuple[str, str]]]:
        """What THIS routine does itself: [(action, EVENT_x)] and [(TOGGLE_x, Show|Hide)].

        The ACTION is kept for BOTH, not flattened. "Turned on here, turned off there" is the whole
        question leadership asked of event flags AND filter flags -- a set and a reset of the same
        flag are opposite facts, and a union of them says nothing at all.
        """
        events: list[tuple[str, str]] = []
        for m in SETEV_RE.finditer(body):
            action, names, ok = expand_event_write(m.group(1), m.group(2) or "", m.group(3),
                                                   self.consts)
            if not ok:
                self.problems.append(
                    f"unresolved {m.group(1)}Event{m.group(2) or ''} {m.group(3).strip()!r}")
            for n in names:
                events.append(("SetEvent" if action == "set" else "ResetEvent", n))
        toggles = [(m.group(1), m.group(2)) for m in TOGGLE_RE.finditer(body)]
        return events, toggles

    def walk_chain(self, label: str) -> tuple[list[dict], list[dict], list[str], bool]:
        """Walk `label` and every step it hands off to, keeping WHO did WHAT, WHERE in the sequence.

        Returns (writes, toggles, visited_labels, hit_cycle_or_depth), where each write is
        `{event, action, routine, step, stepName, viaChain}`.

        ⚠️ This deliberately does NOT collapse to a set of flag names. Leadership, 2026-07-17:

            "the event flags not only need to keep track of the map locations that called them,
             they need to keep track of which script phases a map goes through like (First phase
             flags) (second phase flags) like which scripts and locations did what to them turning
             them on off etc... have the flags be aware of there states across scripts"

        So every write keeps its PHASE (the wCurMapScript value that runs the routine), its
        ROUTINE, and its ACTION. A flag's story across a map is then just its writes in step order.

        The link followed is `ld a, SCRIPT_x` + `ld [w<Map>CurScript], a` resolved through the
        map's `dw_const` pointer table -- a mechanical, source-proven hop, not a guess.
        """
        writes: list[dict] = []
        toggles: list[dict] = []
        seen: list[str] = []
        overflowed = False

        # BFS, so `seen` (and therefore the order flags are discovered) follows the sequence the
        # player actually experiences rather than a stack's reverse.
        queue = [(label, 0)]
        while queue:
            cur, depth = queue.pop(0)
            if cur in seen:
                continue  # cycle guard: a script step may loop back to an earlier one
            if depth > MAX_CHAIN_DEPTH:
                overflowed = True
                continue
            body = self.routines.get(cur)
            if body is None:
                continue
            seen.append(cur)
            evs, tgs = self.own_writes(body)
            # depth 0 is the trigger itself; anything deeper fires LATER in the sequence the
            # trigger starts, not on the tile.
            attrib = {
                "routine": cur,
                "step": self.step_of.get(cur, -1),
                "stepName": self.step_name.get(cur, ""),
                "viaChain": depth > 0,
            }
            for action, name in evs:
                writes.append({"event": name,
                               "action": "reset" if action == "ResetEvent" else "set",
                               **attrib})
            for name, verb in tgs:
                # A filter flag's bit is SET = HIDDEN, so ShowObject clears it and HideObject sets
                # it. Recorded as the verb the game uses, not as the bit -- "shown/hidden" is what
                # a person means, and the inversion is exactly what gets mixed up.
                toggles.append({"toggle": name,
                                "action": "show" if verb == "Show" else "hide",
                                **attrib})
            for m in CHAIN_RE.finditer(body):
                nxt = self.ptrs.get(m.group(1))
                if nxt is not None and nxt not in seen:
                    queue.append((nxt, depth + 1))
        return writes, toggles, seen, overflowed

    def step_writes(self) -> dict[int, dict]:
        """Every script PHASE of this map -> what it does to the world.

        Not limited to located routines: a phase that writes flags from a text box rather than a
        tile still has a phase, and leadership asked for the map's phases, not just its tiles.
        `{step: {"sets": [...], "resets": [...], "toggles": [...], "name": ..., "routine": ...}}`
        """
        out: dict[int, dict] = {}
        for label, step in self.step_of.items():
            body = self.routines.get(label)
            if body is None:
                continue
            evs, tgs = self.own_writes(body)
            if not evs and not tgs:
                continue
            out[step] = {
                "routine": label,
                "name": self.step_name.get(label, ""),
                "sets": sorted({n for a, n in evs if a == "SetEvent"}),
                "resets": sorted({n for a, n in evs if a == "ResetEvent"}),
                "shows": sorted({n for n, v in tgs if v == "Show"}),
                "hides": sorted({n for n, v in tgs if v == "Hide"}),
            }
        return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--pokered", type=Path, default=DEFAULT_POKERED)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    sdir = args.pokered / "scripts"
    mconst = args.pokered / "constants" / "map_constants.asm"
    econst = args.pokered / "constants" / "event_constants.asm"
    if not sdir.is_dir() or not mconst.is_file() or not econst.is_file():
        print(f"FAIL: pokered clone not usable at {args.pokered}", file=sys.stderr)
        return 1

    event_consts = parse_event_constants(read(econst))

    # ── map const -> map id. `const_def` starts at 0 and map_const increments. ────────────
    map_consts = MAPCONST_RE.findall(read(mconst))
    const_to_id = {c: i for i, c in enumerate(map_consts)}

    maps = json.loads(read(MAPS_JSON))
    by_ind = {m["ind"]: m for m in maps}

    events = json.loads(read(EVENTS_JSON))
    event_ind = {e["pretName"]: e["ind"] for e in events if e.get("pretName")}
    missables = json.loads(read(MISSABLES_JSON))
    filter_ind = {m["toggleConst"]: m["ind"] for m in missables if m.get("toggleConst")}

    # script filename key -> map id
    file_to_id = {norm(c): i for c, i in const_to_id.items()}

    problems: list[str] = []
    stats = {"files": 0, "tiles": 0, "rows": 0, "cols": 0, "cardKey": 0,
             "spotsWithEvents": 0, "chainHops": 0, "eventLinks": 0, "phases": 0}
    spots_by_id: dict[int, list[dict]] = {}
    phases_by_id: dict[int, list[dict]] = {}

    for path in sorted(sdir.glob("*.asm")):
        key = norm(path.name)
        map_id = file_to_id.get(key)
        sf = ScriptFile(read(path), event_consts)

        # ── The map's PHASE STORY: what each script step does to the world ────────────────
        #
        # Leadership: "they need to keep track of which script phases a map goes through like
        # (First phase flags) (second phase flags)". Deliberately NOT limited to located routines
        # -- a phase that writes flags from a text box has no tile, and it is still a phase of the
        # map. This is the per-flag "who turned me on, and when" answer; the located spots below
        # add "and where".
        phases: list[dict] = []
        for step, info in sorted(sf.step_writes().items()):
            p: dict = {"step": step, "name": info["name"], "routine": info["routine"]}
            for key_, src in (("sets", "sets"), ("resets", "resets")):
                ids = []
                for name in info[src]:
                    if name not in event_ind:
                        problems.append(f"{path.name}: unknown event {name}")
                    else:
                        ids.append(event_ind[name])
                p[key_] = sorted(ids)
            for key_, src in (("shows", "shows"), ("hides", "hides")):
                ids = []
                for name in info[src]:
                    if name not in filter_ind:
                        problems.append(f"{path.name}: unknown toggle {name}")
                    else:
                        ids.append(filter_ind[name])
                p[key_] = sorted(ids)
            phases.append(p)
        if phases and map_id is not None and map_id in by_ind:
            phases_by_id[map_id] = phases
            stats["phases"] += len(phases)

        # Anything the macro parser could not resolve is a PARSE BUG, and it surfaces here rather
        # than turning into a flag that quietly never appears anywhere.
        for p in sorted(set(sf.problems)):
            problems.append(f"{path.name}: {p}")

        located: list[dict] = []
        for label, body in sf.routines.items():
            writes, tg, seen, overflow = sf.walk_chain(label)
            if overflow:
                problems.append(f"{path.name}:{label}: chain exceeded depth {MAX_CHAIN_DEPTH}")

            def mk(base: dict) -> dict:
                # Resolve to OUR indices. An unresolved constant is a parse bug: fail, never
                # silently drop -- a missing tab is indistinguishable from "nothing happens here".
                evs, fids = [], []
                for w in writes:
                    if w["event"] not in event_ind:
                        problems.append(f"{path.name}:{label}: unknown event {w['event']}")
                        continue
                    evs.append({
                        "ind": event_ind[w["event"]],
                        "action": w["action"],       # set / reset -- opposite facts, kept apart
                        "step": w["step"],           # WHICH PHASE of this map does it
                        "stepName": w["stepName"],
                        "routine": w["routine"],
                        "viaChain": w["viaChain"],
                    })
                for w in tg:
                    if w["toggle"] not in filter_ind:
                        problems.append(f"{path.name}:{label}: unknown toggle {w['toggle']}")
                        continue
                    fids.append({
                        "ind": filter_ind[w["toggle"]],
                        "action": w["action"],       # show / hide -- by whom, and which way
                        "step": w["step"],
                        "stepName": w["stepName"],
                        "routine": w["routine"],
                        "viaChain": w["viaChain"],
                    })
                base["routine"] = label
                base["step"] = sf.step_of.get(label, -1)
                # In the sequence's own order (BFS), then by flag, so "first phase / second phase"
                # reads down the list the way the player lives it.
                base["events"] = sorted(evs, key=lambda e: (e["step"], e["ind"]))
                base["filters"] = sorted(fids, key=lambda f: (f["step"], f["ind"]))
                # The chain is worth recording: a flag reached through it is written by a LATER
                # step, not on the tile itself -- true, and worth being able to say out loud.
                base["chain"] = seen[1:] if len(seen) > 1 else []
                return base

            for m in ARRAY_RE.finditer(body):
                for (x, y) in sf.tables.get(m.group(1), []):
                    located.append(mk({"kind": "scriptTile", "x": x, "y": y}))
                    stats["tiles"] += 1
            for m in CARDKEY_RE.finditer(body):
                for (x, y) in sf.tables.get(m.group(1), []):
                    located.append(mk({"kind": "cardKeyDoor", "x": x, "y": y}))
                    stats["cardKey"] += 1
            for m in RAWY_RE.finditer(body):
                located.append(mk({"kind": "scriptRow", "y": int(m.group(1)), "span": "row"}))
                stats["rows"] += 1
            for m in RAWX_RE.finditer(body):
                located.append(mk({"kind": "scriptCol", "x": int(m.group(1)), "span": "col"}))
                stats["cols"] += 1
            if located:
                stats["chainHops"] += max(0, len(seen) - 1)

        if not located:
            continue
        stats["files"] += 1
        if map_id is None:
            problems.append(f"{path.name}: no map constant matches (key '{key}') -- {len(located)} spot(s) would be orphaned")
            continue
        if map_id not in by_ind:
            problems.append(f"{path.name}: map id {map_id} is not in maps.json")
            continue
        spots_by_id.setdefault(map_id, []).extend(located)

    for lst in spots_by_id.values():
        for s in lst:
            if s["events"]:
                stats["spotsWithEvents"] += 1
                stats["eventLinks"] += len(s["events"])

    print(f"script files with a location : {stats['files']}")
    print(f"  tiles (dbmapcoord)         : {stats['tiles']}")
    print(f"  rows  (raw wYCoord cp)     : {stats['rows']}")
    print(f"  cols  (raw wXCoord cp)     : {stats['cols']}")
    print(f"  card-key doors             : {stats['cardKey']}")
    print(f"maps carrying spots          : {len(spots_by_id)}")
    print(f"located spots that WRITE events: {stats['spotsWithEvents']}  (was 13 without the chain)")
    print(f"  total flag<->place links   : {stats['eventLinks']}")
    print(f"maps with a phase story      : {len(phases_by_id)}")
    print(f"  script phases that change the world: {stats['phases']}")

    if problems:
        print(f"\n!! {len(problems)} problem(s):")
        for p in problems[:40]:
            print(f"  {p}")
        print("\nFAIL: every located spot must attach to a real map and resolve every constant.",
              file=sys.stderr)
        return 1

    # ── Merge additively. Existing fields are never touched. ─────────────────────────────
    for entry in maps:
        for field, src in (("storageSpots", spots_by_id), ("scriptPhases", phases_by_id)):
            got = src.get(entry["ind"])
            if got is not None:
                entry[field] = got
            elif field in entry:
                del entry[field]  # a map that lost its last spot must not keep a stale one

    if args.check:
        current = json.loads(read(MAPS_JSON))
        cur_by_ind = {e["ind"]: e for e in current}
        drift = sum(
            1 for e in maps
            if cur_by_ind.get(e["ind"], {}).get("storageSpots") != e.get("storageSpots")
            or cur_by_ind.get(e["ind"], {}).get("scriptPhases") != e.get("scriptPhases")
        )
        if drift:
            print(f"\n--check: {drift} map(s) would change. Run without --check to write.")
            return 1
        print("\n--check: maps.json storageSpots are up to date.")
        return 0

    MAPS_JSON.write_text(json.dumps(maps, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"\nwrote {MAPS_JSON}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
