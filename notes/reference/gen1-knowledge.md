# Gen 1 (Red/Blue) Save-Format & Game Knowledge

Domain knowledge about *the game and its save file* — distinct from the codebase architecture
(that's `systems/savefile.md`). This is the reverse-engineering and gameplay lore the editor
encodes, mined from the code comments and the in-app tooltips. Read this when working on the
save layer, the databases, or the randomizer, so the editor keeps matching how the real games
actually behave.

Everything here is asserted by the code (and verified against it); inline source comments are
the primary record, and `version.md` shows where several facts were figured out (e.g. the
tile-block map-view VRAM pointer in `14a73be`).

---

## Save file structure (the bytes)

- **Size:** a Gen 1 save is `0x8000` bytes (32 KB) — `SAV_DATA_SIZE`.
- **Species count:** 151 (`maxPokedex`). Pokédex owned/seen are 0-based flag arrays.
- **16-bit values:** stored as two bytes, `0x12,0x34 <=> 0x1234`; reverse the pair for the other
  endianness. Pad to the full width when formatting — a 16-bit `1` is `0x0001`, not `0x1`
  (`0xCAD` → `0x0CAD`). (Item *prices* are 16-bit; an early bug stored them in 8 bits and
  corrupted any price above 255 — `f19ee18`.)
- **Checksum:** 8-bit. The editor uses the efficient form — **start at `0xFF` (255) and subtract
  each byte of the range in order, letting it underflow/wrap around** — which produces exactly the
  value Gen 1 expects (the game itself uses a slower but equivalent algorithm). Recompute on save.
- **Box checksums (bank 2):** each of boxes 1–6 has its own individual checksum, plus one overall
  bank-2 checksum that *excludes* the individual ones. The game doesn't always use or recalculate
  the box checksums, so only recompute them when the boxes are actually touched.
- **Hall of Fame:** records are `0x60` bytes each and start at `0x598`; record *N* lives at
  `0x598 + N*0x60`.
- **Map-view VRAM pointer:** fixed at `0x9800` (`VramBGPtr`, the GB background-tilemap pointer —
  it never changes); the saved map-view VRAM pointer is just set to it.
- **Badges:** stored big-endian, one owned-flag per badge.
- **Font/text values start at 1:** to map a requested character value, offset by 1.
- **Sprite screen position:** pixel-aligned to a 4-pixel offset from the tile grid.
- **DVs / stats are unsigned and can *intentionally* underflow** — editing below certain
  thresholds wraps around by design.
- **A "+3 offset flag"** exists in the data whose purpose is still unknown (documented as such in
  the source — leave it alone unless you learn what it does).

---

## Gameplay rules the editor encodes

These come from the in-app tooltips (the user-facing explanations) and the model logic:

- **Glitch content is available on purpose.** Glitch species (including MissingNo), glitch moves,
  and glitch maps are all selectable in the editor — the data models carry an explicit `glitch`
  flag and the pickers list a "--- Glitch ... ---" section.
- **Trade status:** a Pokémon is "traded" (i.e. *not yours*) when its **OT name + OT ID don't
  match the player's** name + ID. The OT ID is a 4-hex-digit value (each char `0–9`/`A–F`).
  Because of this, editing the player name/ID **cascades** to keep every non-traded mon yours —
  which is exactly why those writes must be atomic/commit-on-finish (see
  `reference/player-name-hang.md` and `reference/ui-patterns.md` → "Commit edits on FINISH").
- **Nickname semantics:** a name counts as a *nickname* only if it isn't the species name in ALL
  CAPS. Nicknames are difficult to change in-game and are **unaffected by evolution**.
- **Level:** 1–100. **Below level 5 can trigger underflow bugs** (a genuine Gen 1 issue) — the
  editor warns about it rather than forbidding it.
- **Catch rate:** unused leftover garbage from when the species was a wild battle encounter. It
  still exists in the data and is editable, but does nothing in normal play.
- **Natures (retroactive):** natures didn't exist until Gen 3. Game Freak later published a
  formula to derive a Gen 1 mon's nature **from its IVs/DVs**; the editor surfaces this as a
  read-only **"future nature."**
- **Shininess (retroactive):** shininess didn't exist in Gen 1. Gen 2 defined a way to read
  shininess from Gen 1 DV data, and that convention has been kept since; the editor exposes a
  **"future shiny"** toggle. **Toggling it rewrites DV bytes**, which is why the checkbox uses
  `onToggled` (real user action) and not `onCheckedChanged` (see `StatsTab.qml`).
- **Status condition:** the sleep status also carries a remaining-turns count.
- **Currency / stack limits:** money max **999,999**; casino coins max **9,999**; item stack max
  **99**.
- **Player starter:** recorded from the starter you chose, but (per the tooltip) probably never
  actually used in gameplay.
- **Rival starter:** chosen at the start of the game; it **determines the rival's whole team** —
  which Pokémon grows alongside him.
- **Name byte budget:** person names are *meant* to be ≤ 7 characters, but there are 10 bytes
  total. The name editor warns at 7, warns again at 10/10 used, and warns when a name **expands
  wider on screen than its byte count suggests** (some single bytes render as multiple tiles).

---

## Place-specific state ("world local")

A handful of save values are tied to *specific maps* rather than to the player or world globally.
The map browser deliberately surfaces extra category icons for exactly these maps
(`MapDetails.qml`'s special-map list):

- **Daycare** (map 72) — the single deposited Pokémon. (Daycare is treated as a likely
  frequently-used "map" even though it isn't the current one.)
- **Vermilion Gym** (map 92) — the two Lt. Surge trash-can switch positions.
- **Cinnabar Gym** (map 166) — the gym quiz's next opponent.
- **Safari Zone** (entrance map 156 plus the zone's sub-maps 217–225) — the Safari game variables
  (steps/captures, etc.).

---

## Experience & growth rates

Each species has a **growth rate** — the EXP-to-level curve — identified by an internal **Growth
ID** that also appears in the Pokemon data table. Only four are used in Gen 1 (IDs **1 and 2 were
never used**).

Recorded **verbatim** from the original research note (`growth-notes.txt`):

| Name | Growth ID | EXP at level 100 | Formula (n = level) |
|------|-----------|------------------|---------------------|
| Fast        | 4 |   800,000 | Exp = 4 * n^0.6 |
| Medium Fast | 0 | 1,000,000 | Exp = n^3 |
| Medium Slow | 3 | 1,059,860 | Exp = 1.2 * n^3 - 15 * n^2 + 100 * n - 140 |
| Slow        | 5 | 1,250,000 | Exp = 5 * n^0.75 |

> **Open question — verify the Fast/Slow formulas before relying on them (kept as written).**
> This is only a plain-arithmetic observation, not a claim about what's "correct": the Fast and
> Slow *formulas* as written don't reach the level-100 *caps* in the same row. `4 * 100^0.6 ≈ 63`
> and `5 * 100^0.75 ≈ 158`, not 800,000 / 1,250,000. (Medium Fast `n^3` and the Medium Slow
> polynomial both already match their caps and aren't in question.) The commonly-cited Gen 1 cubic
> forms — Fast `(4/5)·n^3`, Slow `(5/4)·n^3` — *do* hit those caps, so the `n^0.6`/`n^0.75` lines
> may have been pasted from a different context, be shorthand for something other than
> total-EXP-at-level-n, or just be off. They're preserved exactly as written; check them against
> wherever they came from (or the program's own EXP code) before treating either form as
> authoritative.

**Editor design for EXP (KISS):** the editor offers a **slider to move EXP between the current and
the next level**; to change whole levels you edit the **level number**, which resets EXP to the
correct range for the new level. (This is what the General-tab Exp slider + the "fine-tune exp
between levels; change the level for whole-level changes" tooltip implement.)

---

## Music: banks, IDs, and the address formula

PokeRed abstracts audio so heavily that the underlying numbers are very hard to recover — this
took **over a year** of trial-and-error (the pokered team didn't answer requests for help),
cross-checked against the compiled disassembly's symbol table. What's known:

- Each track lives in a **bank** and has an **ID**; the address within the bank is
  **`ID * 3 + 0x4000`**. The IDs aren't evenly spaced, so they were recovered by guess-and-check:
  apply the formula, compare against the known bank/address from the symbol table, and adjust.
- **Useful discoveries:** a *slightly* wrong ID points to a slightly wrong address and produces
  **interesting (glitchy) music** — usable as a feature, not just a bug. And a **very short SFX**
  used as map music comes out as effectively **"No Music"** — e.g. `SFX_Snare1_1` (bank `1F`,
  ID `FF`) renders **silent**, which is eerier than Lavender Town.

The recovered track → bank → ID table (IDs are hex):

| Track | Bank | ID | | Track | Bank | ID |
|-------|------|----|-|-------|------|----|
| Music_PalletTown | 2 | BA | | Music_Routes2 | 2 | EF |
| Music_Pokecenter | 2 | BD | | Music_Routes3 | 2 | F3 |
| Music_Gym | 2 | C0 | | Music_Routes4 | 2 | F7 |
| Music_Cities1 | 2 | C3 | | Music_IndigoPlateau | 2 | FB |
| Music_Cities2 | 2 | C7 | | Music_GymLeaderBattle | 8 | EA |
| Music_Celadon | 2 | CA | | Music_TrainerBattle | 8 | ED |
| Music_Cinnabar | 2 | CD | | Music_WildBattle | 8 | F0 |
| Music_Vermilion | 2 | D0 | | Music_FinalBattle | 8 | F3 |
| Music_Lavender | 2 | D4 | | Music_DefeatedTrainer | 8 | F6 |
| Music_SSAnne | 2 | D8 | | Music_DefeatedWildMon | 8 | F9 |
| Music_MeetProfOak | 2 | DB | | Music_DefeatedGymLeader | 8 | FC |
| Music_MeetRival | 2 | DE | | Music_TitleScreen | 1F | C3 |
| Music_MuseumGuy | 2 | E1 | | Music_Credits | 1F | C7 |
| Music_SafariZone | 2 | E5 | | Music_HallOfFame | 1F | CA |
| Music_PkmnHealed | 2 | E8 | | Music_OaksLab | 1F | CD |
| Music_Routes1 | 2 | EB | | Music_JigglypuffSong | 1F | D0 |
| Music_BikeRiding | 1F | D2 | | Music_Dungeon1 | 1F | E0 |
| Music_Surfing | 1F | D6 | | Music_Dungeon2 | 1F | E4 |
| Music_GameCorner | 1F | D9 | | Music_Dungeon3 | 1F | E8 |
| Music_IntroBattle | 1F | DC | | Music_CinnabarMansion | 1F | EC |
| Music_PokemonTower | 1F | F0 | | Music_MeetEvilTrainer | 1F | F6 |
| Music_SilphCo | 1F | F3 | | Music_MeetFemaleTrainer | 1F | F9 |
| | | | | Music_MeetMaleTrainer | 1F | FC |

(This is also encoded in the music database / `music.json`; the formula and the discovery notes
above are the part that only lived in the original research note.)

---

## Randomization rules (playable by design)

The flagship feature's *concrete* constraints live in code; the *spirit* is in
`context/principles.md` → "The Randomization Feature". Two rules are worth recording because they
encode real Gen 1 knowledge:

- **Badges are zeroed on a random new game** — random mid-game progress isn't fun for a fresh
  start — **except Thunder, Cascade, Soul, and Rainbow, which are always left ON.** Those four
  aren't about progress here: they grant **out-of-battle HM use** (the HMs gated behind badges:
  Fly/Surf/Strength/Cut/Flash), so the player can actually move around the world. The gyms can
  still be battled for the first time — the badge just confers the HM permission
  (`playerbasics.cpp`).
- **Maps keep valid warps, wild encounters stay balanced, glitch species are avoided, and an
  HM-capable Pokémon is included** so the randomized save is genuinely *playable*. The map model
  even separates normal maps from a "--- Glitch Maps ---" section and labels the glitchy
  in-between ones. (`MapDBEntryWarpOut`, `MapSearch`, and the randomizer notes in
  `context/principles.md`.)
- **Randomize the towns the player has visited, but always include Pallet Town** (the starting
  town must be flagged visited or the new game makes no sense).
- **Badges and event-completed flags are separate.** Because the randomizer hands out a few badges
  for HM use (above) *without* setting the matching gym's event-completed flag, the player still
  has to actually battle and beat that gym the first time. The app should **tell the player this**
  rather than let the mismatch look like a bug.

---

See `systems/savefile.md` for the code that reads and writes these bytes (the toolset,
iterator, and the expanded object model), `systems/db.md` for where the species/move/item/map
data and their `glitch` flags come from, and `context/principles.md` for the randomizer's intent.
