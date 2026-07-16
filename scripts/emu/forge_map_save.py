#!/usr/bin/env python3
"""Generate a REAL save file at ANY map — authored by the console itself.

The problem this solves (2026-07-16): poking a map id into a save makes a CHIMERA
(the Area block — dims/tileset pointers/scripts/sprites/warps/wild tables — still
belongs to the old map) and the console hard-wedges ~100 frames after Continue.
A *consistent* cross-map save needs the whole Area state written correctly, with
every pointer and side-calculation right.

The approach — and the reason it can never drift from the truth: WE DON'T BUILD
THE STATE, THE GAME DOES. This script boots the real ROM on BaseSAV (the player
stands one square below Red's front door), rewrites that door-warp's DESTINATION
in live WRAM to the target map, steps in, and lets the game's own map loader run
— every header byte, every pointer, every sprite slot, the connections, the wild
tables, the script pointer, all authored by the engine they belong to. Once the
arrival settles, it does exactly what SaveSAVtoSRAM does (engine/menus/save.asm):

    sPlayerName  <- wPlayerName            (file 0x2598, 11 bytes; kept = BaseSAV's)
    sMainData    <- wMainDataStart..End    (file 0x25A3 <- WRAM 0xD2F7, 0x789 bytes)
    sSpriteData  <- wSpriteDataStart..End  (file 0x2D2C <- WRAM 0xC100, 0x200 bytes)
    sTileAnimations                        (file 0x3522; from the verified tileset data)
    sMainDataCheckSum                      (file 0x3523; resealed)

…into a copy of BaseSAV, so party/items/box/flags remain a genuine, coherent
game state. The result is byte-for-byte what saving there in-game would produce.

Maps with no warp to land on (8 real routes) are reached by the EDGE HOP: the
connected neighbour's save is generated first (in its OWN process — one PyBoy
per process, always), the player is relocated offline to the neighbour's
connection edge, booted, and steps across; the console performs the connection
transition itself. The walker reads the live coords back after every move, so a
blocked column is detected and the next alignment tried (a zig-zag sweep along
the edge), all frame-bounded.

Self-checks before writing: arrival map id, dims vs maps.json, the stored view
pointer must equal our formula's, and the state must still be sane 120 frames
after settling (the crash-after-load guard). A save this script emits has
already run on the console.

Single-shot + ownable: one PyBoy, one process, frame-bounded loops, exits.
Exit codes: 0 ok · 2 unavailable (no ROM / no PyBoy) · 1 failure.

Usage (under tmp/emu-venv):
    python forge_map_save.py --map 0x21 [--warp 0] [--x 25 --y 8]
                             [--out path.sav] [--force]
"""
from __future__ import annotations
import argparse
import json
import shutil
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROM = REPO / "assets" / "references" / "backup.gb"
BASE_SAV = REPO / "assets" / "saves" / "natural-clean" / "BaseSAV.sav"
MAPS_JSON = REPO / "projects" / "db" / "assets" / "data" / "maps.json"
TILESET_JSON = REPO / "projects" / "db" / "assets" / "data" / "tileset.json"
CACHE_DIR = REPO / "tmp" / "emu" / "map-saves"

sys.path.insert(0, str(Path(__file__).resolve().parent))
from forge_save import reseal, relocate, view_pointer, SAV_VIEW  # noqa: E402

# Live WRAM (pret/pokered ram/wram.asm; file = WRAM - 0xAD54 inside main data)
W_CUR_MAP, W_H, W_W = 0xD35E, 0xD368, 0xD369
W_Y, W_X = 0xD361, 0xD362
W_NUM_WARPS, W_WARP_ENTRIES = 0xD3AE, 0xD3AF          # entries: y, x, destWarpID, destMap
W_OVERWORLD, W_TILEMAP = 0xC6E8, 0xC3A0
SCREEN = 20 * 18

# The SaveSAVtoSRAM copy ranges (engine/menus/save.asm), in file coordinates
F_MAIN, W_MAIN, MAIN_LEN = 0x25A3, 0xD2F7, 0x789      # sMainData <- wMainDataStart
F_SPRITE, W_SPRITE, SPRITE_LEN = 0x2D2C, 0xC100, 0x200  # sSpriteData <- wSpriteDataStart
F_ANIMS = 0x3522                                       # sTileAnimations

# tileset typeAlias -> hTileAnimations value (verified 1:1, notes/reference/tiles.md)
ANIMS = {"INDOOR": 0, "CAVE": 1, "OUTDOOR": 2}

# Refused destinations: link-cable rooms (special-warp only) and the unfinished
# glitch copies. Refusal is a stability choice, stated out loud.
UNREACHABLE = {"Trade Center", "Colosseum"}


def load_db():
    maps = json.loads(MAPS_JSON.read_text(encoding="utf-8"))
    tiles = json.loads(TILESET_JSON.read_text(encoding="utf-8"))
    by_ind = {m["ind"]: m for m in maps if m.get("ind") is not None}
    ts = {}
    for t in tiles:
        ts[t.get("nameAlias")] = t
        ts[t.get("name")] = t
    return by_ind, ts


def anims_for(map_entry, ts) -> int:
    t = ts.get(map_entry.get("tileset"))
    return ANIMS.get((t or {}).get("typeAlias", "OUTDOOR"), 2)


def neighbour_of(entry, by_ind):
    """A warp-reachable map whose connection leads INTO `entry`, + which side of
    the neighbour that connection sits on."""
    for cand in by_ind.values():
        for side, c in (cand.get("connect") or {}).items():
            if c.get("map") == entry["name"] and cand.get("warpIn"):
                return cand, side
    return None, None


def on_overworld(pb) -> bool:
    m = pb.memory
    w, h = m[W_W], m[W_H]
    if not (0 < w <= 64 and 0 < h <= 96):
        return False
    blocks = bytes(m[W_OVERWORLD:W_OVERWORLD + (w + 6) * (h + 6)])
    screen = bytes(m[W_TILEMAP:W_TILEMAP + SCREEN])
    return len(set(blocks)) > 1 and len(set(screen)) > 1


def boot_to_overworld(pb, budget=4000) -> bool:
    f = 0
    while f < budget and not on_overworld(pb):
        pb.button("start" if (f // 24) % 2 == 0 else "a", delay=8)
        for _ in range(24):
            pb.tick()
        f += 24
    return on_overworld(pb)


def start_pyboy(sav_bytes: bytes, workdir: Path):
    from pyboy import PyBoy
    workdir.mkdir(parents=True, exist_ok=True)
    rom = workdir / "rom.gb"
    shutil.copyfile(ROM, rom)
    (workdir / "rom.gb.ram").write_bytes(sav_bytes)
    return PyBoy(str(rom), window="null", sound_emulated=False)


def settle_and_guard(pb, frames=240, guard=120) -> None:
    for _ in range(frames):
        pb.tick()
    if not on_overworld(pb):
        raise RuntimeError("state went insane while settling (crash after load)")
    for _ in range(guard):
        pb.tick()
    if not on_overworld(pb):
        raise RuntimeError("state went insane in the post-settle guard window")


def dump_to_save(pb, target_map: int, map_entry, ts) -> bytes:
    """SaveSAVtoSRAM, replicated: the settled console's own state -> a save file."""
    m = pb.memory
    sav = bytearray(BASE_SAV.read_bytes())
    sav[F_MAIN:F_MAIN + MAIN_LEN] = bytes(m[W_MAIN:W_MAIN + MAIN_LEN])
    sav[F_SPRITE:F_SPRITE + SPRITE_LEN] = bytes(m[W_SPRITE:W_SPRITE + SPRITE_LEN])
    sav[F_ANIMS] = anims_for(map_entry, ts)

    # Self-checks: this file must describe the target map, coherently.
    if sav[0x260A] != target_map:
        raise RuntimeError(f"dump carries map {sav[0x260A]:#04x}, wanted {target_map:#04x}")
    w, h = sav[0x2615], sav[0x2614]
    if (map_entry.get("width"), map_entry.get("height")) != (w, h):
        raise RuntimeError(f"dims {w}x{h} disagree with maps.json "
                           f"{map_entry.get('width')}x{map_entry.get('height')}")
    x, y = sav[0x260E], sav[0x260D]
    stored_vp = sav[SAV_VIEW] | (sav[SAV_VIEW + 1] << 8)
    if stored_vp != view_pointer(x, y, w):
        raise RuntimeError(f"view pointer {stored_vp:#06x} != formula "
                           f"{view_pointer(x, y, w):#06x} at ({x},{y}) w={w}")
    return bytes(reseal(sav))


REDS_HOUSE_1F = 0x25


def _tileset_ind(entry, ts) -> int:
    t = ts.get(entry.get("tileset"))
    return int((t or {}).get("ind", 0))


def _hijack_warp(m, sy: int, sx: int, warp_idx: int, target: int) -> None:
    """Re-aim the live warp entry at (sy, sx) — and any twin sharing that spot's
    row (double door mats) — at the target."""
    hit = False
    for i in range(m[W_NUM_WARPS]):
        wy, wx = m[W_WARP_ENTRIES + 4 * i], m[W_WARP_ENTRIES + 4 * i + 1]
        if wy == sy and abs(wx - sx) <= 1:              # the mat + its twin tile
            m[W_WARP_ENTRIES + 4 * i + 2] = warp_idx
            m[W_WARP_ENTRIES + 4 * i + 3] = target
            hit = True
    if not hit:
        raise RuntimeError(f"no live warp entry at ({sy},{sx})")


def _await_map(pb, m, target: int, budget: int = 1200) -> None:
    for _ in range(budget):
        pb.tick()
        if m[W_CUR_MAP] == target and on_overworld(pb):
            return
    raise RuntimeError(f"never arrived on map {target:#04x} — cur={m[W_CUR_MAP]:#04x}")


def gen_via_door(target: int, warp_idx: int, by_ind, ts) -> bytes:
    """Boot BaseSAV and warp to the target through a hijacked door.

    ⚠️ THE SAME-TILESET TRAP (found on the console, 2026-07-16, confirmed in
    engine/overworld/tilesets.asm LoadTilesetHeader): arrival positioning
    (LoadDestinationWarpPosition) only runs when the destination TILESET differs
    from hPreviousTileset (or is a dungeon tileset). Warp Pallet -> Route 22
    (both tileset 0) and the game keeps the OLD coords and view pointer — the
    player materialises at Pallet's door square on Route 22's grid (the real
    mechanism behind Gen 1's same-tileset warp glitches). So:

      * target tileset != Overworld(0): hijack Red's FRONT DOOR from Pallet
        (tileset 0 -> different) and step in.
      * target tileset == Overworld(0): walk INTO Red's house for real first,
        then hijack its EXIT mat (tileset 1 -> 0 differs) and step out onto it.
    """
    entry = by_ind[target]
    n_dest_warps = len(entry.get("warpOut") or [])
    if not 0 <= warp_idx < n_dest_warps:
        raise RuntimeError(f"{entry['name']} has {n_dest_warps} warps; "
                           f"index {warp_idx} is out of range")
    outdoor_target = _tileset_ind(entry, ts) == 0

    pb = start_pyboy(BASE_SAV.read_bytes(), REPO / "tmp" / "emu" / "forge")
    try:
        if not boot_to_overworld(pb):
            raise RuntimeError("BaseSAV did not reach the overworld")
        m = pb.memory

        if not outdoor_target:
            # Direct: re-aim Red's front door (the (5,5) warp; player is at (5,6)).
            _hijack_warp(m, 5, 5, warp_idx, target)
            pb.button("up", delay=8)
            _await_map(pb, m, target)
        else:
            # Two-hop: enter Red's house for real...
            pb.button("up", delay=8)
            _await_map(pb, m, REDS_HOUSE_1F)
            for _ in range(180):                        # let the entry finish
                pb.tick()
            # ...re-aim the exit mat under our feet. Leaving a house = standing
            # ON the mat and pressing DOWN into the map edge (the standing-on-
            # warp collision path, home/overworld.asm CheckWarpsCollision) —
            # stepping onto the mat from inside does NOT warp.
            ey, ex = m[W_Y], m[W_X]                     # we arrive ON the mat
            _hijack_warp(m, ey, ex, warp_idx, target)
            for attempt in range(4):                    # a nudge can be needed
                pb.button("down", delay=8)
                for _ in range(90):
                    pb.tick()
                    if m[W_CUR_MAP] == target:
                        break
                if m[W_CUR_MAP] == target:
                    break
            _await_map(pb, m, target)

        settle_and_guard(pb)
        return dump_to_save(pb, target, entry, ts)
    finally:
        pb.stop(save=False)


# Crossing geometry per neighbour side: stand on that edge, step `btn`.
EDGE = {"north": ("up", "x", lambda w, h: 0),
        "south": ("down", "x", lambda w, h: 2 * h - 1),
        "west": ("left", "y", lambda w, h: 0),
        "east": ("right", "y", lambda w, h: 2 * w - 1)}

BLOCKS_DIR = REPO / "projects" / "db" / "assets" / "blocks"
TRAITS_JSON = REPO / "projects" / "db" / "assets" / "data" / "tileTraits.json"


def walkable(entry, ts, x: int, y: int) -> bool:
    """Offline walkability from OUR OWN shipped data: the square's bottom-left
    tile (the feet — MapSim::tilePassable samples the same one) must be in the
    tileset's passable list. blk (map blocks) + bst (blockset tiles) + the
    tileTraits passable list are all imported verbatim from pret/pokered."""
    ts_ind = _tileset_ind(entry, ts)
    blk = (BLOCKS_DIR / "maps" / f"{entry['ind']}.blk").read_bytes()
    bst = (BLOCKS_DIR / "tilesets" / f"{ts_ind}.bst").read_bytes()
    traits = json.loads(TRAITS_JSON.read_text(encoding="utf-8"))
    passable = set(next(t["passable"] for t in traits["tilesets"]
                        if t["ind"] == ts_ind))
    w = entry["width"]
    block = blk[(y // 2) * w + (x // 2)]
    tile = bst[block * 16 + ((y % 2) * 2 + 1) * 4 + (x % 2) * 2]
    return tile in passable


def edge_candidates(neighbour, tgt, side, ts) -> list[int]:
    """Positions along the neighbour's connection edge worth attempting: inside
    the shared span, walkable by our own collision data, ordered middle-out."""
    _, sweep_axis, edge_fn = EDGE[side]
    edge_coord = edge_fn(neighbour["width"], neighbour["height"])
    conn = (neighbour.get("connect") or {})[side]
    off2 = 2 * int(conn.get("stripOffset", 0) or 0)
    n_len = 2 * (neighbour["width"] if sweep_axis == "x" else neighbour["height"])
    t_len = 2 * (tgt["width"] if sweep_axis == "x" else tgt["height"])
    lo, hi = max(0, off2), min(n_len, off2 + t_len)
    mid = (lo + hi) // 2
    span = sorted(range(lo, hi), key=lambda p: abs(p - mid))
    out = []
    for pos in span:
        x, y = (pos, edge_coord) if sweep_axis == "x" else (edge_coord, pos)
        try:
            if walkable(neighbour, ts, x, y):
                out.append(pos)
        except Exception:
            out.append(pos)                              # data gap: still try it
    return out


def gen_edge_attempt(target: int, via: int, pos: int, by_ind, ts) -> bytes:
    """ONE crossing attempt (one PyBoy, one process): relocate on the neighbour's
    cached save to `pos` on the connection edge, boot, step across, dump."""
    tgt = by_ind[target]
    neighbour = by_ind[via]
    side = next(d for d, c in (neighbour.get("connect") or {}).items()
                if c.get("map") == tgt["name"])
    btn, sweep_axis, edge_fn = EDGE[side]
    edge_coord = edge_fn(neighbour["width"], neighbour["height"])
    cache = CACHE_DIR / f"map{via:03d}.sav"
    if not cache.exists():
        raise RuntimeError(f"neighbour save missing: {cache}")
    sav = bytearray(cache.read_bytes())
    x, y = (pos, edge_coord) if sweep_axis == "x" else (edge_coord, pos)
    relocate(sav, x, y)
    sav = reseal(sav)

    pb = start_pyboy(bytes(sav), REPO / "tmp" / "emu" / "forge-edge")
    try:
        if not boot_to_overworld(pb):
            raise RuntimeError(f"relocated {neighbour['name']} save did not boot")
        m = pb.memory
        for _ in range(3):                               # step across (a nudge ok)
            pb.button(btn, delay=8)
            for _ in range(90):
                pb.tick()
                if m[W_CUR_MAP] == target:
                    break
            if m[W_CUR_MAP] == target:
                break
        if m[W_CUR_MAP] != target:
            raise RuntimeError(f"edge column {pos} did not cross into {tgt['name']}")
        for _ in range(600):
            pb.tick()
            if on_overworld(pb):
                break
        settle_and_guard(pb)
        return dump_to_save(pb, target, tgt, ts)
    finally:
        pb.stop(save=False)


def ensure_map_save(target: int, warp_idx: int, force: bool, by_ind, ts) -> Path:
    """Cached generation; returns the cache path. The edge hop's neighbour is
    generated in a FRESH subprocess (one PyBoy per process, always)."""
    CACHE_DIR.mkdir(parents=True, exist_ok=True)
    cache = CACHE_DIR / f"map{target:03d}.sav"
    if cache.exists() and not force:
        return cache
    entry = by_ind.get(target)
    if entry is None or entry.get("width") is None:
        raise RuntimeError(f"map {target:#04x} is not a generatable destination")
    if entry["name"] in UNREACHABLE or "Copy" in entry["name"]:
        raise RuntimeError(f"{entry['name']} is not a reachable destination "
                           f"(link room / unfinished copy) — refused for stability")
    if entry.get("warpIn"):
        data = gen_via_door(target, warp_idx, by_ind, ts)
        route = "door-warp"
    else:
        neighbour, side = neighbour_of(entry, by_ind)
        if neighbour is None:
            raise RuntimeError(f"no warp-reachable neighbour for {entry['name']}")
        n_cache = CACHE_DIR / f"map{neighbour['ind']:03d}.sav"
        if not n_cache.exists():
            r = subprocess.run([sys.executable, str(Path(__file__).resolve()),
                                "--map", str(neighbour["ind"])],
                               capture_output=True, text=True, timeout=420,
                               cwd=str(REPO))
            if r.returncode != 0:
                raise RuntimeError(f"neighbour generation failed:\n{r.stdout}{r.stderr}")
        # Each crossing attempt is ONE PyBoy in ONE fresh subprocess; candidates
        # are pre-filtered by our own shipped collision data (walkable()).
        cands = edge_candidates(neighbour, entry, side, ts)
        if not cands:
            raise RuntimeError(f"no walkable {side}-edge column on "
                               f"{neighbour['name']} toward {entry['name']}")
        data = None
        errs = []
        for pos in cands[:12]:
            r = subprocess.run([sys.executable, str(Path(__file__).resolve()),
                                "--map", str(target), "--edge-attempt", str(pos),
                                "--via", str(neighbour["ind"])],
                               capture_output=True, text=True, timeout=180,
                               cwd=str(REPO))
            if r.returncode == 0 and cache.exists():
                data = cache.read_bytes()
                break
            errs.append(f"col {pos}: rc={r.returncode} "
                        f"{(r.stderr or r.stdout).strip().splitlines()[-1:]}")
        if data is None:
            raise RuntimeError(f"every edge attempt failed "
                               f"({neighbour['name']} -> {entry['name']}):\n"
                               + "\n".join(errs))
        route = f"edge-hop via {neighbour['name']}"
    cache.write_bytes(data)
    (CACHE_DIR / f"map{target:03d}.json").write_text(json.dumps(
        {"map": target, "name": entry["name"],
         "x": data[0x260E], "y": data[0x260D],
         "generated": time.strftime("%Y-%m-%d %H:%M:%S"), "route": route}, indent=1))
    return cache


def _int(s):
    return int(s, 0)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--map", type=_int, required=True, help="target map id (hex ok)")
    ap.add_argument("--warp", type=_int, default=0, help="arrival warp index on the target")
    ap.add_argument("--x", type=_int, default=None)
    ap.add_argument("--y", type=_int, default=None)
    ap.add_argument("--out", default=None, help="write the (relocated) save here")
    ap.add_argument("--force", action="store_true", help="regenerate even if cached")
    ap.add_argument("--edge-attempt", type=_int, default=None,
                    help="INTERNAL: one edge-crossing attempt at this column")
    ap.add_argument("--via", type=_int, default=None,
                    help="INTERNAL: the neighbour map ind for --edge-attempt")
    a = ap.parse_args()

    if not ROM.exists():
        sys.stderr.write("no ROM at assets/references/backup.gb (local-only)\n")
        return 2
    try:
        import pyboy  # noqa: F401
    except ImportError:
        sys.stderr.write("PyBoy not installed — run under tmp/emu-venv\n")
        return 2

    by_ind, ts = load_db()

    if a.edge_attempt is not None:                       # internal single attempt
        if a.via is None:
            sys.stderr.write("--edge-attempt needs --via\n")
            return 1
        data = gen_edge_attempt(a.map, a.via, a.edge_attempt, by_ind, ts)
        CACHE_DIR.mkdir(parents=True, exist_ok=True)
        (CACHE_DIR / f"map{a.map:03d}.sav").write_bytes(data)
        print(json.dumps({"map": a.map, "edge_column": a.edge_attempt, "ok": True}))
        return 0

    cache = ensure_map_save(a.map, a.warp, a.force, by_ind, ts)
    data = cache.read_bytes()

    out = cache
    if a.x is not None or a.y is not None:
        sav = bytearray(data)
        relocate(sav,
                 sav[0x260E] if a.x is None else a.x,
                 sav[0x260D] if a.y is None else a.y)
        data = bytes(reseal(sav))
        out = Path(a.out) if a.out else \
            CACHE_DIR / f"map{a.map:03d}_at_{data[0x260E]}_{data[0x260D]}.sav"
    elif a.out:
        out = Path(a.out)
    if out != cache:
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_bytes(data)

    print(json.dumps({"map": a.map, "name": by_ind[a.map]["name"], "sav": str(out),
                      "x": data[0x260E], "y": data[0x260D]}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
