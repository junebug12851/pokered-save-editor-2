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

import json
from pathlib import Path

from navigate import (DIRS, ELEVATORS, SAFFRON_GATES, World, astar, distances,
                      route)

REPO = Path(__file__).resolve().parents[2]
CANON_FLAGS = REPO / "tmp" / "event-flags" / "event_flags_canonical.json"

# ------------------------------------------------------------------ WRAM map
W_MAP, W_H, W_W = 0xD35E, 0xD368, 0xD369
W_Y, W_X = 0xD361, 0xD362
W_BATTLE = 0xD057            # wIsInBattle: 0 no, 1 wild, 2 trainer
W_LASTMAP = 0xD365           # wLastMap (save 0x2611 + 0xAD54)
W_FONT = 0xCFC4              # wFontLoaded bit 0 = a text box is up
W_WALK_COUNTER = 0xCFC5      # wWalkCounter — nonzero while a step is IN FLIGHT
#   ⚠️ wXCoord/wYCoord update at step START (AdvancePlayerSprite), not at
#   completion — reading coords alone says "arrived" while the glide is still
#   running, and a button down at the glide boundary CHAINS a second step.
#   wWalkCounter == 0 is the console's own "the step is over".
W_MENU = 0xCC26              # wCurrentMenuItem
W_JOY_IGNORE = 0xCD6B        # nonzero = the game holds the controls (cutscene)
W_GRASS_TILE = 0xD535        # wGrassTile (validated live by the probe)
W_ENEMY_SPECIES = 0xCFE5     # wEnemyMonSpecies (internal index)
W_ENEMY_HP = 0xCFE6          # wEnemyMonHP (2 bytes, BIG-endian)
W_ENEMY_LEVEL = 0xCFF3       # wEnemyMonLevel
W_STATUS1 = 0xD728           # wStatusFlags1; bit 6 = BIT_GAVE_SAFFRON_GUARDS_DRINK
BIT_GUARD_DRINK = 6          # (pret scripts/Route5Gate.asm)
W_EVENT_FLAGS = 0xD747       # wEventFlags (2560 bits; save 0x29F3 + 0xAD54)
W_BAG_COUNT = 0xD31D         # wNumBagItems (save 0x25C9 + 0xAD54); pairs follow,
W_BAG_ITEMS = 0xD31E         #   0xFF-terminated — validated live by the probe
W_PARTY_COUNT = 0xD163       # wPartyCount; species list follows at 0xD164
W_PARTY_SPECIES = 0xD164
W_PARTY_MON = 0xD16B         # wPartyMon1 (44 bytes each): hp+1 BE, level +0x21,
MON_SIZE = 44                #   max hp +0x22 BE
W_MONEY = 0xD347             # wPlayerMoney, 3 bytes BCD
W_OPTIONS = 0xD355           # wOptions: b0-3 text speed, b6 battle style, b7 anim off
W_SHOP_LIST = 0xCF7B         # wItemList — the mart's for-sale ids, 0xFF-terminated
W_BOX_COUNT = 0xDA80         # wBoxCount (current box)
W_D74B = 0xD74B              # bit 5 = got the Pokédex (start-menu layout depends on it)
W_BIKESURF = 0xD700          # wWalkBikeSurfState: 0 walk, 1 bike, 2 surf
W_NUM_WARPS = 0xD3AF - 1     # wNumberOfWarps 0xD3AE
W_WARP_ENTRIES = 0xD3AF      # y, x, destWarpID, destMap per entry
ITEM_BICYCLE = 0x06          # pret constants/item_constants.asm
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


_FLAG_INDEX: dict | None = None


def flag_index(name_or_idx) -> int:
    """pret EVENT_* name -> bit index (the canonical import), or a raw index."""
    global _FLAG_INDEX
    if isinstance(name_or_idx, int) or str(name_or_idx).isdigit():
        return int(name_or_idx)
    if _FLAG_INDEX is None:
        _FLAG_INDEX = {r["name"]: r["index"]
                       for r in json.loads(CANON_FLAGS.read_text(encoding="utf-8"))
                       if r.get("name")} if CANON_FLAGS.exists() else {}
    if name_or_idx not in _FLAG_INDEX:
        raise KeyError(f"unknown event flag {name_or_idx!r}")
    return _FLAG_INDEX[name_or_idx]


def set_event_flag(pb, name_or_idx, on: bool = True) -> dict:
    """Set/clear one wEventFlags bit in LIVE WRAM (dev tooling — reported,
    never silent)."""
    idx = flag_index(name_or_idx)
    addr = W_EVENT_FLAGS + idx // 8
    m = pb.memory
    before = m[addr]
    m[addr] = (before | (1 << (idx % 8))) if on \
        else (before & ~(1 << (idx % 8)) & 0xFF)
    return {"flag": str(name_or_idx), "index": idx, "on": on,
            "byte_before": before, "byte_after": m[addr]}


def set_guard_drink(pb) -> dict:
    """wStatusFlags1 bit 6 = BIT_GAVE_SAFFRON_GUARDS_DRINK — the thirsty
    Saffron gate guards step aside."""
    m = pb.memory
    before = m[W_STATUS1]
    m[W_STATUS1] = before | (1 << BIT_GUARD_DRINK)
    return {"guard_drink": True, "was_set": bool(before & (1 << BIT_GUARD_DRINK))}


def give_item(pb, item_id: int, qty: int = 1) -> dict:
    """Put an item in the LIVE bag (bump qty if present; append + re-terminate
    if not; 20-slot cap honored)."""
    m = pb.memory
    n = m[W_BAG_COUNT]
    for i in range(min(n, 20)):
        if m[W_BAG_ITEMS + 2 * i] == item_id:
            m[W_BAG_ITEMS + 2 * i + 1] = min(99, m[W_BAG_ITEMS + 2 * i + 1] + qty)
            return {"item": item_id, "qty": m[W_BAG_ITEMS + 2 * i + 1],
                    "slot": i, "added": False}
    if n >= 20:
        return {"error": "bag full"}
    m[W_BAG_ITEMS + 2 * n] = item_id
    m[W_BAG_ITEMS + 2 * n + 1] = min(99, qty)
    m[W_BAG_ITEMS + 2 * n + 2] = 0xFF
    m[W_BAG_COUNT] = n + 1
    return {"item": item_id, "qty": qty, "slot": n, "added": True}


def move_sprite(pb, slot: int, x: int, y: int) -> dict:
    """Relocate an NPC/boulder sprite in LIVE WRAM (StateData2 map coords,
    stored +4). The Strength-boulder lever: put a rock where it's needed —
    reported, never silent."""
    m = pb.memory
    if not 1 <= slot <= 15 or m[SPR1 + slot * 16] == 0:
        return {"error": f"sprite slot {slot} is empty"}
    before = npc_square(m, slot)
    m[SPR2 + slot * 16 + 4] = (y + 4) & 0xFF
    m[SPR2 + slot * 16 + 5] = (x + 4) & 0xFF
    return {"slot": slot, "from": list(before), "to": [x, y]}


def _screen_anchor_ok(pb, grid) -> bool:
    """The player's square renders at screen tiles (8..9, 8..9); his FEET tile
    is wTileMap[9*20+8]. Verified against our own grid before any tile poke."""
    m = pb.memory
    px, py = pos(m)
    if not (px < grid.w2 and py < grid.h2):
        return False
    return m[0xC3A0 + 9 * 20 + 8] == grid.feet[py][px]


def clear_tree(pb, grid, sq) -> dict:
    """CUT, without the ceremony: replace the tree's whole block with its cutTo
    block (our own imported cutTreeBlocks data) in the live block buffer, and
    re-tile the on-screen part of that block so collision (which reads the
    screen buffer) agrees. Reported, never silent."""
    tx, ty = sq
    cut_to = grid.cuttable.get((tx, ty))
    if cut_to is None:
        return {"error": f"square {sq} is not a cuttable tree"}
    m = pb.memory
    w_blocks = grid.entry["width"]
    bx, by = tx // 2, ty // 2
    m[0xC6E8 + (by + 3) * (w_blocks + 6) + (bx + 3)] = cut_to
    tiles_poked = 0
    if _screen_anchor_ok(pb, grid):
        px, py = pos(m)
        for qy in (by * 2, by * 2 + 1):          # the block's four squares
            for qx in (bx * 2, bx * 2 + 1):
                sx, sy = 8 + 2 * (qx - px), 8 + 2 * (qy - py)
                if not (0 <= sx <= 18 and 0 <= sy <= 16):
                    continue
                r, c = (qy % 2) * 2, (qx % 2) * 2
                for i in (0, 1):
                    for j in (0, 1):
                        m[0xC3A0 + (sy + i) * 20 + (sx + j)] = \
                            grid.bst[cut_to * 16 + (r + i) * 4 + (c + j)]
                        tiles_poked += 1
    # our own grid must agree with what the console now holds
    for qy in (by * 2, by * 2 + 1):
        for qx in (bx * 2, bx * 2 + 1):
            if 0 <= qx < grid.w2 and 0 <= qy < grid.h2:
                tile = grid.bst[cut_to * 16
                                + ((qy % 2) * 2 + 1) * 4 + (qx % 2) * 2]
                grid.feet[qy][qx] = tile
                grid.block[qy][qx] = cut_to
                grid.passable[qy][qx] = True
                grid.cuttable.pop((qx, qy), None)
    return {"cut": [tx, ty], "block_to": cut_to, "screen_tiles": tiles_poked}


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
        # a spinner slide chains arrows for a long time — wait it out as long
        # as the player keeps MOVING; only a frozen hold is a real cutscene
        frozen, last = 0, pos(m)
        for _ in range(2400):
            pb.tick()
            if not m[W_JOY_IGNORE]:
                break
            if pos(m) != last:
                last, frozen = pos(m), 0
            else:
                frozen += 1
                if frozen >= 600:
                    return {"event": "held"}
        if m[W_JOY_IGNORE]:
            return {"event": "held"}
        p0 = pos(m)
    # Turn first, then a SHORT press. A square glides 16 frames and a button
    # still down at the glide boundary CHAINS a second step (found twice: the
    # hunt probe walked off a connection edge; the Route 4 cave approach
    # coasted past its goal AFTER walk_to returned) — so the press is 8 frames
    # and the settle below waits out wWalkCounter, the console's own signal.
    if facing(m) != d:
        pb.button(d, delay=3)
        _tick(pb, 10)
    pb.button(d, delay=8)
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
    # settle: the step is over when the console says so — wWalkCounter == 0
    # (coords flip at step START; see the constant's warning) — and coords hold
    # still with the controls free. The long budget is for FORCED MOVEMENT —
    # a spinner/arrow tile slides the player many squares; follow it to rest.
    stable = 0
    for _ in range(900):
        pb.tick()
        if m[W_BATTLE] != 0:
            return {"event": "battle", "battle": m[W_BATTLE]}
        if m[W_MAP] != map0:
            return {"event": "map", "map": m[W_MAP]}
        if pos(m) == moved and m[W_WALK_COUNTER] == 0:
            stable += 1
            if stable >= 8 and not m[W_JOY_IGNORE]:
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
    r = battle_sweep(pb) if policy == "sweep" else battle_mash(pb)
    return {"resume": r.get("ok", False), "battle_result": r}


def walk_to(pb, x: int, y: int, on_battle: str = "run", on_trainer: str = "stop",
            max_steps: int = 800, stop_at_warp: bool = False,
            surf: bool = False, cut: bool = False) -> dict:
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
        plan = astar(grid, cur, goal, blocked, surf=surf, cut=cut)
        if plan is None and temp_blocked:
            temp_blocked.clear()                    # a stale block may be the wall
            plan = astar(grid, cur, goal,
                         frozenset(set(npc_squares(m)) | (warp_sqs - {goal})),
                         surf=surf, cut=cut)
        if plan is None:
            return {"ok": False, "reason": "no path", **snapshot(pb),
                    "events": events}
        replan = False
        for st in plan:
            tgt = tuple(st["to"])
            if cut and tgt in grid.cuttable:
                events.append(clear_tree(pb, grid, tgt))     # CUT, reported
            ty_, tx_ = tgt[1], tgt[0]
            if surf and tx_ < grid.w2 and ty_ < grid.h2 \
                    and grid.water[ty_][tx_] and m[W_BIKESURF] != 2:
                m[W_BIKESURF] = 2                            # mount, reported
                events.append({"surfed": True, "at": list(pos(m))})
            r = step(pb, st["dir"], st.get("jump", False))
            steps_done += 1
            if m[W_BIKESURF] == 2 and r.get("event") == "moved":
                lx, ly = r["pos"]
                if lx < grid.w2 and ly < grid.h2 and not grid.water[ly][lx]:
                    m[W_BIKESURF] = 0                        # ashore — dismount
                    events.append({"dismounted": True, "at": [lx, ly]})
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


_CYCLING: set | None = None


def _cycling_maps(w: World) -> set:
    """Routes 16-18 + their gates — the maps whose guard wants a BICYCLE."""
    global _CYCLING
    if _CYCLING is None:
        _CYCLING = set()
        for name in ("Route 16", "Route 17", "Route 18",
                     "Route 16 Gate 1F", "Route 16 Gate 2F",
                     "Route 18 Gate 1F", "Route 18 Gate 2F",
                     "Route 16 Gate", "Route 18 Gate"):
            try:
                _CYCLING.add(w.resolve(name)["ind"])
            except KeyError:
                continue
    return _CYCLING


def _ride_elevator(pb, leg) -> bool:
    """Ride: re-aim the car's own door-warp entries at the chosen floor in live
    WRAM (what the floor-menu script itself does), then step out the door."""
    m = pb.memory
    doors = {(dq[0], dq[1]) for dq in leg["doors"]}
    hit = False
    for i in range(m[W_NUM_WARPS]):
        ey = m[W_WARP_ENTRIES + 4 * i]
        ex = m[W_WARP_ENTRIES + 4 * i + 1]
        if (ex, ey) in doors:
            m[W_WARP_ENTRIES + 4 * i + 2] = leg["dest_warp"]
            m[W_WARP_ENTRIES + 4 * i + 3] = leg["to_map"]
            hit = True
    if not hit:
        return False
    door = min(doors, key=lambda dq: abs(dq[0] - pos(m)[0])
               + abs(dq[1] - pos(m)[1]))
    r = walk_to(pb, *door, stop_at_warp=True, max_steps=60)
    if m[W_MAP] == leg["map"]:
        if not r["ok"] and r.get("reason") != "map transition":
            return False
        _fire_warp(pb, door, leg["to_map"])
    return m[W_MAP] == leg["to_map"]


def goto(pb, dst, dst_x: int = -1, dst_y: int = -1, on_battle: str = "run",
         on_trainer: str = "stop", max_replans: int = 4,
         unblock: bool = True, bike: bool = True,
         surf: str = "auto", cut: str = "auto") -> dict:
    """CROSS-MAP navigation: plan (warps + connections + elevators + walks),
    execute leg by leg, verify every transition against wCurMap, re-plan on
    surprises. Progression aids, all REPORTED in `prep`, all opt-out:
      unblock  — Saffron gate on the route -> set the guard-drink bit
      bike     — Cycling Road on the route -> a BICYCLE appears in the bag
      surf/cut — 'auto' plans dry first and only opens water / cuttable trees
                 when no dry route exists ('always' / 'never' to force)."""
    m = pb.memory
    w = world()
    entry = w.resolve(dst)
    dst_ind = entry["ind"]
    dst_pos = (dst_x, dst_y) if dst_x >= 0 and dst_y >= 0 else None
    trail = []
    prep = []
    bad_warps: set = set()                  # (map, (x,y)) that refused to open
    use_surf = surf == "always"
    use_cut = cut == "always"
    for _ in range(max_replans):
        if m[W_MAP] == dst_ind and (dst_pos is None or pos(m) == dst_pos):
            return {"ok": True, "trail": trail, "prep": prep, **snapshot(pb)}
        legs = route(w, m[W_MAP], pos(m), dst_ind, dst_pos,
                     last_map=m[W_LASTMAP], avoid_warps=bad_warps,
                     surf=use_surf, cut=use_cut)
        if legs is None and cut == "auto" and not use_cut:
            use_cut = True                  # no dry route — try the smaller
            legs = route(w, m[W_MAP], pos(m), dst_ind, dst_pos,   # intervention
                         last_map=m[W_LASTMAP], avoid_warps=bad_warps,
                         surf=use_surf, cut=True)                 # (cut) first
            if legs is not None:
                prep.append({"cut": "trees on the only route — will clear them"})
        if legs is None and surf == "auto" and not use_surf:
            use_surf = True                 # still nothing — open the water
            legs = route(w, m[W_MAP], pos(m), dst_ind, dst_pos,
                         last_map=m[W_LASTMAP], avoid_warps=bad_warps,
                         surf=True, cut=use_cut)
            if legs is not None:
                prep.append({"surf": "no dry route — water opened"})
        if legs is None:
            return {"ok": False, "reason": "no route", "trail": trail,
                    "prep": prep, **snapshot(pb)}
        touched = {leg["map"] for leg in legs} \
            | {leg.get("to_map") for leg in legs if leg.get("to_map") is not None}
        if unblock and touched & SAFFRON_GATES \
                and not (m[W_STATUS1] & (1 << BIT_GUARD_DRINK)):
            set_guard_drink(pb)
            prep.append({"guard_drink": "Saffron gate on the route — "
                                        "the guard steps aside"})
        if bike and touched & _cycling_maps(w):
            r = give_item(pb, ITEM_BICYCLE)
            if r.get("added"):
                prep.append({"bicycle": "Cycling Road on the route — "
                                        "a BICYCLE is in the bag"})
        ok = True
        for leg in legs:
            if m[W_MAP] != leg["map"]:
                ok = False                          # world moved under us — replan
                break
            if leg["type"] == "elevator":
                rode = _ride_elevator(pb, leg)
                if rode:
                    _settle_arrival(pb, leg["to_map"])
                trail.append({"leg": "elevator", "to_map": leg["to_map"],
                              "ok": rode})
                if not rode:
                    ok = False
                    break
                continue
            if leg["type"] == "walk":
                r = walk_to(pb, *leg["to"], on_battle=on_battle,
                            on_trainer=on_trainer, surf=use_surf, cut=use_cut)
                trail.append({"leg": "walk", "to": leg["to"], "ok": r["ok"],
                              **({"events": r["events"]} if r.get("events") else {})})
                if not r["ok"]:
                    return {"ok": False, "reason": r.get("reason"),
                            "trail": trail, "prep": prep, **snapshot(pb)}
            elif leg["type"] == "warp":
                r = walk_to(pb, *leg["at"], on_battle=on_battle,
                            on_trainer=on_trainer, stop_at_warp=True,
                            surf=use_surf, cut=use_cut)
                if not r["ok"] and r.get("reason") != "map transition":
                    trail.append({"leg": "warp", "at": leg["at"], "ok": False,
                                  "why": r.get("reason")})
                    if r.get("reason") in ("goal would not open", "no path"):
                        bad_warps.add((leg["map"], tuple(leg["at"])))
                        ok = False          # re-plan around this doorway
                        break
                    return {"ok": False, "reason": r.get("reason"),
                            "trail": trail, "prep": prep, **snapshot(pb)}
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
                                on_trainer=on_trainer,
                                surf=use_surf, cut=use_cut)
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
                            on_trainer=on_trainer, surf=use_surf, cut=use_cut)
                if not r["ok"]:
                    return {"ok": False, "reason": r.get("reason"),
                            "trail": trail, "prep": prep, **snapshot(pb)}
            return {"ok": True, "trail": trail, "prep": prep, **snapshot(pb)}
    return {"ok": False, "reason": "replans exhausted", "trail": trail,
            "prep": prep, **snapshot(pb)}


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


def battle_sweep(pb, max_frames: int = 24000) -> dict:
    """WIN, when asked: hold the enemy's HP at 1 in live WRAM and attack —
    every enemy mon (a trainer's whole team) falls to the next hit. B every
    third press declines move-learning prompts. The poke is the point (dev
    tooling for 'progress the story now'), and it is reported, never hidden."""
    m = pb.memory
    f = 0
    pokes = 0
    n = 0
    while m[W_BATTLE] != 0 and f < max_frames:
        hp = (m[W_ENEMY_HP] << 8) | m[W_ENEMY_HP + 1]
        if hp > 1:
            m[W_ENEMY_HP] = 0
            m[W_ENEMY_HP + 1] = 1
            pokes += 1
        pb.button("b" if n % 3 == 2 else "a", delay=6)
        _tick(pb, 22)
        f += 28
        n += 1
    return {"ok": m[W_BATTLE] == 0, "frames": f, "hp_pokes": pokes,
            **snapshot(pb)}


def battle(pb, policy: str = "mash") -> dict:
    """Dispatch on policy: 'mash' | 'run' | 'move:N' | 'sweep' (a certain win —
    enemy HP held at 1 in WRAM; reported)."""
    m = pb.memory
    if m[W_BATTLE] == 0:
        return {"ok": False, "reason": "not in a battle"}
    if policy == "run":
        return battle_run(pb)
    if policy == "sweep":
        return battle_sweep(pb)
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

# ------------------------------------------------------------------ menus
# Everything below drives REAL menus with the cursor read back from
# wCurrentMenuItem after every tap — never a blind button script.

def _tap(pb, b: str, wait: int = 16) -> None:
    pb.button(b, delay=5)
    _tick(pb, wait)


def party_level(m, slot: int = 1) -> int:
    return m[W_PARTY_MON + (slot - 1) * MON_SIZE + 0x21]


def party_hp(m, slot: int = 1) -> tuple[int, int]:
    b = W_PARTY_MON + (slot - 1) * MON_SIZE
    return ((m[b + 1] << 8) | m[b + 2], (m[b + 0x22] << 8) | m[b + 0x23])


def party_species(m) -> list[int]:
    return [m[W_PARTY_SPECIES + i] for i in range(m[W_PARTY_COUNT])]


def money(m) -> int:
    v = 0
    for i in range(3):
        b = m[W_MONEY + i]
        v = v * 100 + (b >> 4) * 10 + (b & 0xF)
    return v


def bag_qty(m, item_id: int) -> int:
    for i in range(min(m[W_BAG_COUNT], 20)):
        if m[W_BAG_ITEMS + 2 * i] == item_id:
            return m[W_BAG_ITEMS + 2 * i + 1]
    return 0


def _cursor_to(pb, idx: int, tries: int = 24) -> bool:
    """Move wCurrentMenuItem to idx, verified after every tap."""
    m = pb.memory
    for _ in range(tries):
        cur = m[W_MENU]
        if cur == idx:
            return True
        _tap(pb, "down" if cur < idx else "up", 14)
    return m[W_MENU] == idx


def _menu_live(pb) -> bool:
    """Is a MENU actually taking input right now? Probe-tap: a live menu moves
    wCurrentMenuItem on a down/up tap (restored); printing text does not.
    The one signal that cannot go stale."""
    m = pb.memory
    cur = m[W_MENU]
    _tap(pb, "down", 14)
    if m[W_MENU] != cur:
        _tap(pb, "up", 14)
        return True
    if cur > 0:                                 # maybe parked at the bottom
        _tap(pb, "up", 14)
        if m[W_MENU] != cur:
            _tap(pb, "down", 14)
            return True
    return False


def _await_menu(pb, max_a: int = 6) -> bool:
    """Advance intermediate text ('Accessed BILL's PC.' …) with A until a live
    menu answers the probe-tap."""
    for _ in range(max_a):
        if _menu_live(pb):
            return True
        _tap(pb, "a", 50)
    return _menu_live(pb)


# ⚠️ REJECTED: a wMaxMenuItem (0xCC28) "sentinel poke" as a fresh-menu
# detector. The byte is READ while a menu is active — poking it unclamps the
# cursor (down moves past the last entry into garbage rows) and an A there
# dispatches wild (it zeroed a party once). Menus are synchronized by the
# probe-tap (_menu_live) only; commits are verified by their outcome.


def start_menu_items(m) -> list[str]:
    """The start menu's layout depends on progress (pret DisplayStartMenu):
    POKéDEX only once obtained (wd74b bit 5), POKéMON only with a party."""
    items = []
    if m[W_D74B] & (1 << 5):
        items.append("POKEDEX")
    if m[W_PARTY_COUNT]:
        items.append("POKEMON")
    items += ["ITEM", "TRAINER", "SAVE", "OPTION", "EXIT"]
    return items


def start_select(pb, item: str) -> dict:
    """Open the start menu and select an entry by NAME. The menu must answer
    the probe-tap (LIVE, not just 'a text box is up') before the cursor moves
    — a stale wCurrentMenuItem happily 'verifies' against no menu at all."""
    m = pb.memory
    close_menus(pb, 10)                         # a clean overworld first
    for _ in range(20):
        if not font_up(m):
            break
        _tap(pb, "b", 20)
    items = start_menu_items(m)
    if item not in items:
        return {"ok": False, "reason": f"{item} not in start menu {items}"}
    opened = False
    for _ in range(5):
        _tap(pb, "start", 30)
        if font_up(m) and _menu_live(pb):
            opened = True
            break
    if not opened:
        return {"ok": False, "reason": "start menu never went live"}
    if not _cursor_to(pb, items.index(item)):
        return {"ok": False, "reason": "cursor would not settle"}
    _tap(pb, "a", 30)
    return {"ok": True, "items": items}


def close_menus(pb, presses: int = 6) -> None:
    m = pb.memory
    for _ in range(presses):
        if not font_up(m):
            break
        _tap(pb, "b", 20)


def save_game(pb) -> dict:
    """Start → SAVE → YES; waits out the 'SAVING...' write. Verified by the
    flow completing back to a free overworld (the battery file itself is only
    flushed by PyBoy at stop)."""
    r = start_select(pb, "SAVE")
    if not r["ok"]:
        return r
    _tap(pb, "a", 40)                           # YES (default) on the confirm
    m = pb.memory
    for _ in range(900):                        # the save text + write
        pb.tick()
        if not font_up(m) and not m[W_JOY_IGNORE]:
            break
    close_menus(pb)
    return {"ok": on_overworld(pb) and not font_up(m), **snapshot(pb)}


def set_options(pb, text_fast: bool = True, anim_off: bool = True,
                style_set: bool = True) -> dict:
    """Start → OPTION and set the three rows with real taps (left/right),
    verified against the wOptions byte afterwards."""
    r = start_select(pb, "OPTION")
    if not r["ok"]:
        return r
    m = pb.memory
    _tick(pb, 40)
    # The options screen keeps its own cursor variables (wCurrentMenuItem is
    # NOT the row) — walk the three rows top-down from the known opening state
    # (TEXT SPEED first), verifying each against the wOptions byte itself.
    def _want(o):
        return bool((not text_fast or (o & 0xF) == 1)
                    and (not anim_off or o & 0x80)
                    and (not style_set or o & 0x40))
    for _ in range(2):                          # two full top-down passes
        if text_fast:
            for _ in range(3):
                if (m[W_OPTIONS] & 0xF) == 1:
                    break
                _tap(pb, "left", 16)
        _tap(pb, "down", 16)
        if anim_off:
            for _ in range(3):
                if m[W_OPTIONS] & 0x80:
                    break
                _tap(pb, "right", 16)
        _tap(pb, "down", 16)
        if style_set:
            for _ in range(3):
                if m[W_OPTIONS] & 0x40:
                    break
                _tap(pb, "right", 16)
        if _want(m[W_OPTIONS]):
            break
        _tap(pb, "down", 16)                    # wrap past CANCEL back to row 0
    poked = False
    if not _want(m[W_OPTIONS]):                 # the screen misbehaved — poke
        o = m[W_OPTIONS]                        # the remainder, SAY SO
        if text_fast:
            o = (o & 0xF0) | 1
        if anim_off:
            o |= 0x80
        if style_set:
            o |= 0x40
        m[W_OPTIONS] = o
        poked = True
    _tap(pb, "b", 30)
    close_menus(pb)
    o = m[W_OPTIONS]
    return {"ok": _want(o), "wOptions": o, "poked": poked, **snapshot(pb)}


def party_swap(pb, a: int, b: int, retries: int = 2) -> dict:
    """Start → POKéMON → mon a → SWITCH → mon b (all cursor-verified);
    proven by the species order actually changing (retried whole if not)."""
    m = pb.memory
    before = party_species(m)
    if not (1 <= a <= len(before) and 1 <= b <= len(before)) or a == b:
        return {"ok": False, "reason": f"bad slots for party of {len(before)}"}
    want = list(before)
    want[a - 1], want[b - 1] = want[b - 1], want[a - 1]
    last = ""
    for _ in range(retries):
        close_menus(pb, 8)
        _tick(pb, 30)
        r = start_select(pb, "POKEMON")
        if not r["ok"]:
            last = str(r.get("reason"))
            continue
        if not _await_menu(pb):
            last = "party list never went live"
            continue
        if not _cursor_to(pb, a - 1):
            last = "party cursor stuck"
            continue
        _tap(pb, "a", 60)
        if not _await_menu(pb):
            last = "submenu never went live"
            continue
        if not _cursor_to(pb, 1):               # SWITCH
            last = "submenu cursor stuck"
            continue
        _tap(pb, "a", 60)
        if not _await_menu(pb):
            last = "switch list never went live"
            continue
        if not _cursor_to(pb, b - 1):
            last = "second cursor stuck"
            continue
        _tap(pb, "a", 50)
        close_menus(pb)
        _tick(pb, 20)
        if party_species(m) == want:
            return {"ok": True, "before": before, "after": party_species(m)}
        last = "order unchanged"
    return {"ok": party_species(m) == want, "before": before,
            "after": party_species(m), "why": last}


# ------------------------------------------- Pokémon Center / Mart / PC

def _interact_over_counter(pb, npc_sq, prefer=("up", "left", "right", "down")):
    """Stand across the counter from an NPC (2 squares away — counters relay
    the A press) or adjacent, face them, press A. True once text opens."""
    m = pb.memory
    w = world()
    grid = w.grid(m[W_MAP])
    nx, ny = npc_sq
    spots = []
    for d in prefer:
        dx, dy = DIRS[d]
        for dist in (2, 1):
            sx, sy = nx + dx * dist, ny + dy * dist
            if grid.ok(sx, sy):
                spots.append(((sx, sy), OPPOSITE[d]))
    for spot, face_dir in spots[:6]:
        r = walk_to(pb, *spot, max_steps=120)
        if not r["ok"]:
            continue
        pb.button(face_dir, delay=8)
        _tick(pb, 16)
        _tap(pb, "a", 40)
        if font_up(m):
            return True
    return False


def heal_at_center(pb) -> dict:
    """Talk to the nurse over the counter, A through the heal (YES is the
    default), verified by every party mon reading HP == max HP."""
    m = pb.memory
    w = world()
    entry = w.by_ind.get(m[W_MAP])
    slot = next((i + 1 for i, sp in enumerate(entry.get("sprites") or [])
                 if "nurse" in sp.get("sprite", "").lower()), 1)
    npc = npc_square(m, slot)
    if npc is None:
        return {"ok": False, "reason": "no nurse here — is this a Pokémon Center?"}
    if not _interact_over_counter(pb, npc):
        return {"ok": False, "reason": "couldn't reach the counter"}
    for _ in range(80):                         # the heal jingle + text
        if not font_up(m) and not m[W_JOY_IGNORE]:
            break
        _tap(pb, "a", 24)
    healed = all(party_hp(m, s + 1)[0] == party_hp(m, s + 1)[1]
                 for s in range(m[W_PARTY_COUNT]))
    return {"ok": healed and on_overworld(pb), "party": m[W_PARTY_COUNT],
            **snapshot(pb)}


def mart_buy(pb, item_id: int, qty: int = 1) -> dict:
    """Talk to the clerk, BUY, pick the item out of the LIVE shop list
    (wItemList), set the quantity, confirm — verified by the bag gaining
    exactly qty and the money going down."""
    m = pb.memory
    w = world()
    entry = w.by_ind.get(m[W_MAP])
    slot = next((i + 1 for i, sp in enumerate(entry.get("sprites") or [])
                 if sp.get("sprite", "").lower() in ("clerk", "mart guy",
                                                     "cashier")), 1)
    npc = npc_square(m, slot)
    if npc is None:
        return {"ok": False, "reason": "no clerk here — is this a mart?"}
    qty_before, money_before = bag_qty(m, item_id), money(m)
    if not _interact_over_counter(pb, npc, prefer=("right", "up", "left", "down")):
        return {"ok": False, "reason": "couldn't reach the counter"}
    if not _await_menu(pb):                     # BUY / SELL / QUIT
        return {"ok": False, "reason": "shop menu never went live"}
    if not _cursor_to(pb, 0):
        return {"ok": False, "reason": "shop menu cursor stuck"}
    _tap(pb, "a", 70)
    # reach the item LIST; if a stray A armed the qty prompt (wMaxItemQuantity
    # 0xCF97 == 99 — write-only for the list, so pre-cleared as a SAFE
    # sentinel), back out with B before probing (a probe DOWN in the prompt
    # wraps 1 -> x99; it bought 99 Poké Balls once)
    m[0xCF97] = 0
    for _ in range(6):
        if m[0xCF97] == 0x63:
            _tap(pb, "b", 30)
            m[0xCF97] = 0
            continue
        if _menu_live(pb):
            break
        _tap(pb, "a", 50)
    # wItemList is COUNT-PREFIXED: [n, id, id, ..., 0xFF]
    n_stock = m[W_SHOP_LIST]
    stock = [m[W_SHOP_LIST + 1 + i] for i in range(min(n_stock, 10))
             if m[W_SHOP_LIST + 1 + i] != 0xFF]
    if item_id not in stock:
        close_menus(pb, 8)
        return {"ok": False, "reason": f"item {item_id} not in stock {stock}"}
    # a long list SCROLLS: the absolute selection is cursor + scroll offset
    target = stock.index(item_id)
    for _ in range(24):
        absolute = m[W_MENU] + m[0xCC36]        # wCurrentMenuItem + wListScrollOffset
        if absolute == target:
            break
        _tap(pb, "down" if absolute < target else "up", 14)
    m[0xCF97] = 0                               # sentinel the qty prompt too
    _tap(pb, "a", 40)
    for _ in range(300):                        # the quantity prompt arms
        if m[0xCF97] == 0x63:                   # wMaxItemQuantity = 99, FRESH
            break
        pb.tick()
    want_q = min(qty, 99)
    for _ in range(want_q * 2 + 4):             # wItemQuantity (0xCF96),
        cur_q = m[0xCF96]                       # verified per tap, both ways
        if cur_q == want_q:
            break
        _tap(pb, "up" if cur_q < want_q else "down", 24)
    _tap(pb, "a", 30)                           # ask the price
    for _ in range(14):                         # the quote prints, then YES —
        if money(m) < money_before:             # the MONEY dropping is the
            break                               # only honest confirmation
        _tap(pb, "a", 30)
    close_menus(pb, 10)
    gained = bag_qty(m, item_id) - qty_before
    return {"ok": gained == qty and money(m) < money_before,
            "gained": gained, "spent": money_before - money(m), **snapshot(pb)}


def _pc_open(pb) -> bool:
    """The Center PC is a HIDDEN EVENT, not a tile: pret data/events/
    hidden_events.asm gives every Pokécenter `hidden_event 13, 3,
    OpenPokemonCenterPC, SPRITE_FACING_UP` — stand at (13,4), face up, A.
    Success = a text box WITH a menu (wMaxMenuItem ≥ 2), not mere dialog."""
    m = pb.memory
    close_menus(pb, 6)                          # a fresh start (post-heal text)
    _tick(pb, 30)
    for sq in ((13, 4), (13, 4), (12, 4)):      # first spot gets two tries
        r = walk_to(pb, *sq, max_steps=120)
        if not r["ok"]:
            continue
        pb.button("up", delay=8)
        _tick(pb, 20)
        _tap(pb, "a", 60)
        if font_up(m) and m[0xCC28] >= 2:       # wMaxMenuItem: it's a MENU
            return True
        close_menus(pb, 4)
        _tick(pb, 20)
    return False


def pc_box(pb, action: str, slot: int = 1) -> dict:
    """Bill's PC: action 'deposit' (party slot -> box) or 'withdraw' (box slot
    -> party) — verified by the party AND box counts moving.

    The flow the console demands (learned from screenshots): the PC and every
    submenu each print an 'Accessed …' TEXT that wants its own A before the
    next menu appears — top menu (BILL's PC first), then Bill's menu
    (WITHDRAW / DEPOSIT / RELEASE / CHANGE BOX / SEE YA), then the mon list."""
    m = pb.memory
    party0, box0 = m[W_PARTY_COUNT], m[W_BOX_COUNT]
    if action == "deposit" and party0 <= 1:
        return {"ok": False, "reason": "won't deposit the last party member"}
    if action == "withdraw" and box0 == 0:
        return {"ok": False, "reason": "the box is empty"}
    want = 0 if action == "withdraw" else 1     # index in Bill's menu
    for _ in range(2):                          # whole flow, retried once
        if not _pc_open(pb):
            return {"ok": False, "reason": "couldn't switch the PC on"}
        # each stage: advance text until the MENU answers the probe-tap,
        # then park the cursor (verified) and commit with A
        if not _await_menu(pb):
            close_menus(pb, 10)
            continue
        _cursor_to(pb, 0)                       # top menu: BILL's PC
        _tap(pb, "a", 70)
        if not _await_menu(pb):
            close_menus(pb, 10)
            continue
        _cursor_to(pb, want)                    # WITHDRAW(0) / DEPOSIT(1)
        _tap(pb, "a", 70)
        if not _await_menu(pb):
            close_menus(pb, 10)
            continue
        _cursor_to(pb, slot - 1)                # the mon list
        _tap(pb, "a", 100)                      # do it — 'stored'/'taken' text
        # ⚠️ dismiss with B ONLY: the mon list re-shows after the commit and
        # an A here would deposit/withdraw ANOTHER one (it emptied the whole
        # box once) — B never re-selects
        close_menus(pb, 12)
        _tick(pb, 30)
        party1, box1 = m[W_PARTY_COUNT], m[W_BOX_COUNT]
        if (party1, box1) == ((party0 - 1, box0 + 1) if action == "deposit"
                              else (party0 + 1, box0 - 1)):
            return {"ok": True, "party": [party0, party1],
                    "box": [box0, box1]}
    return {"ok": False, "party": [party0, m[W_PARTY_COUNT]],
            "box": [box0, m[W_BOX_COUNT]]}


# ------------------------------------------------------------------ training

def train_to(pb, level: int, slot: int = 1, policy: str = "sweep",
             max_battles: int = 60) -> dict:
    """LEVEL UP: hunt + win (sweep by default — real XP, certain wins) until
    the party mon reaches the target level. Evolution prompts are declined by
    the B-heavy dismissal (the moveset and the mon stay what they were)."""
    m = pb.memory
    if not 1 <= slot <= m[W_PARTY_COUNT]:
        return {"ok": False, "reason": f"no party slot {slot}"}
    start_lv = party_level(m, slot)
    battles = 0
    while party_level(m, slot) < level and battles < max_battles:
        r = hunt(pb, max_steps=400, policy=policy)
        if not r.get("ok"):
            return {"ok": False, "reason": f"hunt failed: {r.get('reason')}",
                    "battles": battles, "level": party_level(m, slot),
                    **snapshot(pb)}
        battles += 1
        dismiss(pb)                             # level-up / evolution boxes (B)
    lv = party_level(m, slot)
    return {"ok": lv >= level, "level": lv, "from": start_lv,
            "battles": battles, **snapshot(pb)}


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
