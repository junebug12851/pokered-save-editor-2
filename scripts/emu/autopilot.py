#!/usr/bin/env python3
"""The autopilot EXECUTOR — walks navigate.py's plans on a live PyBoy, inside the
session child (drive_session.py). One PyBoy, one process; every loop frame-bounded.

Trusts WRAM, never pixels: coords (wXCoord/wYCoord), map (wCurMap), battle
(wIsInBattle), facing (sprite 0 StateData1+9), text box (wFontLoaded bit 0), the
menu cursor (wCurrentMenuItem), and the LIVE NPC squares (StateData2 mapY/mapX,
re-read every step — NPCs are moving collision, and talk_to's moving targets).

Design + verification battery: notes/plans/dev-autopilot.md.
The menu/text addresses (below wMainDataStart, so absent from every save-field
probe) are validated live by probe_autopilot.py before anything relies on them.
"""
from __future__ import annotations

from navigate import DIRS, World, astar, route

# ------------------------------------------------------------------ WRAM map
W_MAP, W_H, W_W = 0xD35E, 0xD368, 0xD369
W_Y, W_X = 0xD361, 0xD362
W_BATTLE = 0xD057            # wIsInBattle: 0 no, 1 wild, 2 trainer
W_LASTMAP = 0xD365           # wLastMap (save 0x2611 + 0xAD54)
W_FONT = 0xCFC4              # wFontLoaded bit 0 = a text box is up
W_MENU = 0xCC26              # wCurrentMenuItem
W_JOY_IGNORE = 0xCD6B        # nonzero = the game holds the controls (cutscene)
W_GRASS_TILE = 0xD535        # wGrassTile (validated live by the probe)
W_ENEMY_SPECIES = 0xCFE5     # wEnemyMonSpecies (internal index)
W_ENEMY_LEVEL = 0xCFF3       # wEnemyMonLevel
SPR1, SPR2 = 0xC100, 0xC200  # wSpriteStateData1/2 (16 slots x 16 bytes)
FACING_NAME = {0x0: "down", 0x4: "up", 0x8: "left", 0xC: "right"}
FACE_VAL = {v: k for k, v in FACING_NAME.items()}
OPPOSITE = {"up": "down", "down": "up", "left": "right", "right": "left"}

W_OVERWORLD, W_TILEMAP = 0xC6E8, 0xC3A0
SCREEN = 20 * 18

_WORLD: World | None = None


def world() -> World:
    global _WORLD
    if _WORLD is None:
        _WORLD = World()
    return _WORLD


# ------------------------------------------------------------------ tiny prims

def _tick(pb, n: int) -> None:
    for _ in range(n):
        pb.tick()


def pos(m) -> tuple[int, int]:
    return m[W_X], m[W_Y]


def facing(m) -> str:
    return FACING_NAME.get(m[0xC109] & 0xC, "down")


def font_up(m) -> bool:
    return bool(m[W_FONT] & 1)


def on_overworld(pb) -> bool:
    m = pb.memory
    w, h = m[W_W], m[W_H]
    if not (0 < w <= 64 and 0 < h <= 96):
        return False
    blocks = bytes(m[W_OVERWORLD:W_OVERWORLD + (w + 6) * (h + 6)])
    screen = bytes(m[W_TILEMAP:W_TILEMAP + SCREEN])
    return len(set(blocks)) > 1 and len(set(screen)) > 1


def npc_squares(m, skip: int = -1) -> list[tuple[int, int]]:
    """Live squares of the 15 NPC slots (slot 0 is the player)."""
    out = []
    for i in range(1, 16):
        if i == skip or m[SPR1 + i * 16] == 0:      # picture id 0 = empty slot
            continue
        out.append((m[SPR2 + i * 16 + 5] - 4, m[SPR2 + i * 16 + 4] - 4))
    return out


def npc_square(m, slot: int) -> tuple[int, int] | None:
    if not 1 <= slot <= 15 or m[SPR1 + slot * 16] == 0:
        return None
    return (m[SPR2 + slot * 16 + 5] - 4, m[SPR2 + slot * 16 + 4] - 4)


def snapshot(pb) -> dict:
    m = pb.memory
    return {"map": m[W_MAP], "x": m[W_X], "y": m[W_Y], "facing": facing(m),
            "battle": m[W_BATTLE], "last_map": m[W_LASTMAP],
            "text_box": font_up(m), "on_overworld": on_overworld(pb)}


def dismiss(pb, max_presses: int = 40) -> dict:
    """Clear text boxes: B (never selects / never buys / declines learning a
    move), with an A every 4th press for boxes that only advance on A. Stops
    instantly if a battle starts (a sign/NPC line can precede one)."""
    m = pb.memory
    n = 0
    while font_up(m) and n < max_presses:
        if m[W_BATTLE] != 0:
            return {"cleared": False, "battle": m[W_BATTLE], "presses": n}
        pb.button("a" if n % 4 == 3 else "b", delay=4)
        _tick(pb, 18)
        n += 1
    return {"cleared": not font_up(m), "presses": n}


# ------------------------------------------------------------------ stepping

def step(pb, d: str, jump: bool = False) -> dict:
    """ONE verified square step: press, then watch the console's own coords.
    Outcomes: moved (with pos) / blocked / battle / text / map (transition) /
    held (wJoyIgnore — a cutscene owns the controls)."""
    m = pb.memory
    map0, p0 = m[W_MAP], pos(m)
    if m[W_JOY_IGNORE]:
        for _ in range(240):                       # bounded wait for release
            pb.tick()
            if not m[W_JOY_IGNORE]:
                break
        if m[W_JOY_IGNORE]:
            return {"event": "held"}
    # Turn first, then a SHORT press. A square is 16 frames; a hold longer than
    # that starts a SECOND step before release — on a connection edge row that
    # second step walks clean off the map (found by the hunt probe, 2026-07-16).
    if facing(m) != d:
        pb.button(d, delay=3)
        _tick(pb, 10)
    pb.button(d, delay=12)
    budget = 110 if jump else 70
    moved = None
    for _ in range(budget):
        pb.tick()
        if m[W_BATTLE] != 0:
            return {"event": "battle", "battle": m[W_BATTLE]}
        if m[W_MAP] != map0:
            return {"event": "map", "map": m[W_MAP]}
        if pos(m) != p0:
            moved = pos(m)
            break
    if moved is None:
        if font_up(m):
            return {"event": "text"}
        return {"event": "blocked"}
    # settle: coords stable 10 frames (a ledge hop lands 2 squares out)
    stable = 0
    for _ in range(60):
        pb.tick()
        if m[W_BATTLE] != 0:
            return {"event": "battle", "battle": m[W_BATTLE]}
        if m[W_MAP] != map0:
            return {"event": "map", "map": m[W_MAP]}
        if pos(m) == moved:
            stable += 1
            if stable >= 10:
                break
        else:
            moved = pos(m)
            stable = 0
    return {"event": "moved", "pos": moved}


def _handle_battle(pb, kind: int, on_battle: str, on_trainer: str) -> dict:
    """Apply the battle policy mid-walk. Returns {'resume': bool, ...}."""
    policy = on_trainer if kind == 2 else on_battle
    if policy == "stop":
        return {"resume": False, "stopped_on": "battle", "battle": kind}
    if policy == "run" and kind != 2:
        r = battle_run(pb)
        return {"resume": r.get("ok", False), "battle_result": r}
    r = battle_mash(pb)
    return {"resume": r.get("ok", False), "battle_result": r}


def walk_to(pb, x: int, y: int, on_battle: str = "run", on_trainer: str = "stop",
            max_steps: int = 800, stop_at_warp: bool = False) -> dict:
    """In-map A* walk with live re-planning. Warp squares are BLOCKED unless they
    are the goal (walking over a door/ladder would teleport you); wandering NPCs
    are soft collision — bounded retries, then re-plan around their live squares.
    Ends: at the goal · on a map transition (stepping onto the goal warp square)
    · on a policy stop · or with the honest reason."""
    m = pb.memory
    w = world()
    goal = (x, y)
    events = []
    steps_done = 0
    goal_bumps = 0
    temp_blocked: set = set()
    while steps_done < max_steps:
        if m[W_BATTLE] != 0:
            hb = _handle_battle(pb, m[W_BATTLE], on_battle, on_trainer)
            events.append(hb)
            if not hb.get("resume"):
                return {"ok": False, "reason": "battle", **snapshot(pb),
                        "events": events}
            continue
        if font_up(m):
            dismiss(pb)
            continue
        cur = pos(m)
        if cur == goal:
            return {"ok": True, "steps": steps_done, **snapshot(pb),
                    "events": events}
        map_ind = m[W_MAP]
        entry = w.by_ind.get(map_ind)
        if entry is None or not w.walkable_map(entry):
            return {"ok": False, "reason": f"map {map_ind:#04x} not walkable",
                    **snapshot(pb), "events": events}
        grid = w.grid(map_ind)
        if not (cur[0] < grid.w2 and cur[1] < grid.h2):
            _settle_arrival(pb, map_ind)            # mid-transition read — wait
            continue
        warp_sqs = {(wp["x"], wp["y"]) for wp in entry.get("warpOut") or []}
        blocked = frozenset(set(npc_squares(m)) | temp_blocked
                            | (warp_sqs - {goal}))
        plan = astar(grid, cur, goal, blocked)
        if plan is None and temp_blocked:
            temp_blocked.clear()                    # a stale block may be the wall
            plan = astar(grid, cur, goal,
                         frozenset(set(npc_squares(m)) | (warp_sqs - {goal})))
        if plan is None:
            return {"ok": False, "reason": "no path", **snapshot(pb),
                    "events": events}
        replan = False
        for st in plan:
            r = step(pb, st["dir"], st.get("jump", False))
            steps_done += 1
            ev = r["event"]
            if ev == "moved":
                if tuple(r["pos"]) != tuple(st["to"]):
                    replan = True                   # slid / hopped elsewhere
                    break
                continue
            if ev == "map":
                return {"ok": tuple(st["to"]) == goal or stop_at_warp,
                        "reason": "map transition", **snapshot(pb),
                        "events": events}
            if ev == "battle":
                replan = True
                break
            if ev == "text":
                dismiss(pb)
                replan = True
                break
            if ev == "blocked":
                if tuple(st["to"]) == goal:
                    # a warp square that refuses to open (a solid doorway whose
                    # warp doesn't fire on the bump) — honest fail, fast
                    goal_bumps += 1
                    if goal_bumps >= 3:
                        return {"ok": False, "reason": "goal would not open",
                                **snapshot(pb), "events": events}
                # someone's in the square — wait for them to pass, then re-plan
                _tick(pb, 40)
                r2 = step(pb, st["dir"], st.get("jump", False))
                steps_done += 1
                if r2["event"] == "moved" and tuple(r2["pos"]) == tuple(st["to"]):
                    continue
                if tuple(st["to"]) != goal:
                    temp_blocked.add(tuple(st["to"]))
                replan = True
                break
            if ev == "held":
                return {"ok": False, "reason": "controls held (cutscene)",
                        **snapshot(pb), "events": events}
        if not replan and pos(m) == goal:
            return {"ok": True, "steps": steps_done, **snapshot(pb),
                    "events": events}
    return {"ok": False, "reason": "step budget exceeded", **snapshot(pb),
            "events": events}


# ------------------------------------------------------------------ warps & legs

def _settle_arrival(pb, expect_map: int | None = None, budget: int = 1200) -> None:
    """Wait out a map transition PROPERLY. ⚠️ wCurMap updates FIRST — dims,
    coords and the block buffer lag behind it (found on the Route 2 gate,
    2026-07-16: reading position right after the map byte flips hands back the
    OLD map's coordinates). Settled = the destination's own dims are in WRAM,
    the player's coords are inside them, the screen is real, controls are free."""
    m = pb.memory
    w = world()
    e = w.by_ind.get(expect_map) if expect_map is not None else None
    stable = 0
    for _ in range(budget):
        pb.tick()
        if font_up(m):
            dismiss(pb, 4)
        if m[W_JOY_IGNORE] or not on_overworld(pb):
            stable = 0
            continue
        if e is not None:
            if m[W_MAP] != expect_map or m[W_W] != e["width"] \
                    or m[W_H] != e["height"]:
                stable = 0
                continue
            if not (m[W_X] < 2 * e["width"] and m[W_Y] < 2 * e["height"]):
                stable = 0
                continue
        stable += 1
        if stable >= 20:
            break
    if font_up(m):
        dismiss(pb)


def _fire_warp(pb, at: tuple[int, int], to_map: int) -> bool:
    """Standing ON the warp square with no transition = a mat (fires on pressing
    into the edge — CheckWarpsCollision). Nudge toward the nearest edge first,
    then the other directions."""
    m = pb.memory
    g = world().grid(m[W_MAP])
    x, y = at
    order = sorted(DIRS, key=lambda d: {
        "up": y, "down": g.h2 - 1 - y, "left": x, "right": g.w2 - 1 - x}[d])
    for d in order:
        pb.button(d, delay=12)
        for _ in range(110):
            pb.tick()
            if m[W_MAP] == to_map:
                return True
        if pos(m) != at:                            # walked off the mat — back on
            walk_to(pb, *at)
    return m[W_MAP] == to_map


def goto(pb, dst, dst_x: int = -1, dst_y: int = -1, on_battle: str = "run",
         on_trainer: str = "stop", max_replans: int = 4) -> dict:
    """CROSS-MAP navigation: plan (warps + connections + walks), execute leg by
    leg, verify every transition against wCurMap, re-plan on surprises."""
    m = pb.memory
    w = world()
    entry = w.resolve(dst)
    dst_ind = entry["ind"]
    dst_pos = (dst_x, dst_y) if dst_x >= 0 and dst_y >= 0 else None
    trail = []
    bad_warps: set = set()                  # (map, (x,y)) that refused to open
    for _ in range(max_replans):
        if m[W_MAP] == dst_ind and (dst_pos is None or pos(m) == dst_pos):
            return {"ok": True, "trail": trail, **snapshot(pb)}
        legs = route(w, m[W_MAP], pos(m), dst_ind, dst_pos,
                     last_map=m[W_LASTMAP], avoid_warps=bad_warps)
        if legs is None:
            return {"ok": False, "reason": "no route", "trail": trail,
                    **snapshot(pb)}
        ok = True
        for leg in legs:
            if m[W_MAP] != leg["map"]:
                ok = False                          # world moved under us — replan
                break
            if leg["type"] == "walk":
                r = walk_to(pb, *leg["to"], on_battle=on_battle,
                            on_trainer=on_trainer)
                trail.append({"leg": "walk", "to": leg["to"], "ok": r["ok"]})
                if not r["ok"]:
                    return {"ok": False, "reason": r.get("reason"),
                            "trail": trail, **snapshot(pb)}
            elif leg["type"] == "warp":
                r = walk_to(pb, *leg["at"], on_battle=on_battle,
                            on_trainer=on_trainer, stop_at_warp=True)
                if not r["ok"] and r.get("reason") != "map transition":
                    trail.append({"leg": "warp", "at": leg["at"], "ok": False,
                                  "why": r.get("reason")})
                    if r.get("reason") in ("goal would not open", "no path"):
                        bad_warps.add((leg["map"], tuple(leg["at"])))
                        ok = False          # re-plan around this doorway
                        break
                    return {"ok": False, "reason": r.get("reason"),
                            "trail": trail, **snapshot(pb)}
                if m[W_MAP] == leg["map"]:          # on the mat, not yet warped
                    if not _fire_warp(pb, tuple(leg["at"]), leg["to_map"]):
                        trail.append({"leg": "warp", "at": leg["at"],
                                      "ok": False, "why": "would not fire"})
                        ok = False
                        break
                _settle_arrival(pb, leg["to_map"])
                good = m[W_MAP] == leg["to_map"]
                trail.append({"leg": "warp", "to_map": leg["to_map"],
                              "ok": good, "arrived": m[W_MAP]})
                if not good:
                    ok = False
                    break
            elif leg["type"] == "cross":
                cols = [tuple(c) for c in leg.get("columns") or [leg["at"]]]
                cols.sort(key=lambda c: abs(c[0] - pos(m)[0])
                          + abs(c[1] - pos(m)[1]))
                crossed = False
                for col in cols[:3]:
                    r = walk_to(pb, *col, on_battle=on_battle,
                                on_trainer=on_trainer)
                    if not r["ok"]:
                        continue
                    for _ in range(3):
                        pb.button(leg["btn"], delay=14)
                        for _ in range(110):
                            pb.tick()
                            if m[W_MAP] == leg["to_map"]:
                                break
                        if m[W_MAP] == leg["to_map"]:
                            break
                    if m[W_MAP] == leg["to_map"]:
                        crossed = True
                        break
                _settle_arrival(pb, leg["to_map"] if crossed else None)
                trail.append({"leg": "cross", "side": leg["side"],
                              "to_map": leg["to_map"], "ok": crossed})
                if not crossed:
                    ok = False
                    break
        if ok and m[W_MAP] == dst_ind:
            if dst_pos is not None and pos(m) != dst_pos:
                r = walk_to(pb, *dst_pos, on_battle=on_battle,
                            on_trainer=on_trainer)
                if not r["ok"]:
                    return {"ok": False, "reason": r.get("reason"),
                            "trail": trail, **snapshot(pb)}
            return {"ok": True, "trail": trail, **snapshot(pb)}
    return {"ok": False, "reason": "replans exhausted", "trail": trail,
            **snapshot(pb)}


# ------------------------------------------------------------------ battles

def _menu_taps(pb, dirs: list[str]) -> None:
    for d in dirs:
        pb.button(d, delay=4)
        _tick(pb, 10)


def battle_mash(pb, max_frames: int = 12000) -> dict:
    """A until the battle ends — first move, every text advanced. The blunt
    default; works for wild and trainer alike (win or faint, it ends)."""
    m = pb.memory
    f = 0
    while m[W_BATTLE] != 0 and f < max_frames:
        pb.button("a", delay=6)
        _tick(pb, 22)
        f += 28
    return {"ok": m[W_BATTLE] == 0, "frames": f, **snapshot(pb)}


def battle_run(pb, attempts: int = 12) -> dict:
    """Flee a WILD battle: pin the cursor at FIGHT (UP+LEFT), then DOWN+RIGHT =
    RUN, A. 'Can't escape!' just means try again (each attempt raises the odds).
    Trainer battles are refused with the honest reason."""
    m = pb.memory
    if m[W_BATTLE] == 2:
        return {"ok": False, "reason": "trainer battles can't be run from — "
                                       "use policy 'mash' or 'stop'"}
    for att in range(attempts):
        if m[W_BATTLE] == 0:
            return {"ok": True, "attempts": att, **snapshot(pb)}
        for _ in range(3):                          # clear any battle text first
            if not font_up(m):
                break
            pb.button("b", delay=4)
            _tick(pb, 18)
        _menu_taps(pb, ["b", "up", "left", "down", "right"])
        pb.button("a", delay=6)
        for _ in range(260):
            pb.tick()
            if m[W_BATTLE] == 0:
                break
        if m[W_BATTLE] == 0:
            _tick(pb, 40)
            return {"ok": True, "attempts": att + 1, **snapshot(pb)}
        dismiss(pb, 6)                              # "Can't escape!" box
    return {"ok": m[W_BATTLE] == 0, "attempts": attempts, **snapshot(pb)}


def battle_move(pb, slot: int = 1, max_turns: int = 40) -> dict:
    """Fight with move N (1-based) every turn until the battle ends. B declines
    move-learning prompts on level-up (never silently overwrites a moveset)."""
    m = pb.memory
    turns = 0
    while m[W_BATTLE] != 0 and turns < max_turns:
        for _ in range(4):                          # advance any pending text
            if not font_up(m):
                break
            pb.button("b", delay=4)
            _tick(pb, 18)
        _menu_taps(pb, ["up", "left"])              # pin at FIGHT
        pb.button("a", delay=6)
        _tick(pb, 30)
        _menu_taps(pb, ["up", "up", "up"])          # pin at move 1
        _menu_taps(pb, ["down"] * (max(1, slot) - 1))
        pb.button("a", delay=6)
        for _ in range(24):                         # the turn plays out
            _tick(pb, 30)
            if m[W_BATTLE] == 0:
                break
            if font_up(m):
                pb.button("b", delay=4)
        turns += 1
    return {"ok": m[W_BATTLE] == 0, "turns": turns, **snapshot(pb)}


def battle(pb, policy: str = "mash") -> dict:
    """Dispatch on policy: 'mash' | 'run' | 'move:N'."""
    m = pb.memory
    if m[W_BATTLE] == 0:
        return {"ok": False, "reason": "not in a battle"}
    if policy == "run":
        return battle_run(pb)
    if policy.startswith("move:"):
        return battle_move(pb, int(policy.split(":", 1)[1]))
    return battle_mash(pb)


# ------------------------------------------------------------------ hunting

def hunt(pb, max_steps: int = 240, policy: str = "stop") -> dict:
    """Pace until a wild battle starts. Prefers a shuttle whose BOTH squares are
    the map's own grass tile (wGrassTile, read live); caves encounter anywhere,
    so any passable shuttle does. Then applies the battle policy ('stop' hands
    the live battle back; the enemy species/level are reported either way)."""
    m = pb.memory
    w = world()
    entry = w.by_ind.get(m[W_MAP])
    if entry is None or not w.walkable_map(entry):
        return {"ok": False, "reason": "map not walkable"}
    grid = w.grid(m[W_MAP])
    grass = m[W_GRASS_TILE]
    warp_sqs = {(wp["x"], wp["y"]) for wp in entry.get("warpOut") or []}

    has_grass = 0 < grass < 255 and any(
        grid.feet[y2][x2] == grass and grid.passable[y2][x2]
        for y2 in range(grid.h2) for x2 in range(grid.w2))

    def shuttle_from(cur, allow_plain):
        """A grassy shuttle if one starts here; a plain one only when the map
        has no grass (caves encounter anywhere) or we've already relocated —
        pacing plain ground on a grass map would never encounter a thing."""
        best = None
        for d, (dx, dy) in DIRS.items():
            nx, ny = cur[0] + dx, cur[1] + dy
            if not grid.ok(nx, ny) or (nx, ny) in warp_sqs:
                continue
            if grid.ledges.get((grid.feet[cur[1]][cur[0]], d)):
                continue
            grassy = (grid.feet[ny][nx] == grass
                      and grid.feet[cur[1]][cur[0]] == grass)
            if grassy:
                return d
            best = best or d
        return best if allow_plain else None

    def pacing_spot(cur):
        """Nearest square that can host a shuttle — grassy pairs first (that is
        where encounters ARE on outdoor maps), any passable pair as fallback."""
        cands = []
        for y2 in range(grid.h2):
            for x2 in range(grid.w2):
                if not grid.passable[y2][x2] or (x2, y2) in warp_sqs:
                    continue
                mate = any(grid.ok(x2 + dx, y2 + dy)
                           and (x2 + dx, y2 + dy) not in warp_sqs
                           for dx, dy in ((0, 1), (0, -1), (1, 0), (-1, 0)))
                if not mate:
                    continue
                grassy = grid.feet[y2][x2] == grass and any(
                    grid.ok(x2 + dx, y2 + dy)
                    and grid.feet[y2 + dy][x2 + dx] == grass
                    for dx, dy in ((0, 1), (0, -1), (1, 0), (-1, 0)))
                cands.append((not grassy, abs(x2 - cur[0]) + abs(y2 - cur[1]),
                              (x2, y2)))
        cands.sort()
        return [c[2] for c in cands[:6]]

    steps = 0
    d = None
    relocations = 0
    while steps < max_steps:
        if m[W_BATTLE] != 0:
            enemy = {"species": m[W_ENEMY_SPECIES], "level": m[W_ENEMY_LEVEL],
                     "kind": m[W_BATTLE]}
            out = {"ok": True, "steps": steps, "enemy": enemy}
            if policy != "stop":
                out["battle_result"] = battle(pb, policy)
            return {**out, **snapshot(pb)}
        if font_up(m):
            dismiss(pb)
        cur = pos(m)
        d = d if d else shuttle_from(cur, relocations > 0 or not has_grass)
        if d is None:
            # boxed in / off the grass — move to the nearest pacing spot
            if relocations >= 2:
                return {"ok": False, "reason": "nowhere to pace", **snapshot(pb)}
            relocations += 1
            moved = False
            for spot in pacing_spot(cur):
                r = walk_to(pb, *spot, on_battle="stop", on_trainer="stop",
                            max_steps=300)
                if m[W_BATTLE] != 0:        # found one on the way — perfect
                    moved = True
                    break
                if r["ok"]:
                    moved = True
                    break
            if not moved:
                return {"ok": False, "reason": "nowhere to pace", **snapshot(pb)}
            continue
        r = step(pb, d)
        steps += 1
        if r["event"] == "moved":
            d = OPPOSITE[d]
        elif r["event"] == "battle":
            continue
        elif r["event"] in ("blocked", "text"):
            dismiss(pb, 6)
            d = None                                # pick a fresh direction
        elif r["event"] == "map":
            return {"ok": False, "reason": "walked off the map", **snapshot(pb)}
    return {"ok": False, "reason": "no encounter in budget", "steps": steps,
            **snapshot(pb)}


# ------------------------------------------------------------------ talk to NPC

def talk_to(pb, target, dismiss_text: bool = True,
            max_approaches: int = 12) -> dict:
    """Talk to an NPC — a MOVING target: chase its live square, stand 4-adjacent,
    face it, press A, and confirm the text box actually opened. `target` is a
    sprite slot (1..15) or the maps.json sprite name ('Bug Catcher'; first match).
    A trainer's line can start a battle — reported, never hidden."""
    m = pb.memory
    w = world()
    entry = w.by_ind.get(m[W_MAP])
    slot = None
    if isinstance(target, int) or str(target).isdigit():
        slot = int(target)
    elif entry is not None:
        for i, sp in enumerate(entry.get("sprites") or []):
            if sp.get("sprite", "").lower() == str(target).lower():
                slot = i + 1                        # sprites[0] lives in slot 1
                break
    if slot is None or not 1 <= slot <= 15:
        return {"ok": False, "reason": f"no sprite {target!r} on this map"}
    grid = w.grid(m[W_MAP])
    for _ in range(max_approaches):
        if m[W_BATTLE] != 0:
            return {"ok": False, "reason": "battle started", **snapshot(pb)}
        npc = npc_square(m, slot)
        if npc is None:
            return {"ok": False, "reason": f"sprite slot {slot} is empty/hidden"}
        cur = pos(m)
        dx, dy = npc[0] - cur[0], npc[1] - cur[1]
        if abs(dx) + abs(dy) == 1:                  # adjacent — face and speak
            d = ("right" if dx > 0 else "left") if dx else \
                ("down" if dy > 0 else "up")
            pb.button(d, delay=8)                   # a bump into an NPC just turns
            _tick(pb, 16)
            pb.button("a", delay=6)
            _tick(pb, 40)
            if font_up(m):
                out = {"ok": True, "slot": slot, "npc_at": list(npc)}
                if dismiss_text:
                    out["dismiss"] = dismiss(pb)
                    if out["dismiss"].get("battle"):
                        out["battle_started"] = True
                return {**out, **snapshot(pb)}
            if m[W_BATTLE] != 0:
                return {"ok": True, "slot": slot, "battle_started": True,
                        **snapshot(pb)}
            continue                                # it stepped away mid-press
        adjacents = [(npc[0] + ddx, npc[1] + ddy)
                     for ddx, ddy in ((0, 1), (0, -1), (1, 0), (-1, 0))
                     if grid.ok(npc[0] + ddx, npc[1] + ddy)]
        adjacents.sort(key=lambda a: abs(a[0] - cur[0]) + abs(a[1] - cur[1]))
        if not adjacents:
            return {"ok": False, "reason": "NPC unreachable (no open side)"}
        r = walk_to(pb, *adjacents[0], on_battle="run", on_trainer="stop",
                    max_steps=200)
        if not r["ok"] and r.get("reason") not in ("no path",):
            return {"ok": False, "reason": r.get("reason"), **snapshot(pb)}
    return {"ok": False, "reason": "couldn't corner the NPC", **snapshot(pb)}
