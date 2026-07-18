# Map States — per-map progression blueprints (briefed 2026-07-17, Fairy Fox)

**The brief (2026-07-17):** every map has one or more *default states* — where the NPCs stand,
which filter flags (missables) are on, which event flags are set, and what the map's script byte
holds. The map's script largely determines the map's **progress**: each map has a progression, and
the world/persistent data changes with it. We need, per map, a researched JSON blueprint of every
progression state so that:

1. **The player can roll a map back and forth in progression** — find the current state, step one
   forward or one back, and the app applies exactly the condensed save-file changes.
2. **Changing the current map to another map constructs that map properly** — pick a state from
   the blueprint list, copy its sprite set-up and save-file state in, land the player on a
   designated entry spot (the first warp by default).
3. **The randomization engine gets better map/state randomization** — pick any legal state.
4. **"Map script" is renamed "map state"** across the map screen (new/current map state instead
   of map script/cur map script), with progression numbers ("1.", "2a.", "2b.", "3.") plus a name,
   description, triggers, effects, and its place on the game's timeline.

Save-file-only doctrine: we do not care about ROM or transient WRAM — a state's effects are
expressed **only** as save-file facts (script byte, event flags, missables, sprites already in the
save's Area block, and the handful of world bytes a stage owns).

## The model (what a "map state" is)

Research finding (see [`reference/map-states.md`](../reference/map-states.md)): a map's
`SCRIPT_<MAP>_*` values are **not all states of the world** — they split into:

- **Resting stages** — values the byte *sits at* between play sessions (DEFAULT, NOOP, the
  post-cutscene watcher steps). These are the progression stages a person rolls through.
- **Transient cutscene steps** — values the engine passes through frame-by-frame mid-cutscene
  (Oak walking to the player, a rival walking in). They are real, storable byte values (we never
  refuse them) but they are *not* what you roll to; loading a save inside one resumes mid-cutscene.

And a resting stage is **more than the script byte**: it is script byte + the event flags the
stage's story has set + the missable visibility bits + (occasionally) other world bytes. The
blueprint carries all of it.

## The data (shipped, generated, checked)

- **One JSON per map** with any state at all:
  `projects/db/assets/data/map-states/<MapFileBase>.json` + an `_index.json`.
- **Generator:** `scripts/extract_map_states.py` — parses `pret/pokered`'s `scripts/<Map>.asm`
  (the game's own source, per the file-format rule: their macro names are the command names),
  joins onto our shipped vocabularies (`events.json` ind/pretName, `missables.json`
  ind/toggleConst, `scripts.json` slot, `maps.json` warps/sprites), merges the curated overlay,
  and emits the blueprints. `--check` mode diffs against the committed files (CI-able).
- **Curated overlay:** `scripts/data/map_states_curated.json` — the hand-researched story layer
  (stage grouping, names, descriptions, triggers, timeline) for the story maps. The extractor
  refuses a curated stage that references a script value / event / missable that does not exist
  in the sources (self-checking, the import_events_db precedent).

### Schema (per map file)

```json
{
  "map": "Pallet Town",            // maps.json name (join key)
  "mapInd": 0,
  "scriptSlot": 1,                  // scripts.json ind (the w<Map>CurScript save byte); null = no script byte
  "source": "scripts/PalletTown.asm",
  "entry": { "kind": "warp", "warp": 0, "x": 5, "y": 6 },   // default landing spot (first warp; fallback: map centre)
  "progression": {
    "start": "1", "end": "3",
    "order": ["1", "2", "3"],       // resting stages only, story order
    "branches": {},                  // stage -> ["2a","2b"] where genuinely parallel/alternate
    "messy": false,                  // true where the graph resists a clean line (notes say why)
    "notes": "…"
  },
  "states": [
    {
      "id": "1",                     // resting stages: "1","2","2a","2b","3"… ; transient: "1.1","1.2"… (the dots are the cutscene between stages)
      "kind": "resting",             // resting | transient
      "script": 0,                   // the byte value w<Map>CurScript holds in this state
      "scriptName": "DEFAULT",
      "name": "…",                   // human name
      "desc": "…",                   // what the map looks/behaves like here
      "timeline": "…",               // where this sits in the run of the game
      "trigger": { "type": "start|coord|talk|cutscene|battle|event|item", "text": "what moved the game INTO this state" },
      "advance": [ { "to": "2", "text": "…", "auto": false } ],   // outgoing edges (auto = advances by itself)
      "save": {                      // ABSOLUTE blueprint — select this state from the list and write these
        "script": 0,
        "events":    { "set": [ {"ind": 2, "name": "EVENT_…", "owned": true} ], "cleared": [ … ] },
        "missables": { "show": [ {"ind": 0, "name": "…"} ], "hide": [ … ] },
        "notes": [ "…anything that can't be expressed as bits…" ]
      },
      "delta": {                     // vs the PREVIOUS resting stage — the roll-forward increment
        "events":    { "set": [...], "cleared": [...] },
        "missables": { "show": [...], "hide": [...] }
      }
    }
  ]
}
```

Semantics that keep it honest:

- **`owned` vs context flags.** `owned: true` = this map's own scripts write the flag. `owned:
  false` = the stage *requires* a flag another map's story sets (Pallet's post-Parcel stage needs
  `EVENT_GOT_POKEBALLS_FROM_OAK`, which Oak's Lab owns). Constructing a state writes both — but the
  UI can warn that a context flag rewrites another map's story position.
- **Rolling backward** = writing the earlier stage's absolute `save` block (deltas are not
  invertible one-sidedly once branches exist; absolute is the rollback path).
- **Transient states ship but don't sit in `order`.** They are listed (full byte range, never
  refused, flagged in words) — rolling skips them; hand-picking one is the power path.
- **Every referenced ind is validated** against events.json / missables.json / scripts.json /
  maps.json at generation time; a dangling reference fails the build of the data, not the app.

## Phases

- **Phase MS-1 — Research + extractor + blueprints (THIS SESSION, 2026-07-17).** The deep dive,
  `extract_map_states.py`, curated overlay for the story maps, generated `map-states/*.json`,
  reference note, this plan. Data only — no model/UI.
- **Phase MS-2 — Model.** A `MapStatesDB` (qrc wiring, `db.qrc` gains the folder), deep-linked to
  MapsDB; `MapModel` surface: `stateList()`, `currentStateId()` (match the live save against
  blueprints), `applyState(id)`, `rollForward()`, `rollBack()` — each writing exactly the bytes the
  blueprint names. Byte-exact tests in the `tst_world`/`tst_area_state` idiom.
- **Phase MS-3 — Rename "map script" → "map state"** everywhere on the map screen (Map Storage
  panel section title, Details panel combo labels, docs). Wording only; the underlying
  `w<Map>CurScript` reference notes keep the game's own names.
- **Phase MS-4 — UI.** The state picker fed by blueprints (per the map-screen design language),
  roll ◀ ▶ controls, and the change-map flow: pick destination map → pick state (default: the
  stage matching global story position) → construct (copy blueprint save state + land on `entry`).
  Needs Twilight's design pass before build (standing rule).
- **Phase MS-5 — Randomizer hook.** State-aware map randomization: pick a random resting stage
  per map from the blueprints instead of raw bytes.
- **Phase MS-6 — Console verification.** Forge saves at constructed states (`emu_make_map_save` +
  blueprint) and prove on the cartridge that each story-map stage boots healthy and behaves as the
  stage says (the probe idiom). Gate MS-4's ship on the story maps passing.

## Open questions for Twilight

- Blueprint-driven **map change** default: when switching the current map, silently apply the
  destination's stage that matches global story progress, or always ask which state?
- Should rolling a map forward also advance **cross-map context flags** (the `owned: false` set)
  by default, or only with a confirmation?
- The transient steps: show them in the state dropdown (flagged), or behind "Something else…"?
