# The "Map" page — the seven leftover AreaMap state bytes

**Status:** researched 2026-07-15, **verified against the cartridge**
(`scripts/emu/probe_area_map_state.py`). Read this before adding the extra fields to the
map-details panel (the panel shown when nothing is selected). The design that consumes it
is [`../plans/map-screen.md`](../plans/map-screen.md) → **Phase 8 (Area State)**.

> **Read first:** [`gen1-knowledge.md`](gen1-knowledge.md) (the save format),
> [`warps.md`](warps.md) (the `BIT_NO_PREVIOUS_MAP` linchpin — it decides the view pointer too),
> [`player-state.md`](player-state.md) (the sibling "which bytes survive a Continue" pass),
> [`emulator-verification.md`](emulator-verification.md) (how the console gets asked).

---

## 0. The one-paragraph version

`AreaMap` is the structural core of an Area — map id, size, the data/text/script pointers,
the edge connections. Most of it is obviously-meaningful and already on the screen. What was
*left over* is a cluster of seven fields from v1's "Map" page that nobody had explained:
**Current Script, two "upper-left corner" pointers, "Run cur map script instead", "force bike
ride", "to blackout dest", and "card key door X/Y."** They turn out to be a mix of **durable
editable state (2), a derived value (1), live/scratch bytes the game rewrites on every load
(3), and one ghost that has already moved to another panel (1).** The console settled every
case, and it overturned two source-reads on the way.

**The address maths (same as warps/player):** the save's Main Data block starts at file
offset `0x25A3` and maps to `wMainDataStart` = `$D2F7`, so

> **`WRAM address = save offset + 0xAD54`**

---

## 1. The complete table

Legend: **`!`** = **rewritten/reset on every load** (the amber-`!` treatment).
**`≈`** = **derived** (the game computes it from other state; show it, offer recompute, never
silently sync). **`↔`** = **transient handoff** (kept across load, but consumed the moment the
right script runs). No mark = durable state a person can edit and keep.

| | Save | Bit | Real WRAM name | v1 label / v2 field | Plain-English | On load (console-verified) |
|---|---|---|---|---|---|---|
| | `0x2CE5` | — | `wCurMapScript` | Current Script / `curMapScript` | **Working script-step index** (pairs with the bit below) | **kept** |
| ≈ | `0x260B` | — | `wCurrentTileBlockMapViewPointer` | UL Corner of Cur View in Tile Blocks / `currentTileBlockMapViewPointer` | **Camera: which block is top-left** (derived from coords) | **kept — but trusted blindly** |
| **!** | `0x27D2` | — | `wMapViewVRAMPointer` | UL Corner visible BG Tilemap in VR / `mapViewVRAMPointer` | **Where the screen sits in VRAM** | **reset to `$9800`** |
| ↔ | `0x29DF` | 4 | `BIT_USE_CUR_MAP_SCRIPT` (`wStatusFlags7`) | Run cur map script instead / `curMapNextFrame` | **"Resume at the saved script step"** | **kept on a quiet map; consumed on a scripted one** |
| | `0x29DE` | 5 | `BIT_ALWAYS_ON_BIKE` (`wStatusFlags6`) | force bike ride / `forceBikeRide` | **Locked on the bike (Cycling Road)** | **kept** |
| **!** | `0x29EB` | — | `wCardKeyDoorY` | card key door Y / `cardKeyDoorY` | **Card-Key door tile Y (scratch)** | **zeroed** |
| **!** | `0x29EC` | — | `wCardKeyDoorX` | card key door X / `cardKeyDoorX` | **Card-Key door tile X (scratch)** | **zeroed** |
| — | `0x29DE` | 6 | `BIT_ESCAPE_WARP` (`wStatusFlags6`) | **to blackout dest** / *(moved)* | **Ghost — already relocated** (see §3) | n/a |

---

## 2. What each one actually is

### `wCurMapScript` — "Current Script" — **a working index, and half of a pair** ✅

`wram.asm`: *"index of current map script, mostly used as index for function pointer array;
mostly copied from map-specific map script pointer and written back later."* It is **which
step of a map's scripted-event sequence runs** — Pokémon Tower, the Rocket Hideout, every Gym,
Silph Co each carry a small numbered list of steps
(`SCRIPT_POKEMONTOWER6F_DEFAULT` / `_START_BATTLE` / `_END_BATTLE` / `_PLAYER_MOVING` /
`_MAROWAK_BATTLE`, …). **Console: the byte at `0x2CE5` is kept** — write a marker, Continue,
read it back unchanged.

**But mind the mechanism — it changes what editing it *does*.** `RunMapScript` jumps through
`wCurMapScriptPtr` into the current map's `<Map>_Script` routine, and that routine reads its
**own per-map variable** (`w<Map>CurScript` — `wPokemonTower6FCurScript`, `wOaksLabCurScript`,
… **~90 of them**, one per scripted map, in the saved event-progress block ~`$D5F5`–`$D65C`),
runs the indexed sub-script via `ExecuteCurMapScriptInTable`, and writes it **back to the
per-map variable.** So:

- **The durable "story progress" for a map is its `w<Map>CurScript`, not `wCurMapScript`.** That
  ~90-byte per-map block is a **separate, un-briefed topic** (game-progress state) — see §12b of
  the plan. This field (`0x2CE5`) is not that block; do not build the block's UI here.
- **`wCurMapScript` (`0x2CE5`) is the shared *working* index** that `ExecuteCurMapScriptInTable`
  writes each time a map script runs. Edited alone, a scripted map overwrites it on the next tick
  from its per-map variable — so on its own it's near-scratch.

> 🔗 **The pair.** `ExecuteCurMapScriptInTable` opens with `bit BIT_USE_CUR_MAP_SCRIPT / res …`,
> and **if that bit is set it uses `wCurMapScript` as the index instead of the map's default.**
> So **"Current Script" (`wCurMapScript`) and "Run cur map script instead"
> (`BIT_USE_CUR_MAP_SCRIPT`, §"Run cur map script instead") are one feature in two bytes:** set
> the bit **and** the index, and the map runs *that* script step next tick — the auto-trigger.
> The panel should present them together: the step selector plus a "run this step on load" toggle.

> **UI (Twilight, 2026-07-15):** a **ComboBox with an actually descriptive list** — the named
> steps for *the current map* — plus a **"Something else…" link** for a raw value. **The data:**
> `pret/pokered` has **98 `_ScriptPointers` tables** (`def_script_pointers` + `dw_const <routine>,
> SCRIPT_<MAP>_<NAME>`); parsing them yields the ordered, named step list per map — the same
> extraction shape as the sign-text pass (Phase 6a), **additive to `maps.json`**. Owed as its own
> data pass (ask before touching `maps.json`) before the combo can be truly descriptive; until
> then it falls back to raw indices + "Something else…".

### `wCurrentTileBlockMapViewPointer` — "UL Corner of Cur View in Tile Blocks" — **derived** ≈

The address, inside the map's block buffer, of the **top-left block currently on screen** — the
camera, expressed as a pointer. It is **computed from the player's coordinates** and the map
width (`AreaMap::coordsToPtr()` already reproduces the formula; `tst_map`'s
`viewPointer_matchesWhatTheGameStored` proves ours matches the console's byte-for-byte).

> ⚠️ **The surprise the probe caught.** A source read says "it's recomputed every load, so
> editing it is pointless." **Wrong.** The recompute lives in `CheckMapConnections` (a *step*
> across a map edge) and in warp-arrival — **neither runs on Continue**, and `LoadMapHeader`
> bails out early on Continue (`BIT_NO_PREVIOUS_MAP`, the warp linchpin). So the console
> **trusts the save's pointer as-is.** Writing `0xFFFF` here made the real cartridge draw the
> screen out of address `$FFFF` — **on-screen garbage** (`tmp/emu-areamap/areamap.png`). It is
> corrected only when you take a step.

So this is the textbook **derived byte** — and the doctrine (clarified by Twilight 2026-07-15,
CLAUDE.md → *"kept in sync by default; power users can break sync"*) is **NOT** "show a mismatch
and make the user press Sync." It is:

- **Synced by default.** The camera pointer tracks the player's coordinates automatically — move
  the player, the view box follows. Most people editing the Map want exactly this, and it would be
  bad UX to let a novice break their map because they didn't *also* hand-edit the pointer they had
  no reason to touch.
- **Power users can break sync.** The pointer *may* legitimately differ from the player (the
  console trusts it, remember). So provide a **break-sync toggle**; and if the user types a
  different pointer value while synced, raise an **alert offering to break sync**. Once desynced,
  the source stops driving it and it stands alone.
- **The view box is its own movable thing on the canvas.** Because the box renders from *the
  pointer*, not the player, a desynced box can be **dragged around the map with the mouse** — a
  real, welcome capability (it surprised even Twilight).

> **UI (Twilight, 2026-07-15):** *not* an address field in the normal case. Show it as an
> **intuitively selectable camera / view box** that follows the coordinates by default, with a
> break-sync affordance and canvas dragging when desynced; a raw-pointer entry only behind a
> **"Something else…" link** for the power/hack path.

### `wMapViewVRAMPointer` — "UL Corner visible BG Tilemap in VR" — **reset every load** !

`wram.asm`: *"the address of the upper left corner of the visible portion of the BG tile map
in VRAM."* It is a **pure runtime rendering pointer** that scrolls around VRAM as you walk.
**`LoadMapData` sets it to `$9800` (`vBGMap0`) unconditionally on every map load** — console
confirmed: wrote `0xFFFF`, read back `0x9800`. It is **not persistent state**; editing it
accomplishes nothing. If it appears at all it belongs behind the "Something else…"/advanced
disclosure with a note that the game resets it to `$9800`.

### `BIT_USE_CUR_MAP_SCRIPT` — "Run cur map script instead" — **transient handoff** ↔

`wStatusFlags7` bit 4. Meaning: *"the next time a map script runs, use `wCurMapScript` as the
index instead of the default one."* `ExecuteCurMapScriptInTable` reads it and **immediately
clears it** — it is a **one-shot flag** the trainer engine sets so a post-battle script resumes
at the right step (`home/trainers.asm`: `set BIT_USE_CUR_MAP_SCRIPT` before a battle; the map's
next tick consumes it). **Console: kept on Pallet** (a quiet map never routes through
`ExecuteCurMapScriptInTable`, so the bit survives), but it would be consumed on the first tick
of a scripted/trainer map.

> **Not dead — a feature (Twilight, 2026-07-15).** Because it *survives* load on a quiet map, a
> save can carry it in **already set**, which means the next map-script tick runs `wCurMapScript`
> instead of the default step — i.e. it can be used to **auto-trigger a chosen map script on
> load**. So the panel should present it as an editable, meaningful toggle ("resume/run this
> map's saved script step on load"), paired with the `wCurMapScript` selector — not hide it as
> plumbing. On a scripted map it is consumed on the first tick (say so), but on the maps where it
> persists it is a genuine lever.

### `BIT_ALWAYS_ON_BIKE` — "force bike ride" — **durable, meaningful, editable** ✅

`wStatusFlags6` bit 5. You are **locked on the bike** — the Cycling Road. Set when you enter
Cycling Road (`player_state.asm`), cleared at its gates (`Route16Gate1F` / `Route18Gate1F`
`res BIT_ALWAYS_ON_BIKE`). Nothing on the load path touches it — in fact `oak_speech.asm`
carries a **BUG note** that it *wrongly* persists into a brand-new game, which is hard proof it
survives a load. **Console: kept.** Genuinely editable state. Good name: **"Always on bike
(Cycling Road)."**

### `wCardKeyDoorY` / `wCardKeyDoorX` — "card key door x/y" — **scratch, zeroed every load** !

The tile coordinates of the door you just used the **Card Key** on. `card_key.asm` records the
door tile here; the **Silph Co. floor scripts** (`SilphCo2F` … `11F`) read `wCardKeyDoorY` to
open that exact door; then **`ClearVariablesOnEnterMap` zeroes both on every map entry**
(`ld hl, wCardKeyDoorY / ld [hli], a / ld [hl], a`). **Console: both read back `0`.** Pure
momentary scratch — not persistent state, editing them is pointless. Behind the advanced
disclosure at most, labelled as game scratch.

---

## 3. The "to blackout dest" ghost — already handled, do NOT re-add it here

v1's "Map" page listed **"to blackout dest"** as a bool. It is **`BIT_ESCAPE_WARP`**
(`wStatusFlags6` bit 6) — Dig, Escape Rope and blacking out — **not a destination and not
unused.** It was **moved out of `AreaMap` to `AreaWarps::escapeWarp` on 2026-07-14** (see the
comment block in `areamap.h`/`.cpp` and [`warps.md`](warps.md) → §3 bug 1). Two owners writing
one bit is how a save gets quietly corrupted, so it lives on the **warp panel** now, under its
real name.

And the thing the label *sounds* like — the actual **blackout destination map** — is a whole
different byte, **`wLastBlackoutMap`** (`0x29C5`), which lives in **`WorldGeneral`** and is
already surfaced per [`warps.md`](warps.md) → §4. **Nothing about blackout belongs on the Map
page.** If Twilight wants it shown here, it *reads* the warp/world field; it does not re-own it.

---

## 4. How the panel should carve this up

Three honest buckets:

- **Editable state** — **Map script step** (`wCurMapScript`, descriptive ComboBox + "Something
  else…"), **Run saved script step on load** (`BIT_USE_CUR_MAP_SCRIPT`, a toggle paired with the
  step selector — it auto-triggers that script on load on the maps where it persists), and
  **Always on bike** (`BIT_ALWAYS_ON_BIKE`, a toggle). The real, durable, meaningful levers.
- **Derived, synced by default** — **Camera / view box** (`wCurrentTileBlockMapViewPointer`):
  tracks the coordinates automatically, with a **break-sync** toggle + an alert on manual entry +
  **canvas dragging** for the power path; a raw address only behind "Something else…". Not a bare
  address in the normal case.
- **Rewritten on load / scratch, collapsed behind a disclosure** — **VRAM view pointer**
  (`→ $9800`) and **Card-Key door X/Y** (`→ 0`). Each wears the amber **!** with its own reason,
  exactly like the player panel's *Reloaded values* group.

Every value stays **full-range and editable, hack values included** — the project rule. The
panel never refuses a value and never silently rewrites one; it only tells the truth about what
the console does with it on the next load.

---

## 5. The console's testimony

`scripts/emu/probe_area_map_state.py` stamps markers into all seven values, seals the checksum,
boots the **real ROM** (Pallet Town fixture, ordinary Continue), and reads back. Verbatim:

```
===== Area-map state on an ordinary Continue (Pallet Town fixture) =====
  word fields (view pointers):
    wCurrentTileBlockMapViewPointer wrote 0xFFFF  read 0xFFFF   kept          <- trusted; drew GARBAGE
    wMapViewVRAMPointer             wrote 0xFFFF  read 0x9800   DERIVED/RESET
  byte fields:
    wCardKeyDoorY                   wrote 0x11  read 0x00   REWRITE
    wCardKeyDoorX                   wrote 0x12  read 0x00   REWRITE
    wCurMapScript                   wrote 0x03  read 0x03   kept
  bit fields:
    SF6.b5 forceBikeRide (ALWAYS_ON_BIKE)   wrote 1  read 1   kept
    SF7.b4 curMapNextFrame (USE_CUR_SCRIPT) wrote 1  read 1   kept
```

> ⚠️ **The probe earned its keep — twice.** A source read predicted the **view pointer** would
> be *recomputed* on load (it is trusted) and `BIT_USE_CUR_MAP_SCRIPT` would be *cleared* (it
> survives on a quiet map). Both wrong from the assembly alone; the cartridge settled both. Same
> lesson as the sprite pass ([`sprites.md`](sprites.md) → Part 5): the console is the oracle.

The `wCurrentTileBlockMapViewPointer` result is confirmed by the garbage framebuffer
`tmp/emu-areamap/areamap.png` — a valid pointer would have drawn Pallet Town.

---

## 6. Bugs / follow-ups this found

| | Item | Action |
|---|---|---|
| 1 | v2 field docs call `cardKeyDoorX/Y` *"Unknown ???"* and `curMapNextFrame`/`forceBikeRide` *"Flags that may not be used, unknown"* | **Rename + document** with the real names above (truth-in-labelling, like the player pass). No offset/bit is wrong — only the words. |
| 2 | `wCurMapScript` needs a **descriptive per-map script-step list** for the ComboBox | ✅ **DONE (2026-07-15):** `scripts/import_map_scripts.py` imports the `SCRIPT_<MAP>_<NAME>` steps from `pret/pokered`'s `_ScriptPointers` tables into a new **`scriptEntries`** field on each scripted map in `maps.json` — **additive-only** (0 lines removed), self-validating, `--check`-idempotent. **116 map ids, 458 steps** (e.g. Pallet Town → Default / Oak Hey Wait / Oak Walks To Player / … / Daisy / Noop). Each entry is `{id (0-based, the `wCurMapScript` value), name, label}`. The combo reads it; unscripted maps have none → raw index + "Something else…". |
| 2b | `wCurMapScript` (`0x2CE5`) is a **shared working index**, not the durable per-map progress | The per-map progress is the **~90 `w<Map>CurScript` bytes** (`$D5F5`–`$D65C`), a **separate, un-briefed** game-progress topic (plan §12b). Present `0x2CE5` as the *"run this step next tick"* index, paired with `BIT_USE_CUR_MAP_SCRIPT`; do **not** build the per-map block's UI here. |
| 3 | `wCurrentTileBlockMapViewPointer` is derived and **trusted on load** | **Keep it synced to the coords by default** (auto-recompute via `coordsToPtr`); give power users a **break-sync** toggle + an alert on manual entry + **canvas dragging** of the view box. Raw address only behind "Something else…". |
| 4 | `mapViewVRAMPointer`, `cardKeyDoorX/Y` are not real editable state | Collapse behind the disclosure with the amber **!** and a plain reason. (`curMapNextFrame` is **not** here — it's an editable auto-trigger lever, see §2.) |

`AreaMap::save()` writes the correct bits to the correct addresses — there is **no
save-corruption bug** here (unlike the tileset `collPtr` or the sprite-mobility inversion), and
`setTo()`/`randomize()` only reconfigure the map, so **no loaded gun** like `AreaWarps::setTo()`.
It is a naming/organisation problem plus one derived-value discipline.

---

## 7. Sources

| What | Where |
|---|---|
| The seven WRAM labels + comments | `ram/wram.asm` (`wCurrentTileBlockMapViewPointer`, `wMapViewVRAMPointer`, `wCardKeyDoorY/X`, `wCurMapScript`) |
| `BIT_ALWAYS_ON_BIKE`, `BIT_USE_CUR_MAP_SCRIPT`, `BIT_ESCAPE_WARP` constants | `constants/ram_constants.asm` |
| The load path (`EnterMap` → `LoadMapData` → `ClearVariablesOnEnterMap`) | `home/overworld.asm`, `engine/overworld/clear_variables.asm` |
| VRAM pointer reset to `$9800` | `home/overworld.asm` → `LoadMapData` |
| The view-pointer recompute lives off the Continue path | `home/overworld.asm` → `CheckMapConnections`; `LoadMapHeader` `ret nz` on `BIT_NO_PREVIOUS_MAP` |
| `BIT_USE_CUR_MAP_SCRIPT` read/consumed, and `wCurMapScript` used as the index when it's set | `home/trainers.asm` → `ExecuteCurMapScriptInTable` |
| How a map's script runs (`wCurMapScriptPtr` → `<Map>_Script` → its own `w<Map>CurScript`) | `home/overworld.asm` → `RunMapScript`; `scripts/*.asm` (e.g. `PokemonTower6F.asm`) |
| The ~90 per-map `w<Map>CurScript` durable-progress bytes | `ram/wram.asm` |
| The 98 named per-map script-step tables (combo source) | `scripts/*.asm` → `def_script_pointers` / `dw_const …, SCRIPT_<MAP>_<NAME>`; `constants/script_constants.asm` |
| `BIT_ALWAYS_ON_BIKE` persists into a new game (bug note) | `engine/movie/oak_speech/oak_speech.asm` |
| Card-Key door coords set/read/cleared | `engine/events/card_key.asm`, `scripts/SilphCo*.asm`, `engine/overworld/clear_variables.asm` |
| The whole thing, byte by byte | `scripts/emu/probe_area_map_state.py` (local-only, ROM-gated) |
