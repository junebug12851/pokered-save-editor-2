# Game progression — the global map graph (researched 2026-07-19)

Briefed by Fairy Fox 2026-07-19: the map-states pass modelled each map's *own* timeline but never
put those timelines into one **global order** — her bet was that maps later in the game **unset
flags** the per-map view assumed were durable, that map access depends on *other* maps' progress
(Saffron being the memory), and that the whole thing needs one big graph: every route from 0% to
100%, per map and for the game, with the expected flag state at each point.

**The bet is confirmed, and the class is bigger than suspected.** This note is the graph: the
cross-map write matrix, the gates, the mandatory spine, the free windows, and the
expected-flag-state-by-stage table. Evidence: `scripts/analyze_cross_map_writes.py` →
`tmp/event-flags/cross_map_writes.json` (regenerable; all file/line citations below are
`pret/pokered`). Sister notes: [`map-states.md`](map-states.md) (per-map blueprints),
[`event-flags.md`](event-flags.md) (the 2,560-bit field), [`world-completed.md`](world-completed.md)
(the milestone one-shots).

## 1. The headline: progress is MONOTONE except for exactly 70 flags

Of 2,560 event flags, **only 70 are ever reset by anything**. Everything else is
set-once-never-cleared — the game's progression is overwhelmingly monotone, which is what makes a
global ordering (and a progress metric) possible at all. The 70 exceptions split into **five
classes**, and knowing the class tells you exactly how far to trust a bit:

### 1a. Per-entry RE-ARM scripts — the class the per-map view cannot see ⚠️

Whole puzzles are **reset from OUTSIDE, every time you approach them**. A save's copy of these
flags is only meaningful while the player is standing inside the puzzle:

| Re-armer (writer) | What it rewrites | When |
|---|---|---|
| **Route 23** (`Route23SetVictoryRoadBoulders`, scripts/Route23.asm:8) | Resets ALL FOUR `EVENT_VICTORY_ROAD_2/3_BOULDER_ON_SWITCH1/2`, **shows** VR3F's boulder, **hides** VR2F's | once per map load (`BIT_CUR_MAP_LOADED_2` guard) |
| **Indigo Plateau Lobby** (scripts/IndigoPlateauLobby.asm) | Resets `EVENT_VICTORY_ROAD_1_BOULDER_ON_SWITCH` | on entry |
| **Victory Road 2F** | also resets VR1F's switch; **VR3F shows VR2F's boulder** | on entry |
| **Route 20** (`Route20BoulderScript`) | Reconciles Seafoam's boulders — keyed on `EVENT_IN_SEAFOAM_ISLANDS` (set by Seafoam 1F, cleared when you emerge onto Route 20) + the two `*_DOWN_HOLE` pairs: a **finished** side re-shows the 1F pair and hides the six intermediate-floor boulders; an **unfinished** side is simply forgotten | on stepping out of the islands |
| **Seafoam floors** | each floor toggles the floor **below**'s boulders as they fall through | during play |
| **Pewter City** (scripts/PewterCity.asm:20) | Resets `EVENT_BOUGHT_MUSEUM_TICKET` — **the museum ticket is per-visit** | every entry |
| **Celadon City** (scripts/CeladonCity.asm:3) | Zeroes THREE unnamed gap bits every entry: `EVENT_1B8`, `EVENT_1BF` (Celadon block), `EVENT_67F` (Rocket-Hideout block) — **event bits used as per-visit scratch** | every entry |
| **Safari gate / engine** | `EVENT_IN_SAFARI_ZONE` / `EVENT_SAFARI_GAME_OVER` set/cleared by the gate + `engine/items/item_effects.asm` | per game |
| **Mansion floors + Cinnabar Island** | `EVENT_MANSION_SWITCH_ON` toggled by 1F/2F (the switches) and cleared by the island (leaving resets it) | during play / on exit |

**Consequences.** (1) The auto-derived map-state blueprints for Victory Road 1F/2F/3F and the
Seafoam floors present the switch/boulder states as durable progress — they are not; any state
built on them is rewritten by the *approach road* before the player can stand in it again.
(2) "Boulder on switch" flags, the museum ticket, the Celadon trio, Safari state and the Mansion
switch must all read **expected-zero at rest** for state matching, progress metrics and the
randomizer. (3) ⚠️ probe-worthy: whether the Route 23 re-arm also fires on a **Continue that
starts on Route 23** (the guard is a map-script-flags bit, not the `BIT_NO_PREVIOUS_MAP`
linchpin) — if yes, even a save made mid-gauntlet gets its puzzle re-armed.

### 1b. Story ARM / DISARM — one-shot cross-map rewiring (the Route 22 pattern)

- **Oak's Lab** arms ambush 1 (`SetEvent EVENT_1ST_ROUTE22_RIVAL_BATTLE` +
  `EVENT_ROUTE22_RIVAL_WANTS_BATTLE`, shows `TOGGLE_ROUTE_22_RIVAL_1`) — and *resets*
  `EVENT_2ND_ROUTE22_RIVAL_BATTLE` while it's at it.
- **Pewter Gym** DISARMS ambush 1 if it never happened (`ResetEvents` both flags + hides the
  rival sprite, scripts/PewterGym.asm:72) — beating Brock closes the window forever.
- **Viridian Gym** arms ambush 2 (`SetEvents EVENT_2ND_ROUTE22_RIVAL_BATTLE, …WANTS_BATTLE` +
  shows `TOGGLE_ROUTE_22_RIVAL_2`, scripts/ViridianGym.asm:164).

Same pattern elsewhere: **Oak's Lab** swaps Viridian's lying-old-man for the standing one on
parcel delivery; **Pallet Town** swaps Daisy sitting/walking in Blue's House; **Bill's House**
flips Cerulean's two guards (the break-in arms only after you've met Bill); **Pokémon Tower 7F**
shows Mr. Fuji at home AND swaps two Saffron townsfolk (`TOGGLE_SAFFRON_CITY_E`→hide,
`_F`→show); **Hall of Fame** hides Cerulean's cave guard (the post-game gate opens).

### 1c. The Silph liberation — the biggest single cross-map write in the game

`SilphCo11FTeamRocketLeavesScript` (scripts/SilphCo11F.asm:75), on beating Giovanni: one loop
**hides 34 objects across eleven maps** (every rocket + scientist on Silph 2F–11F, plus Saffron
City's seven street rockets + two more) and **shows six** (Saffron's civilians,
`TOGGLE_SAFFRON_CITY_8`–`_D`). One event (`EVENT_BEAT_SILPH_CO_GIOVANNI`) plus this sweep IS the
"Saffron occupied → Saffron liberated" state change Twilight remembered. ⚠️ It lives in
**`db TOGGLE_*` data tables**, not `ld a, TOGGLE_*` instructions — a mining pass that only greps
the `ld` form (as this one's first cut did) sees *none* of it.

### 1d. The Hall-of-Fame WIPE (already known, restated for the ordering)

`ResetEventRange INDIGO_PLATEAU_EVENTS_START, INDIGO_PLATEAU_EVENTS_END` — all 40 Indigo bits
(door one-shots, beat flags, `EVENT_BEAT_LANCE`, `EVENT_BEAT_CHAMPION_RIVAL`, the door lock)
cleared so the gauntlet re-arms. Indigo Plateau Lobby also clears stragglers on entry. **A 100%
save shows the E4 range EMPTY** — set bits there mean "mid-run", not "beaten"; the durable record
of victory is the Hall of Fame roster + `wNumSetBits`-style counters, not these flags.

### 1e. Dialog / mechanism scratch (harmless, but not progress)

Fan-club boasts (`EVENT_SEEL_FAN_BOAST`/`PIKACHU…` — reset each conversation), the Pokémon Tower
purified-zone bit (set/reset as you cross the tile), `EVENT_HALL_OF_FAME_DEX_RATING` (set by the
credits, consumed by Oak's phone rating), Cinnabar Gym's `EVENT_2A7` door scratch, the fossil
handover trio (`GAVE_FOSSIL`/`STILL_REVIVING`/`HANDING_OVER` — a little state machine that
rewinds if your party is full), and the S.S. Anne dock-walk cutscene steps.

## 2. The gates — every requirement that joins the graph

The complete gate inventory (event flags · badges · key items · one save byte):

| Gate | Where | Mechanism (exact) |
|---|---|---|
| Route 1 → world | Pallet | none (Oak intercept is on the grass edge, auto) |
| Viridian → Route 2 north | Viridian | old man blocks until **`EVENT_GOT_POKEDEX`** path; sprite swapped on **parcel delivery** (Oak's Lab toggles `TOGGLE_LYING_OLD_MAN`→`TOGGLE_OLD_MAN`) |
| Pewter → Route 3 | Pewter | blocker walks you back until **`EVENT_BEAT_BROCK`** — Brock is unskippable |
| Nugget Bridge → Bill | Route 24/25 | rival + trainers only; Bill gives **S.S. Ticket** (`EVENT_GOT_SS_TICKET`) |
| Cerulean → Route 9 | Route 9 | **Cut** tree (Cut = HM01 from S.S. Anne captain; usable with **Cascade badge**) |
| Vermilion dock | Vermilion | guard: **S.S. Ticket in bag**, and `EVENT_SS_ANNE_LEFT` closes the ship forever (departure triggers on walking off the dock with `EVENT_GOT_HM01`) |
| Vermilion Gym | Vermilion | **Cut** tree in front |
| Saffron (all four gates) | Routes 5/6/7/8 gates | **`wStatusFlags1` bit `BIT_GAVE_SAFFRON_GUARDS_DRINK`** — any vending-machine drink (Celadon rooftop), one drink opens all four |
| Rock Tunnel | Route 10 | dark only — **Flash optional** (Boulder badge to use) |
| Celadon Gym / Erika | Celadon | open (a Cut tree on one approach) |
| Rocket Hideout | Celadon Game Corner | poster rocket → `EVENT_FOUND_ROCKET_HIDEOUT`; **Lift Key** for the elevator (`IsItemInBag LIFT_KEY`); B4F holds the **Silph Scope** |
| Pokémon Tower ascent | Lavender | Ghost Marowak can't be identified (engine checks **SILPH_SCOPE**) — hard-blocks 7F |
| Mr. Fuji → Poké Flute | Tower 7F → his house | `EVENT_RESCUED_MR_FUJI` → `EVENT_GOT_POKE_FLUTE` |
| Snorlax ×2 | Routes 12/16 | **Poké Flute used** (engine sets `EVENT_FIGHT_ROUTE12/16_SNORLAX`) — opens both southern corridors |
| Cycling Road | Route 16 gate | **Bicycle in bag** (`IsItemInBag BICYCLE`; Bike Voucher ← Fan Club chairman, Vermilion) |
| Silph Co | Saffron | building open while occupied; locked doors need **Card Key** (5F pickup; `engine/events/card_key.asm`); Giovanni 11F → liberation sweep (§1c) |
| Saffron Gym | Saffron | street rocket at the door until the liberation sweep hides him (from the toggle lists; ⚠ worth a console spot-check) |
| Safari / Surf / Strength | Fuchsia | Warden: **Gold Teeth** → HM04; Safari Secret House: **HM03**; Surf usable with **Soul badge**, Strength with **Rainbow badge** |
| Seafoam traversal | Route 20 | current on B3F/B4F needs the boulder pairs dropped (**Strength**); flags re-reconciled by Route 20 (§1a) |
| Cinnabar Gym | Cinnabar | door: **Secret Key in bag** (from the Mansion) |
| Viridian Gym | Viridian | door opens when **`wObtainedBadges == all-but-Earth`** → `EVENT_VIRIDIAN_GYM_OPEN` (set by Viridian City's own entry script — the *city* watches your badges) |
| Route 22 gate | Route 22 | **Boulder badge** (`bit BIT_BOULDERBADGE`) |
| Route 23 gauntlet | Route 23 | **seven guards**, each `bit` the real badge in `wObtainedBadges` (Cascade, Thunder, Rainbow, Soul, Marsh, Volcano, Earth) then set a durable `EVENT_PASSED_<BADGE>_CHECK` — with Route 22's gate, **all 8 badges are hard-required** |
| Victory Road | inside | **Strength** + the boulder-switch puzzle (re-armed per approach, §1a) |
| E4 rooms | Indigo | each door locks behind you; each room checks its `EVENT_BEAT_*_ROOM_TRAINER_0` to unlock the far door — strict Lorelei→Bruno→Agatha→Lance→Champion order |
| Cerulean Cave | Cerulean | guard NPC until **Hall of Fame** hides him |
| Field moves | everywhere | Cut=**Cascade**, Fly=**Thunder**, Surf=**Soul**, Strength=**Rainbow**, Flash=**Boulder** (`engine/menus/start_sub_menus.asm`) |

## 3. The spine and the windows — routes from 0% to 100%

**The mandatory spine** (every run passes these, in this order — each is gated by the previous):

```
M0 New game (Pallet)
M1 Starter + lab rival           (Oak's Lab; arms Route 22 window #1)
M2 Parcel delivered → Pokédex    (opens Viridian north; old-man swap)
M3 Brock beaten                  (forced by the Pewter blocker; DISARMS window #1)
M4 Mt. Moon crossed              (fossil branch: Dome XOR Helix — the run's first permanent fork)
M5 Bill rescued → S.S. Ticket    (Nugget Bridge; arms the Cerulean break-in)
M6 Misty (Cascade)               (any time from Cerulean arrival until M8 needs Cut)
M7 S.S. Anne → HM01 Cut          (ship leaves when you walk off; Cut usable via Cascade)
M8 Cut opens the east            (Route 9 → Rock Tunnel → Lavender; Vermilion Gym tree)
   ── the map now fans out; the WINDOWS below open ──
M9 Rocket Hideout → Silph Scope  (via the Game Corner poster; Lift Key)
M10 Tower cleared → Fuji → Flute (needs Scope; populates Fuji's house, swaps Saffron folk)
M11 Snorlax moved                (Flute; opens Routes 12–15 and 16–18 → Fuchsia)
M12 Koga (Soul) → Surf usable    (HM03 + Gold Teeth→Strength from the Safari Zone)
M13 Silph cleared                (Card Key → Giovanni 11F; THE liberation sweep, §1c)
M14 Sabrina (Marsh)              (gym reachable after M13)
M15 Blaine (Volcano)             (Surf to Cinnabar via 19–20–21; Secret Key from the Mansion)
M16 Viridian Gym OPENS at 7 badges → Giovanni (Earth); arms Route 22 window #2
M17 Route 22 #2 (optional fight) → R22 gate + R23 gauntlet (all 8 badges shown)
M18 Victory Road (Strength + re-armed boulder puzzle)
M19 E4 in fixed order → Champion → HALL OF FAME (the wipe fires; cave guard leaves)
M20 Post-game: Cerulean Cave → Mewtwo   (the only content the ending unlocks)
```

**The windows (the non-linearity Twilight named):**

- **Route 22 #1** exists only between M1 and M3 — Pewter Gym erases it. **#2** only after M16.
  A save's Route 22 state is *meaningless* without knowing where you are on the spine.
- **Misty** can wait as late as you like — but Cut-usable (M8) needs her, so she fences the east.
- **Surge** any time after M7 (his tree needs Cut).
- **After M8 the graph is genuinely non-linear:** Lavender/Celadon/Saffron-drink order is free;
  Erika any time; the Hideout (M9) can precede or follow Erika; Fuchsia is reachable by EITHER
  snorlax corridor (12–15 on foot, 16–18 with the bike) — the bike is entirely optional.
- **Gym order freedom is real but fenced:** Erika/Koga/Sabrina/Blaine permute within their item
  gates (Sabrina ≥ M13; Blaine needs Surf ⇒ ≥ M12; Koga before any Surf-gated content).
- **Optional forever:** Flash, Fly, the bike, the Old Amber (Cut door in the Pewter museum),
  the Fan Club chain, Copycat's TM31 (Poké Doll), Mr. Psychic's TM29, the fighting Dojo (its
  gift is the run's second permanent fork: Hitmonlee XOR Hitmonchan), the fossil revival lab,
  Articuno/Zapdos/Moltres, the Safari's non-mandatory wings. "100%" is a *choice set*, not a
  bit-count.
- **Permanently closeable content:** the S.S. Anne (leaves at M7; Cut cannot be missed, the boat
  can), Route 22 #1 (closed by M3), the un-chosen fossil, the un-chosen Hitmon. A completion
  metric must treat these as "resolved", not "missing".

## 4. Expected flag state by spine position

What a legit save *should* look like at each stage — the durable spine markers (set at the stage,
never unset), plus what must read CLEAR:

| Stage | Newly-set durable markers (the tell) | Must ALSO be clear |
|---|---|---|
| M1 | `EVENT_GOT_STARTER`, `EVENT_BATTLED_RIVAL_IN_OAKS_LAB`, `EVENT_1ST_ROUTE22_RIVAL_BATTLE`(armed) | everything later |
| M2 | `EVENT_OAK_GOT_PARCEL`, `EVENT_GOT_POKEDEX`, `EVENT_GOT_POKEBALLS_FROM_OAK` | |
| M3 | `EVENT_BEAT_BROCK` (+ badge b0) | `EVENT_1ST_ROUTE22_RIVAL_BATTLE` + `…WANTS_BATTLE` **now clear** (either fought or disarmed) |
| M4 | `EVENT_BEAT_MT_MOON_*`, exactly ONE of `EVENT_GOT_DOME_FOSSIL` / `EVENT_GOT_HELIX_FOSSIL` | the other fossil flag, forever |
| M5 | `EVENT_MET_BILL`, `EVENT_USED_CELL_SEPARATOR_ON_BILL`, `EVENT_GOT_SS_TICKET` | |
| M6 | `EVENT_BEAT_MISTY` (+ badge b1) | |
| M7 | `EVENT_GOT_HM01`, `EVENT_SS_ANNE_LEFT`, `EVENT_WALKED_OUT_OF_DOCK` | |
| M9 | `EVENT_FOUND_ROCKET_HIDEOUT`, `EVENT_BEAT_ROCKET_HIDEOUT_GIOVANNI`, Scope in bag | |
| M10 | `EVENT_BEAT_GHOST_MAROWAK`, `EVENT_RESCUED_MR_FUJI`(+`_2`), `EVENT_GOT_POKE_FLUTE` | |
| M11 | `EVENT_BEAT_ROUTE12_SNORLAX` / `…ROUTE16…` (the FIGHT_* trigger bits clear again) | `EVENT_FIGHT_ROUTE12/16_SNORLAX` |
| M12 | `EVENT_BEAT_KOGA` (+ b4), `EVENT_GAVE_GOLD_TEETH`, `EVENT_GOT_HM03`, `EVENT_GOT_HM04` | Safari pair (`IN_SAFARI_ZONE`, `GAME_OVER`) at rest |
| M13 | `EVENT_BEAT_SILPH_CO_GIOVANNI` (+ `EVENT_GOT_MASTER_BALL`, floor beat-flags) | |
| M14 | `EVENT_BEAT_SABRINA` (+ b5) | |
| M15 | `EVENT_GOT_SECRET_KEY`, `EVENT_BEAT_BLAINE` (+ b6) | `EVENT_MANSION_SWITCH_ON` at rest |
| M16 | `EVENT_VIRIDIAN_GYM_OPEN`, `EVENT_BEAT_VIRIDIAN_GYM_GIOVANNI` (+ b7), `EVENT_2ND_ROUTE22_RIVAL_BATTLE`(armed) | |
| M17 | `EVENT_BEAT_ROUTE22_RIVAL_2ND_BATTLE`; the seven `EVENT_PASSED_*BADGE_CHECK` bits accumulate as guards are shown badges | |
| M18–19 | (E4 bits are all TRANSIENT mid-run) | **after the Hall of Fame the whole Indigo range reads CLEAR**; VR switches clear |
| M20 | Cerulean cave guard hidden (missable bit); Mewtwo static-encounter bit after the catch | |

**Always-expected-zero at any resting save, at any stage:** the four VR boulder switches + VR1F's,
the museum ticket, Celadon's `1B8/1BF/67F`, the Safari pair, `EVENT_MANSION_SWITCH_ON`,
`EVENT_IN_SEAFOAM_ISLANDS`, the purified-zone bit, both fan-boasts, the fossil-handover trio
mid-states, the S.S. Anne dock-walk steps, `EVENT_FIGHT_*_SNORLAX`, and (post-HoF or pre-E4) the
whole Indigo range. **A "wrong" bit here is not corruption — it's a save taken mid-mechanism.**

## 5. What it means for OUR code

- **Global progress metric:** count the spine markers (M1–M19 tells above) — they are monotone,
  console-owned, and unambiguous. Do NOT count the 70 resettable flags or the 2,023 placeholders.
  Optional content contributes a separate "completion" axis (choice-set aware: fossil/Hitmon
  forks count as resolved-by-either).
- **True map progress:** a map's own % = its blueprint stage; but Route 22, Victory Road,
  Seafoam, Saffron, Indigo, the Museum and the Game Corner can only be read **jointly with the
  spine position** — the same local bytes mean different things at different global stages.
- **Map-states blueprints need a re-arm annotation (recorded, NOT built — needs its own brief):**
  Victory Road 1F/2F/3F + Seafoam floors' switch/boulder states should be marked
  "re-armed on approach; not durable"; matching should not report them as progression stages.
  Filed as an open item in [`plans/map-states.md`](../plans/map-states.md) (MS-7 candidate).
- **Randomizer legality:** a randomized world must respect the cross-map invariant groups —
  Route 22's arm-state ↔ (lab done, Brock beaten, Viridian Gym beaten); Saffron street state ↔
  `EVENT_BEAT_SILPH_CO_GIOVANNI`; Fuji-home ↔ Tower 7F; ship-present ↔ `EVENT_SS_ANNE_LEFT`
  clear; E4 range ∅ unless mid-run; exactly-one fossil; the always-zero list of §4.
- **The mining lesson (§1c):** cross-map toggles hide in `db TOGGLE_*` tables; cross-map resets
  hide in per-entry map-load hooks. Any future extractor that only reads a map's *own* script
  table is blind to both — run `analyze_cross_map_writes.py` alongside.

## 6. Verification status

Everything above is read from the disassembly with file:line evidence (regenerate via the
script). **Console probes recommended before any UI ships on this:** (1) the Route 23 re-arm on
a Continue that starts there (§1a); (2) the Saffron gym-door rocket actually blocking entry
pre-liberation (§2 — inferred from the toggle lists, not yet walked); (3) one spine save per
milestone through the forge (`emu_make_map_save`) to pin §4's table — the map-states MS-6 probe
pass is the natural vehicle.
