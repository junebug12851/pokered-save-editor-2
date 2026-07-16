#!/usr/bin/env python3
"""Phase 2 — cross-reference every event flag across the whole pret/pokered tree.

Reads the canonical bit-map (Phase 1) and scans every .asm file in the
disassembly for each EVENT_* name, classifying each occurrence:

  check   CheckEvent*   (the game reads the bit)
  set     SetEvent*     (the game turns it on)
  reset   ResetEvent*   (the game turns it off)
  def     the const declaration in event_constants.asm
  data    any other reference (data tables, script pointer gating, arithmetic)

From the evidence it derives, per flag:
  used            has >=1 non-def reference
  set/check/reset counts
  temporary       the game ResetEvent's it somewhere (durable story flags never are)
  multi_map       referenced from >1 distinct map/engine source area
  files           the distinct source files that touch it (its functional home)

This is the evidence base for the dossiers and classifications. It does NOT
name or describe anything — it records what the ROM does with each bit.

Output (git-ignored): tmp/event-flags/event_usage.json  (+ a printed summary)
"""
from __future__ import annotations
import json
import os
import re
import sys
from collections import defaultdict

HOME = os.path.expanduser("~")
DEFAULT_PK = os.path.join(HOME, "Documents", "projects", "pokered")
REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(REPO, "tmp", "event-flags")
CANON = os.path.join(OUT_DIR, "event_flags_canonical.json")

TOKEN = re.compile(r"EVENT_[A-Z0-9_]+")


def kind_of(line: str) -> str:
    s = line.strip()
    if s.startswith(";"):
        return "comment"
    if "CheckEvent" in line:
        return "check"
    if "ResetEvent" in line:
        return "reset"
    if "SetEvent" in line:
        return "set"
    if re.match(r"\s*const\s+EVENT_", line):
        return "def"
    return "data"


def area_of(relpath: str) -> str:
    """A coarse 'where does this live' bucket for multi-map detection."""
    parts = relpath.replace("\\", "/").split("/")
    # data/maps/objects/PalletTown.asm -> PalletTown ; scripts/PalletTown.asm -> PalletTown
    if "maps" in parts:
        return os.path.splitext(parts[-1])[0]
    if parts and parts[0] == "scripts":
        return os.path.splitext(parts[-1])[0]
    if parts and parts[0] == "engine":
        return "engine/" + (parts[1] if len(parts) > 1 else "")
    return "/".join(parts[:2])


def main() -> int:
    pk = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PK
    if not os.path.isdir(pk):
        sys.exit(f"pokered tree not found: {pk}")
    if not os.path.exists(CANON):
        sys.exit("run import_event_flags.py first (canonical map missing)")

    with open(CANON, encoding="utf-8") as fh:
        table = json.load(fh)
    name_to_idx = {r["name"]: r["index"] for r in table if r["name"]}

    # --- symbol table for range endpoints (EVENT_* names + boundary DEFs) ---
    # boundary constants like INDIGO_PLATEAU_EVENTS_START/END are DEF'd against
    # const_value mid-file, so replay the const walk to resolve them.
    sym = dict(name_to_idx)
    ec = os.path.join(pk, "constants", "event_constants.asm")
    cv = None
    with open(ec, encoding="utf-8") as fh:
        for line in fh:
            code = line.split(";", 1)[0].strip()
            if not code:
                continue
            p = code.split()
            op = p[0]
            def _int(t):
                t = t.strip()
                return int(t[1:], 16) if t.startswith("$") else int(t)
            if op == "const_def":
                cv = _int(p[1]) if len(p) > 1 else 0
            elif op == "const_next":
                cv = _int(p[1])
            elif op == "const_skip":
                cv += _int(p[1]) if len(p) > 1 else 1
            elif op == "const":
                cv += 1
            elif op == "DEF" and len(p) >= 4 and p[2] == "EQU":
                # DEF NAME EQU const_value [- 1]
                if "const_value" in code:
                    val = cv
                    if "- 1" in code or "-1" in code:
                        val = cv - 1
                    sym[p[1]] = val

    range_ops = []   # (kind 'set'|'reset', start_idx, end_idx, relfile, line)

    refs = defaultdict(list)      # index -> [ {file,line_no,kind,line} ]
    unknown_tokens = defaultdict(int)   # EVENT_ token not in constants (typos/aliases)
    raw_hits = 0                   # wEventFlags + N style references

    for root, _dirs, files in os.walk(pk):
        if os.sep + ".git" in root:
            continue
        for fn in files:
            if not fn.endswith(".asm"):
                continue
            full = os.path.join(root, fn)
            rel = os.path.relpath(full, pk)
            try:
                with open(full, encoding="utf-8") as fh:
                    lines = fh.readlines()
            except (UnicodeDecodeError, OSError):
                continue
            for n, line in enumerate(lines, 1):
                if line.strip().startswith(";"):
                    pass
                else:
                    rm = re.match(r"\s*(SetEventRange|ResetEventRange)\s+(\S+?)\s*,\s*(\S+?)(?:\s*,.*)?$",
                                  line)
                    if rm:
                        a, b = rm.group(2), rm.group(3)
                        if a in sym and b in sym:
                            kind = "set" if rm.group(1) == "SetEventRange" else "reset"
                            lo, hi = sorted((sym[a], sym[b]))
                            range_ops.append((kind, lo, hi,
                                              rel.replace("\\", "/"), line.strip(),
                                              a, b))
                if "wEventFlags" in line and "EVENT_" not in line and "MACRO" not in line \
                        and "event_byte" not in line:
                    raw_hits += 1
                for tok in TOKEN.findall(line):
                    if tok not in name_to_idx:
                        unknown_tokens[tok] += 1
                        continue
                    k = kind_of(line)
                    if k == "comment":
                        continue
                    refs[name_to_idx[tok]].append(
                        {"file": rel.replace("\\", "/"), "line_no": n,
                         "kind": k, "line": line.strip()})

    # --- apply range sweeps: every bit in a span is touched by that op ---
    # Each range is also a natural FLAG GROUP (project leadership, 2026-07-15):
    # the span's bits (named + gap) are toggled together, so the range defines a
    # group with a single group-toggle.
    def range_group(a, b, kind, relfile):
        ta = a[len("EVENT_"):] if a.startswith("EVENT_") else a
        tb = b[len("EVENT_"):] if b.startswith("EVENT_") else b
        pa, pb = ta.split("_"), tb.split("_")
        common = []
        for x, y in zip(pa, pb):
            if x == y:
                common.append(x)
            else:
                break
        base = " ".join(w.capitalize() for w in common) if common \
            else os.path.splitext(os.path.basename(relfile))[0]
        verb = "set together" if kind == "set" else "cleared together"
        return f"{base} ({verb})".strip()

    swept = defaultdict(list)   # index -> list of (kind, file, group)
    ranges_out = []
    for kind, lo, hi, relfile, line, a, b in range_ops:
        grp = range_group(a, b, kind, relfile)
        ranges_out.append({"kind": kind, "start": lo, "end": hi,
                           "start_name": a, "end_name": b, "file": relfile,
                           "group": grp, "line": line,
                           "members": list(range(lo, hi + 1))})
        for idx in range(lo, hi + 1):
            refs[idx].append({"file": relfile, "line_no": 0, "kind": kind,
                              "line": line, "via": "range"})
            swept[idx].append((kind, relfile, grp))

    with open(os.path.join(OUT_DIR, "event_ranges.json"), "w", encoding="utf-8") as fh:
        json.dump(ranges_out, fh, indent=1)

    # --- derive per-flag facts ------------------------------------------
    enriched = []
    used = temp = multimap = unused = 0
    for r in table:
        idx = r["index"]
        rr = refs.get(idx, [])
        nondef = [x for x in rr if x["kind"] != "def"]
        kinds = defaultdict(int)
        for x in rr:
            kinds[x["kind"]] += 1
        areas = sorted({area_of(x["file"]) for x in nondef})
        files = sorted({x["file"] for x in nondef})
        is_used = len(nondef) > 0
        is_temp = kinds.get("reset", 0) > 0
        is_multi = len(areas) > 1
        used += is_used
        unused += (not is_used)
        temp += is_temp
        multimap += is_multi
        enriched.append({
            **r,
            "used": is_used,
            "n_set": kinds.get("set", 0),
            "n_check": kinds.get("check", 0),
            "n_reset": kinds.get("reset", 0),
            "n_data": kinds.get("data", 0),
            "temporary": is_temp,
            "multi_map": is_multi,
            "block_swept": idx in swept,
            "range_groups": sorted({g for _, _, g in swept.get(idx, [])}),
            "areas": areas,
            "files": files,
            "refs": rr,
        })

    os.makedirs(OUT_DIR, exist_ok=True)
    with open(os.path.join(OUT_DIR, "event_usage.json"), "w", encoding="utf-8") as fh:
        json.dump(enriched, fh, indent=1)

    named = sum(1 for r in table if r["name"])
    named_unused = sum(1 for r in enriched if r["name"] and not r["used"])
    gap_used = sum(1 for r in enriched if not r["name"] and r["used"])
    print(f"scanned pokered tree: {pk}")
    print(f"flags: 2560  named={named}  gap={named and 2560-named}")
    print(f"USED (has non-def refs): {used}   UNUSED: {unused}")
    print(f"  named-but-unused: {named_unused}   gap-but-used: {gap_used}")
    print(f"temporary (reset somewhere): {temp}")
    print(f"multi-map (refs in >1 area): {multimap}")
    print(f"raw 'wEventFlags + N' refs outside macros: {raw_hits}")
    print(f"range ops (Set/ResetEventRange): {len(range_ops)}  ->  bits swept: {len(swept)}")
    gap_swept = sum(1 for r in enriched if not r["name"] and r["block_swept"])
    print(f"  gap bits that are block-swept (were 'unused' by name): {gap_swept}")
    if unknown_tokens:
        print("EVENT_ tokens seen but NOT in event_constants (aliases/typos):")
        for t, c in sorted(unknown_tokens.items()):
            print(f"    {t}  x{c}")
    print(f"out -> {os.path.join(OUT_DIR, 'event_usage.json')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
