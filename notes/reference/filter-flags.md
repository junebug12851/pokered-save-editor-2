# Filter flags — the 228 missable lifecycles (researched 2026-07-19)

Briefed by Fairy Fox 2026-07-19: research each filter flag in detail — what it does, when it
usually flips in the game or on its map, what on the map reacts to it (Prof. Oak in Pallet Town
being her exemplar), and whether scripts or event flags drive the same machinery. Evidence:
`scripts/analyze_filter_flags.py` → `tmp/filter-flags/filter_flag_dossiers.json` (per-flag:
default, object, every Show/Hide call site with its routine, the blueprint stages that flip it,
class). Sister notes: [`map-scripts-missables.md`](map-scripts-missables.md) (the save layout),
[`map-states.md`](map-states.md) (per-stage visibility), [`game-progression.md`](game-progression.md)
(the global order).

## The model — what a filter flag IS

One bit per **toggleable object** (`wToggleableObjectFlags`, save `0x2852`, bit set = HIDDEN).
The bit is the *rendered truth* — the overworld draws the object iff its bit is clear — but it
is rarely the *durable* truth: **for most story objects the durable truth is an event flag, and
map entry scripts RE-DERIVE the bit from it.** That is the answer to "do scripts or event flags
also do this": yes — the three systems form one chain, `event flag (durable fact) → map script
(the reconciler, often per entry) → filter flag (what the renderer consults)`. Editing a filter
flag alone therefore often only lasts until the owning map's script next runs; editing the
EVENT is what the game treats as the fact. (The map-states blueprints encode exactly this: a
stage's save block carries script byte + events + the map's own missable bits together.)

## The five classes (228 = 121 + 44 + 36 + 14 + 13)

### 1. STATIC — 121 flags, never script-toggled (all default ON)

Pret's X-marks: **items on the ground and one-shot static encounters** (Snorlax excepted, see
one-shots). No script ever touches them — when you pick the item up / beat the encounter, the
**engine** records it by setting the hide bit directly (the `wToggleableObjectList` path). One
flip, ever, engine-owned. For state matching these are per-item "collected" bits, not story
stages.

### 2. SWEEP — 44 flags, all flipped by ONE script

`SilphCo11FTeamRocketLeavesScript` (the liberation): beating Giovanni hides every rocket +
scientist on Silph 2F–11F and swaps Saffron's streets (7 rockets out, 6 civilians in) in one
`db TOGGLE_*` table loop. One durable event (`EVENT_BEAT_SILPH_CO_GIOVANNI`) owns all 44 bits'
story meaning.

### 3. ONE-SHOT — 36 flags, a story moment flips them once and leaves them

Each has an exact trigger routine (full list in the dossiers). The shapes:

- **Swap pairs** (hide A + show B at one moment): Viridian's lying-old-man → standing old man
  (Oak gives the Pokédex!); Daisy sitting → walking (Pallet's script, once the lab intro is
  done); Bill's-house Pokémon → Bill (the cell separator); Cerulean's rocket → guard-1/guard-2
  swap (the burglary, armed by Bill's S.S.-Ticket text).
- **Gift/choice removals**: the three starter balls, both Mt. Moon fossils (the branch), the
  dojo's two gift balls (the other one vanishes), Eevee's ball, Old Amber, TM/Pokédex props.
- **Departures**: tower/Silph/gym bosses and rivals leaving after their beat (Giovanni ×2,
  tower rival, Silph 7F rival, Game Corner rocket, Nugget-bridge recruiter), Mr. Fuji tower →
  home (Tower 7F flips his two bits AND Saffron's E/F pair), the Cerulean-cave guard hidden by
  the **Hall of Fame**.
- ⚠️ **The two Snorlax** (`TOGGLE_ROUTE_12/16_SNORLAX`, default ON) are hidden by their route's
  own `DefaultScript` **checking the beat-event on entry** — the derive-on-entry pattern in its
  purest form: the console re-decides the bit every time the route loads.

### 4. RE-ARMED — 14 flags, rewritten from OUTSIDE per approach

The Victory Road pair (Route 23 hides VR2F's boulder + shows VR3F's on every load; VR3F itself
swaps them once its switches are held) and the twelve Seafoam boulders (each floor drops
boulders into the floor below; Route 20 reconciles the whole island, keyed on the
`*_DOWN_HOLE` events, when you emerge). A save's copy of these bits is only meaningful inside
the puzzle — [`game-progression.md`](game-progression.md) §1a.

### 5. CUTSCENE / PER-ENTRY VISITORS — 13 flags, shown AND hidden by scripts

The class Oak's street sprite heads (her exemplar, full lifecycle below). Also: **Pewter's two
dragger NPCs** (museum guy + gym guy — the city's entry script RESETS them shown each visit
until their story flags say otherwise, then hides them mid-drag), the Cerulean rival
(shown/hidden around the Nugget-bridge battle), the S.S. Anne 2F rival, Champion-room Oak
(walks in, walks out), the Route 22 rivals (armed by Oak's Lab / Viridian Gym, disarmed by
Pewter Gym / their own exit), the Silph 1F receptionist, the Bill pair's transient halves.

## The exemplar — TOGGLE_PALLET_TOWN_OAK, start to finish

- **Default OFF (hidden).** A fresh save has no Oak on the street.
- **Shown** by `PalletTownOakHeyWaitScript` — the "OAK: Hey! Wait!" intercept when you first
  step toward the grass (Pallet stage 1's exit cutscene, transient steps 1.1–1.4).
- **Walks you to the lab**, then **hidden again by the ENGINE** — `PalletMovementScript_Done`
  in `engine/overworld/auto_movement.asm`, not by any map script (the trap the map-states
  research recorded: a scripts/-only reading leaves Oak standing in Pallet forever).
- **Every resting stage of Pallet Town (1, 2, 3) therefore holds the bit HIDDEN.** The bit is
  set (visible) ONLY inside the cutscene window — a save mid-cutscene is the only legitimate
  save where Oak renders on the street.
- What reacts on the map: the street Oak **sprite**. In the editor, rolling Pallet's stages
  keeps him ghost-hidden throughout (correct); hand-clearing his filter flag shows him — and
  the game will re-hide him only if the cutscene's engine path runs again.

## What it means for OUR code

- **State matching weighs missables correctly but must weigh EVENTS first** — for the
  derive-on-entry flags (Snorlax, Cerulean rival, Silph receptionist, the re-armed boulders)
  the bit follows the event, so the event is the giveaway and the bit is echo.
- **The canvas must re-render from the missable bits whenever a state is applied/rolled** —
  the filter-flagged ghost view IS these bits' rendering. (Verified this session — the
  storageBlocks/sprite chain keys on the model revision, which `applyState` bumps.)
- **Editing UX truth-telling:** a filter-flag edit on a derive-on-entry object should be
  understood as cosmetic-until-next-load; the World panel's per-flag blurbs already carry the
  flag↔event links where verified (the 14), and the dossiers now name every script site.
- **Raw "Noop"-style steps are often just the parked script** — a map whose work is done rests
  there; the byte alone is not a stage, which is why stages carry events + missables too.
