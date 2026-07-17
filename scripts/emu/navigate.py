#!/usr/bin/env python3
"""World model + pathfinding for the dev autopilot — PURE DATA, no PyBoy.

Everything here is computed from the project's own shipped data, all of it imported
verbatim from pret/pokered: maps.json (dims, warps, connections, sprites),
tileTraits.json (passable lists, ledges, pair collisions), and the .blk/.bst block
data. The executor (autopilot.py, inside the session child) walks the plans this
module makes, verifying every step against the console's own WRAM.

Design: notes/plans/dev-autopilot.md.

  World().resolve("Mt Moon B1F" | "Mt. Moon 2" | 60 | "0x3C") -> map entry
  World().grid(ind)          -> Grid (square passability + feet tiles + ledges)
  astar(grid, a, b, blocked) -> [step, ...] | None       (step = {"dir","to","jump"})
  route(world, src, pos, dst, dst_pos, last_map) -> [leg, ...] | None
      leg = {"type":"walk",  "map":ind, "to":[x,y]}
          | {"type":"warp",  "map":ind, "at":[x,y], "to_map":ind, "arrive":[x,y]}
          | {"type":"cross", "map":ind, "side":s, "btn":dir, "to_map":ind,
             "columns":[[x,y],...]}       (both-side-walkable crossing squares)

Coordinates are SQUARES (2x2 tiles), the player's own unit (wXCoord/wYCoord).
The feet sample is the square's bottom-left tile — the same one MapSim::tilePassable
and forge_map_save.walkable() use.

Ledges only exist on the Overworld tileset (HandleLedges: `ld a,[wCurMapTileset] /
and a / ret nz`) — a "ledge tile id" in a cave is just a rock.
"""
from __future__ import annotations

import heapq
import json
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
MAPS_JSON = REPO / "projects" / "db" / "assets" / "data" / "maps.json"
TILESET_JSON = REPO / "projects" / "db" / "assets" / "data" / "tileset.json"
TRAITS_JSON = REPO / "projects" / "db" / "assets" / "data" / "tileTraits.json"
BLOCKS_DIR = REPO / "projects" / "db" / "assets" / "blocks"

DIRS = {"up": (0, -1), "down": (0, 1), "left": (-1, 0), "right": (1, 0)}

# FORCED-MOVEMENT squares ("spinner"/arrow tiles + the Pokémon Tower 7F carry
# spots) — per-map coordinates extracted from pret/pokered's own scripts
# (`map_coord_movement x, y, …` in scripts/{ViridianGym,RocketHideoutB2F,
# RocketHideoutB3F,PokemonTower7F}.asm). Stepping on one hands the controls to
# the map script; the planner prices them high (routes around when possible,
# crosses when there is no other way) and the executor settles the slide.
SPINNERS = {
    "Viridian Gym": {(0, 15), (1, 15), (4, 6), (4, 14), (5, 13), (11, 2),
                     (13, 16), (13, 17), (16, 10), (18, 2), (19, 1), (19, 11)},
    "Rocket Hideout B2F": {
        (4, 9), (4, 11), (4, 15), (4, 16), (4, 19), (4, 22), (5, 14), (6, 22),
        (6, 24), (8, 9), (8, 12), (8, 15), (8, 19), (8, 23), (9, 14), (9, 22),
        (10, 9), (10, 10), (10, 15), (10, 17), (10, 19), (10, 25), (11, 14),
        (11, 16), (11, 18), (12, 9), (12, 11), (12, 13), (12, 17), (13, 10),
        (13, 12), (13, 16), (13, 18), (13, 19), (13, 22), (13, 23), (14, 17),
        (15, 16), (16, 14), (16, 16), (16, 18), (17, 10), (17, 11)},
    "Rocket Hideout B3F": {
        (10, 13), (10, 19), (11, 18), (12, 11), (12, 17), (12, 20), (13, 16),
        (14, 11), (14, 15), (14, 17), (14, 19), (15, 16), (15, 18), (16, 13),
        (17, 12), (18, 16)},
    "Pokemon Tower 7F": {(9, 8), (9, 9), (9, 12), (10, 7), (10, 9), (10, 11),
                         (11, 7), (11, 9), (11, 11), (12, 7), (12, 10),
                         (12, 11)},
}
SPIN_COST = 400                             # avoided at almost any cost: entering
#   one hands the controls to the map script and the player lands wherever the
#   arrow chain drops them (the planner can't predict it without the per-square
#   arrow directions — a future import). Still an edge, so a maze that CANNOT
#   be crossed on foot is attempted rather than refused, and the executor
#   settles the slide and re-plans from wherever it lands.

# Elevator maps: ridden by re-aiming the car's own door warps in live WRAM (the
# forge's sanctioned door-hijack technique) — the car's script does exactly the
# same thing off its floor menu.
ELEVATORS = {127: "Celadon Mart Elevator", 203: "Rocket Hideout Elevator",
             236: "Silph Co Elevator"}

# The Saffron gate maps whose guard blocks passage until the drink flag —
# wStatusFlags1 (0xD728) bit 6 = BIT_GAVE_SAFFRON_GUARDS_DRINK
# (pret scripts/Route5Gate.asm et al.). Route 5/6/7/8 Gate.
SAFFRON_GATES = {70, 73, 76, 79}

# Crossing geometry per connection side of the map being LEFT: the edge row/col
# on this map, the button that steps out, and the arrival row/col on the target.
EDGE_OUT = {
    "north": ("up",    "x", lambda w2, h2: 0,      lambda tw2, th2: th2 - 1),
    "south": ("down",  "x", lambda w2, h2: h2 - 1, lambda tw2, th2: 0),
    "west":  ("left",  "y", lambda w2, h2: 0,      lambda tw2, th2: tw2 - 1),
    "east":  ("right", "y", lambda w2, h2: w2 - 1, lambda tw2, th2: 0),
}


class Grid:
    """One map's square grid: feet tiles, passability, ledges, pair collisions,
    water (surfable), cuttable trees, and forced-movement hazards."""

    def __init__(self, entry: dict, ts: dict, traits: dict):
        self.entry = entry
        self.w2 = 2 * entry["width"]
        self.h2 = 2 * entry["height"]
        ts_ind = int(ts.get("ind", 0))
        self.ts_ind = ts_ind
        blk = (BLOCKS_DIR / "maps" / f"{entry['ind']}.blk").read_bytes()
        bst = (BLOCKS_DIR / "tilesets" / f"{ts_ind}.bst").read_bytes()
        t = next(x for x in traits["tilesets"] if x["ind"] == ts_ind)
        passable = set(t["passable"])
        self.pairs = {frozenset(p) for p in (t.get("pairCollisions") or [])}
        self.pairs_water = {frozenset(p)
                            for p in (t.get("pairCollisionsWater") or [])}
        self.bst = bst
        water_tile = int(traits.get("waterTile", 20))
        cut_blocks = {c["block"]: c["cutTo"]
                      for c in (traits.get("cutTreeBlocks") or [])}
        # Ledges are an Overworld-tileset-only mechanism (see module doc).
        self.ledges: dict[tuple[int, str], set[int]] = {}
        if ts_ind == 0:
            for l in traits.get("ledges") or []:
                self.ledges.setdefault((l["standingOn"], l["facing"]),
                                       set()).add(l["tile"])
        w = entry["width"]
        self.feet = [[0] * self.w2 for _ in range(self.h2)]
        self.passable = [[False] * self.w2 for _ in range(self.h2)]
        self.water = [[False] * self.w2 for _ in range(self.h2)]
        self.block = [[0] * self.w2 for _ in range(self.h2)]
        # square -> replacement block id, when its block is a cuttable tree
        # (cutTreeBlocks: OUR OWN imported data carries the cut-to block)
        self.cuttable: dict[tuple[int, int], int] = {}
        for y in range(self.h2):
            for x in range(self.w2):
                block = blk[(y // 2) * w + (x // 2)]
                tile = bst[block * 16 + ((y % 2) * 2 + 1) * 4 + (x % 2) * 2]
                self.feet[y][x] = tile
                self.block[y][x] = block
                self.passable[y][x] = tile in passable
                self.water[y][x] = tile == water_tile and bool(t.get("hasWater"))
                if ts_ind == 0 and block in cut_blocks \
                        and not self.passable[y][x]:
                    self.cuttable[(x, y)] = cut_blocks[block]
        # forced-movement squares (SPINNERS), matched by map name/modernName
        self.hazards: set = set()
        for key in (entry.get("name"), entry.get("modernName")):
            if key in SPINNERS:
                self.hazards = SPINNERS[key]

    def ok(self, x: int, y: int) -> bool:
        return 0 <= x < self.w2 and 0 <= y < self.h2 and self.passable[y][x]

    def neighbors(self, x: int, y: int, blocked: frozenset,
                  surf: bool = False, cut: bool = False):
        """(nx, ny, dir, cost) — 4-dir walks + one-way ledge hops. surf=True
        opens water squares (pairCollisionsWater honored); cut=True opens
        cuttable tree squares (the executor clears the block before stepping).
        Forced-movement hazards cost SPIN_COST — priced, never refused."""
        if not (0 <= x < self.w2 and 0 <= y < self.h2):
            return                                    # mid-transition coords
        here = self.feet[y][x]
        for d, (dx, dy) in DIRS.items():
            nx, ny = x + dx, y + dy
            if not (0 <= nx < self.w2 and 0 <= ny < self.h2):
                continue
            front = self.feet[ny][nx]
            hop = self.ledges.get((here, d))
            if hop and front in hop:                      # jump the ledge: 2 squares
                lx, ly = x + 2 * dx, y + 2 * dy
                if self.ok(lx, ly) and (lx, ly) not in blocked:
                    yield lx, ly, d, 2
                continue
            if (nx, ny) in blocked:
                continue
            enterable = self.passable[ny][nx]
            cost = 1
            if not enterable and surf and self.water[ny][nx]:
                enterable = True
                if self.pairs_water and frozenset((here, front)) in self.pairs_water:
                    continue
            if not enterable and cut and (nx, ny) in self.cuttable:
                enterable = True
                cost = 9                                  # clearing a tree isn't free
            if not enterable:
                continue
            if self.passable[ny][nx] and self.pairs \
                    and frozenset((here, front)) in self.pairs:
                continue                                  # elevation edge (caves)
            if (nx, ny) in self.hazards:
                cost += SPIN_COST
            yield nx, ny, d, cost


def astar(grid: Grid, start, goal, blocked: frozenset = frozenset(),
          max_nodes: int = 60000, force_goal: bool = True,
          surf: bool = False, cut: bool = False):
    """Steps from start to goal: [{"dir","to":[x,y],"jump":bool}, ...] | None.
    `blocked` (live NPC squares, failed squares) is honored except at the goal.
    force_goal: a goal square is enterable even when its tile is SOLID — warp
    squares are portals (gate doorways, cave ladders): the console warps you on
    the ATTEMPT to walk in (CheckWarpsNoCollision), the tile never has to yield."""
    start, goal = tuple(start), tuple(goal)
    if start == goal:
        return []
    in_bounds = 0 <= goal[0] < grid.w2 and 0 <= goal[1] < grid.h2
    forced = force_goal and in_bounds and not grid.ok(*goal)
    if not in_bounds or (not grid.ok(*goal) and not forced):
        return None
    blocked = frozenset(b for b in blocked if b != goal)
    openq = [(abs(start[0] - goal[0]) + abs(start[1] - goal[1]), 0, start)]
    g = {start: 0}
    came: dict = {}
    seen = 0
    while openq and seen < max_nodes:
        _, cost, cur = heapq.heappop(openq)
        if cur == goal:
            steps = []
            while cur != start:
                px, py, d, jump = came[cur]
                steps.append({"dir": d, "to": [cur[0], cur[1]], "jump": jump})
                cur = (px, py)
            steps.reverse()
            return steps
        if cost > g.get(cur, 1 << 30):
            continue
        seen += 1
        steps_out = list(grid.neighbors(cur[0], cur[1], blocked, surf, cut))
        if forced and abs(cur[0] - goal[0]) + abs(cur[1] - goal[1]) == 1:
            d = next(dr for dr, (dx, dy) in DIRS.items()
                     if (cur[0] + dx, cur[1] + dy) == goal)
            steps_out.append((goal[0], goal[1], d, 1))
        for nx, ny, d, c in steps_out:
            nc = cost + c
            if nc < g.get((nx, ny), 1 << 30):
                g[(nx, ny)] = nc
                jump = abs(nx - cur[0]) + abs(ny - cur[1]) == 2
                came[(nx, ny)] = (cur[0], cur[1], d, jump)
                heapq.heappush(openq, (nc + abs(nx - goal[0]) + abs(ny - goal[1]),
                                       nc, (nx, ny)))
    return None


class World:
    """maps.json + tileTraits.json, indexed; grids cached."""

    def __init__(self):
        self.maps = json.loads(MAPS_JSON.read_text(encoding="utf-8"))
        tilesets = json.loads(TILESET_JSON.read_text(encoding="utf-8"))
        self.traits = json.loads(TRAITS_JSON.read_text(encoding="utf-8"))
        self.by_ind = {m["ind"]: m for m in self.maps if m.get("ind") is not None}
        self.by_name: dict[str, dict] = {}
        for m in self.maps:
            for k in ("name", "modernName"):
                if m.get(k):
                    self.by_name[m[k].lower()] = m
        self.ts = {}
        for t in tilesets:
            self.ts[t.get("nameAlias")] = t
            self.ts[t.get("name")] = t
        self._grids: dict[int, Grid] = {}

    def resolve(self, ref) -> dict:
        """A map, by ind (int / '0x3B' / '59') or by name / modernName."""
        if isinstance(ref, int):
            e = self.by_ind.get(ref)
        else:
            s = str(ref).strip()
            try:
                e = self.by_ind.get(int(s, 0))
            except ValueError:
                e = self.by_name.get(s.lower())
        if e is None:
            raise KeyError(f"unknown map: {ref!r}")
        return e

    def walkable_map(self, e: dict) -> bool:
        return (e.get("width") is not None and not e.get("glitch")
                and e.get("incomplete") is None and e.get("tileset") in self.ts)

    def grid(self, ind: int) -> Grid:
        if ind not in self._grids:
            e = self.by_ind[ind]
            self._grids[ind] = Grid(e, self.ts[e["tileset"]], self.traits)
        return self._grids[ind]

    # ---------------------------------------------------------------- portals

    def warps_of(self, ind: int, last_map: int | None):
        """Usable warps: (at, dest_ind, arrive) — 'Last Map' resolved via context."""
        e = self.by_ind[ind]
        out = []
        for w in e.get("warpOut") or []:
            to = w.get("toMap")
            if to == "Last Map":
                if last_map is None or last_map not in self.by_ind:
                    continue
                d = self.by_ind[last_map]
            else:
                try:
                    d = self.resolve(to)
                except KeyError:
                    continue
            if not self.walkable_map(d):
                continue
            arr = (d.get("warpIn") or [])
            k = w.get("toWarp", 0)
            if not 0 <= k < len(arr):
                continue
            out.append(((w["x"], w["y"]), d["ind"], (arr[k]["x"], arr[k]["y"])))
        return out

    def elevator_floors(self, elev_ind: int):
        """Floors an elevator serves: every map with a warp INTO the car.
        Returns (floor_ind, floor's-elevator-door warp index, arrival square).
        Riding = re-aim the car's own door warps at (floor, that index) in live
        WRAM and step out — exactly what the car's floor-menu script does."""
        out = []
        for m in self.maps:
            if m.get("ind") is None or not self.walkable_map(m):
                continue
            for k, w in enumerate(m.get("warpOut") or []):
                try:
                    if w.get("toMap") != "Last Map" \
                            and self.resolve(w["toMap"])["ind"] == elev_ind:
                        arr = (m.get("warpIn") or [])
                        if k < len(arr):
                            out.append((m["ind"], k, (arr[k]["x"], arr[k]["y"])))
                        break
                except KeyError:
                    continue
        return out

    def approaches_of(self, dst_ind: int):
        """Maps you'd naturally ARRIVE FROM: everything with a warp into dst,
        plus dst's connection neighbours — outdoor-ish first (the doorstep
        drop-in: boot one map out, walk in for real, wLastMap authored by the
        walk). Elevators and link rooms are not approaches."""
        dst = self.by_ind[dst_ind]
        cands = []
        for m in self.maps:
            if m.get("ind") is None or m["ind"] == dst_ind \
                    or not self.walkable_map(m) or m["ind"] in ELEVATORS:
                continue
            for w in m.get("warpOut") or []:
                try:
                    if w.get("toMap") != "Last Map" \
                            and self.resolve(w["toMap"])["ind"] == dst_ind:
                        cands.append(m)
                        break
                except KeyError:
                    continue
        for conn in (dst.get("connect") or {}).values():
            try:
                n = self.resolve(conn["map"])
                if self.walkable_map(n):
                    cands.append(n)
            except KeyError:
                continue
        # "Last Map" doors: dst's own $FF exits lead to whoever owns dst —
        # find the map that warps INTO dst (already collected above).
        seen: set = set()
        ranked = []
        for m in sorted(cands, key=lambda e: 0 if _outdoorish(e) else 1):
            if m["ind"] not in seen:
                seen.add(m["ind"])
                ranked.append(m["ind"])
        return ranked

    def crossings_of(self, ind: int, surf: bool = False):
        """Connections: (side, btn, dest_ind, columns=[((x,y) here, (x,y) there)]).
        Columns are walkable on BOTH sides (offline check; the executor still
        verifies live). surf=True lets a column be WATER on either side —
        Pallet -> Route 21 is a water-only crossing.

        ⚠️ maps.json stores the POST-CLAMP pair (stripMove == tgt-3, stripOffset
        == src), NOT the raw signed offset. The clamp is invertible — the same
        rule as the cartridge-verified MapEngine::connectionOffset():
        offset = (stripOffset != 0) ? (-stripOffset - 3) : stripMove.
        Crossing at column p on this map arrives at column p - 2*offset."""
        e = self.by_ind[ind]
        g = self.grid(ind)
        out = []
        for side, conn in (e.get("connect") or {}).items():
            try:
                d = self.resolve(conn["map"])
            except KeyError:
                continue
            if not self.walkable_map(d):
                continue
            gd = self.grid(d["ind"])
            btn, axis, edge_fn, arr_edge_fn = EDGE_OUT[side]
            edge = edge_fn(g.w2, g.h2)
            arr_edge = arr_edge_fn(gd.w2, gd.h2)
            so = int(conn.get("stripOffset", 0) or 0)
            sm = int(conn.get("stripMove", 0) or 0)
            offset = (-so - 3) if so != 0 else sm
            off2 = 2 * offset
            m_len = g.w2 if axis == "x" else g.h2
            t_len = gd.w2 if axis == "x" else gd.h2
            lo, hi = max(0, off2), min(m_len, off2 + t_len)
            def _enterable(gr, cx, cy):
                return gr.ok(cx, cy) or (surf and 0 <= cx < gr.w2
                                         and 0 <= cy < gr.h2
                                         and gr.water[cy][cx])
            cols = []
            for p in range(lo, hi):
                x, y = (p, edge) if axis == "x" else (edge, p)
                tp = p - off2
                tx, ty = (tp, arr_edge) if axis == "x" else (arr_edge, tp)
                if _enterable(g, x, y) and _enterable(gd, tx, ty):
                    cols.append(((x, y), (tx, ty)))
            if cols:
                out.append((side, btn, d["ind"], cols))
        return out


def _outdoorish(e: dict) -> bool:
    """Maps that update wLastMap when you warp OUT of them into a building."""
    return bool(e.get("connect")) or e.get("tileset") == "OUTDOOR"


def distances(grid: Grid, start, blocked: frozenset = frozenset(),
              surf: bool = False, cut: bool = False) -> dict:
    """Dijkstra distance map from start over the whole grid (ledge hops cost 2).
    One of these per route-search state replaces an A* per portal."""
    start = tuple(start)
    dist = {start: 0}
    pq = [(0, start)]
    while pq:
        d, cur = heapq.heappop(pq)
        if d > dist.get(cur, 1 << 30):
            continue
        for nx, ny, _, c in grid.neighbors(cur[0], cur[1], blocked, surf, cut):
            nd = d + c
            if nd < dist.get((nx, ny), 1 << 30):
                dist[(nx, ny)] = nd
                heapq.heappush(pq, (nd, (nx, ny)))
    return dist


def route(world: World, src_map: int, src_pos, dst_map: int, dst_pos=None,
          last_map: int | None = None, max_states: int = 3000,
          avoid_warps: set | None = None,
          surf: bool = False, cut: bool = False):
    """Cross-map plan: Dijkstra over (map, position, last-map context) states.
    Per expanded state ONE grid distance map (warp squares blocked — walking
    over a door/ladder teleports; a warp GOAL is priced through its passable
    neighbours + 1, matching how the executor approaches it). Returns legs
    (see module doc) or None. dst_pos=None = 'anywhere on dst_map'."""
    src_pos = tuple(src_pos)
    if dst_pos is not None:
        dst_pos = tuple(dst_pos)

    start = (src_map, src_pos, last_map)
    dist = {start: 0}
    prev: dict = {}
    pq = [(0, 0, start)]
    tie = 1
    expanded = 0
    goal_state = None
    dmaps: dict = {}
    while pq and expanded < max_states:
        d, _, state = heapq.heappop(pq)
        if d > dist.get(state, 1 << 30):
            continue
        m_ind, pos, ctx = state
        if m_ind == dst_map and (dst_pos is None or pos == dst_pos):
            goal_state = state
            break
        expanded += 1
        e = world.by_ind.get(m_ind)
        if e is None or not world.walkable_map(e):
            continue
        grid = world.grid(m_ind)
        warp_sqs = frozenset((w["x"], w["y"]) for w in e.get("warpOut") or [])
        dkey = (m_ind, pos)
        if dkey not in dmaps:
            dmaps[dkey] = distances(grid, pos, warp_sqs, surf, cut)
        dmap = dmaps[dkey]

        def cost_to(sq):
            if sq == pos:
                return 0
            if sq in dmap:
                return dmap[sq]
            if sq in warp_sqs:              # enter the warp square as a last step
                best = None
                for dx, dy in ((0, 1), (0, -1), (1, 0), (-1, 0)):
                    c = dmap.get((sq[0] + dx, sq[1] + dy))
                    if c is not None and (best is None or c + 1 < best):
                        best = c + 1
                return best
            return None

        edges = []
        if m_ind == dst_map and dst_pos is not None:
            c = cost_to(dst_pos)
            if c is not None:
                edges.append((c, (m_ind, dst_pos, ctx),
                              {"type": "walk", "map": m_ind,
                               "to": [dst_pos[0], dst_pos[1]]}))
        new_ctx = m_ind if _outdoorish(e) else ctx
        for at, to_ind, arrive in world.warps_of(m_ind, ctx):
            if avoid_warps and (m_ind, at) in avoid_warps:
                continue                    # refused to open for the executor
            c = cost_to(at)
            if c is None:
                continue
            # a SOLID warp square (gate doorway half whose warp may not fire on
            # the bump) is dispreferred when a passable sibling exists
            if not grid.ok(*at):
                c += 6
            edges.append((c + 2, (to_ind, arrive, new_ctx),
                          {"type": "warp", "map": m_ind, "at": [at[0], at[1]],
                           "to_map": to_ind, "arrive": [arrive[0], arrive[1]]}))
        if m_ind in ELEVATORS:
            doors = [(w["x"], w["y"]) for w in e.get("warpOut") or []]
            door_cost = min((c for c in (cost_to(dq) for dq in doors)
                             if c is not None), default=None)
            if door_cost is not None:
                for f_ind, k, arrive in world.elevator_floors(m_ind):
                    edges.append((door_cost + 4, (f_ind, arrive, ctx),
                                  {"type": "elevator", "map": m_ind,
                                   "doors": [[dq[0], dq[1]] for dq in doors],
                                   "to_map": f_ind, "dest_warp": k,
                                   "arrive": [arrive[0], arrive[1]]}))
        for side, btn, to_ind, cols in world.crossings_of(m_ind, surf=surf):
            reach = [(dmap.get(c_[0]), c_) for c_ in cols]
            reach = [(c, c_) for c, c_ in reach if c is not None]
            if not reach:
                continue
            reach.sort(key=lambda r: r[0])
            c, ((cx, cy), (tx, ty)) = reach[0]
            edges.append((c + 2, (to_ind, (tx, ty), new_ctx),
                          {"type": "cross", "map": m_ind, "side": side,
                           "btn": btn, "at": [cx, cy], "to_map": to_ind,
                           "columns": [[r[1][0][0], r[1][0][1]]
                                       for r in reach[:5]]}))
        for c, nstate, leg in edges:
            nd = d + c
            if nd < dist.get(nstate, 1 << 30):
                dist[nstate] = nd
                prev[nstate] = (state, leg)
                heapq.heappush(pq, (nd, tie, nstate))
                tie += 1
    if goal_state is None:
        return None
    legs = []
    s = goal_state
    while s in prev:
        s, leg = prev[s]
        legs.append(leg)
    legs.reverse()
    return legs
