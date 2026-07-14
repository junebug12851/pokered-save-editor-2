# Project Status

_Current state only._ For the chronological history of what changed each session and why, see
[`sessions/`](sessions/README.md) (one file per day). For root-cause mechanics see
[`reference/qt-patterns.md`](reference/qt-patterns.md) and [`decisions/`](decisions/architecture.md). For the
commit-by-commit changelog see [`version.md`](version.md).

**Version:** `0.20.0-alpha` — on `dev`, **awaiting Twilight's in-app review, then "ship"**. (Previous
release: `0.16.6-alpha`, shipped 2026-07-11.) Single source of truth: repo-root `VERSION`; see
[`reference/versioning.md`](reference/versioning.md). Full `ctest` green (76/76).

> **Releases are MANUAL.** Commit and push to `dev` freely, but `main` only moves when Twilight says
> **"ship"**. Green is necessary, not sufficient. See [`reference/git-workflow.md`](reference/git-workflow.md).

## Current state (read this first)

### 🚪 WARPS — the research is done; the model has seven bugs (2026-07-14)

The next phase of the map screen is **warps**, and — as with sprites — the research pass found that our
model is a straight port of v1's guesses. Nothing is built; **Phase 5a exists to fix this first.**
Everything: [`reference/warps.md`](reference/warps.md). Design:
[`plans/map-screen.md`](plans/map-screen.md) → **Phase 5** (5a–5e).

**The linchpin (and it is why a warp editor is possible at all).** `LoadMapHeader` rebuilds the warp
list from ROM on every map load, with **no escape-hatch bit** — but `LoadMainData` **sets
`BIT_NO_PREVIOUS_MAP` on the saved tileset byte** as it reads the save, which makes the next
`LoadMapHeader` **`ret nz` before it copies anything**. So **an edited warp is genuinely live on
Continue** — console-verified, including a **4th warp invented in a 3-warp town** — and the game
restores the map's original doors when the player leaves and walks back in. The sprite rule, reached
by a different route, and the screen must say it out loud.

⚠️ **`wStatusFlags3` is ZEROED on every save load — the whole byte.** It shares an address with
`wCableClubDestinationMap`, and `SpecialEnterMap` (on the Continue path) writes `0` to it. Verified:
wrote `$FF`, console read back `$00`. So `scriptedWarp` + `isDungeonWarp` — **and `npcsFaceAway`,
`tradeCenterSpritesFaced`, `isBattle` (really `BIT_TALKED_TO_TRAINER`) and `isTrainerBattle` (really
`BIT_PRINT_END_BATTLE_TEXT`)** — can never survive a save. Every other warp byte came back exactly as
written.

💀 **`wWarpedFromWhichWarp` / `wWarpedFromWhichMap` are DEAD.** The game writes them on every warp and
**nothing anywhere reads them**. Two writes, zero reads. A wiped byte and an unread byte are different
facts and the panel must say which is which.

⭐ **The two bytes that matter weren't on the warps screen at all:** `wLastMap` (`0x2611`) — *the map a
`$FF` door returns you to*, i.e. every building's exit — and `wLastBlackoutMap` (`0x29C5`) — where
blacking out / **Dig** / **Escape Rope** put you. Both **already modelled** in `WorldGeneral`; they just
need surfacing. `wLastMap` goes in the **toolbar**, in words (*"Outside is: Pallet Town"*), because
changing it re-labels every `$FF` door on the map at once.

🔫 **Two loaded guns, and `AreaWarps::setTo()`/`randomize()` load them today.** `wDestinationMap` (Fly)
has **13** legal values in an unterminated, unbounded table; the dungeon (map, hole) pair has **12**, and
hole numbers are **1-based**. `setTo()` currently picks *any* cave, *any* map, and a 0-based hole index —
out-of-table values that make a real console read arbitrary ROM as warp data. **Dormant only until the
`MapsDB` deep-link landmine is defused** (see Open issues), exactly like `AreaAudio::setTo()` was.

Tool: `scripts/emu/probe_warp_persistence.py` (local-only, ROM-gated).

### 🚶 The NPCs walk the way the GAME walks (2026-07-13, `0.30.0-alpha`)

`MapSim` is now **`UpdateNPCSprite`, transliterated** — the console's per-frame state machine out of
`engine/overworld/movement.asm`, instruction for instruction, **bugs included**, ticking at the DMG's
own **59.7275 Hz** (every counter in it is measured in Game Boy frames). Full write-up with the assembly
beside it: [`reference/npc-movement.md`](reference/npc-movement.md).

Three things it taught us, and none of them were guessable:

1. ⚠️ **A `STAY` sprite is not standing still — it is TURNING.** It runs the whole random-direction path,
   and `TryWalking` writes the new facing *before* `CanWalkOntoTile` refuses the step. Oak picks a
   direction, turns to face it, fails to move, and sets a new delay — about once a second, forever. Our
   first sim just *skipped* `STAY` sprites, which is a still picture, not a simulation.
2. ⚠️ **The game only animates the people NEAR THE PLAYER.** The screen-bounds check is measured
   *relative to him*, so Pallet's Fisherman — eight tiles below Red — takes no step until Red walks
   towards him. The test found this by failing on him.
3. ⚠️ **The wander limit is BUGGED and we copy the bug.** Walk up five times and `yDisplacement` hits 3;
   now `3 + 1 = 4 < 5` and the sprite **can never walk down again**. The disassembly flags it itself.

**And every "animation scratch" byte is now live state** — `movementStatus`, `movementDelay`,
`walkAnimationCounter`, the two frame counters, the step vectors, `yDisp`/`xDisp`. They are not scratch.
They are the simulation.

### 📐 STANDING RULE: use the GAME's file formats (2026-07-13)

Where `pret/pokered` has a format, **we use that format** — by default, without asking. `.blk` already is
theirs. **Music becomes their own `.asm` sheet music, parsed line by line** (⏳ owed): it is line-based
assembly with macro names, so a *line parser* turns it straight into sound-engine commands — we never
needed a byte compiler. See [`reference/file-formats.md`](reference/file-formats.md) and `CLAUDE.md`.

### ⏳ Owed from Twilight's 2026-07-13 review (captured, not yet done)

- **The sprite Details panel is bad and gets rebuilt.** Raw values, cryptic, cramped, "Who/Where/When" is
  dumb. Group X+Y into one control; picture = a *picker* with the artwork; hide fields the combo makes
  irrelevant; movement status is not a number; "delay until next move" must be *shown*, not explained in
  prose. **Text id / item / trainer class / trainer team must resolve to REAL map data** (extract it if we
  haven't got it). `✕` becomes a **Delete** button. A **?** icon in the panel title (the one allowed
  tooltip-icon), and a **yellow !** on anything the game rebuilds on load.
- **Music**: a real grouped **ComboBox** like the map picker (not a hand-rolled list), **volume slider
  below it**, and **hover must not change the track** — only selection does.
- **Canvas/panels**: player drag is glitchy; drag-and-drop *still* not working; drop the "cast no longer
  matches" notice; the map must not resize when panels open; Layers "Clear" belongs in the panel title
  (+ one per category) and must look like a clear button; delete "none here"; **"Meaning" → "Components"**;
  the Sprite Set and Strength panels' walls of text go; the scrollbar-overlap problem (it is in the notes).

### 🧍 SPRITES — phase 4 is COMPLETE (2026-07-13, `0.27.0-alpha`)

| | | |
|---|---|---|
| **4a** | The data model, made true | ✅ the four bugs fixed + **negative-controlled**, the five unread fields modelled, `sprites.json` grouped |
| **4b** | NPCs on the canvas | ✅ all 72 sprites **imported and drawn**; *People & objects* layer; **select, drag-to-move, ✕ delete, ✎ edit**; the ground is **no longer selectable** |
| **4c** | The Characters bar | ✅ 72 characters on 5 shelves + filter; **drag in to place, drag out to delete**; collapses to a strip |
| **4d** | The Details panel | ✅ edits what's selected; **the map's own details when nothing is**; every sprite byte named, explained, full-range, hack values flagged not refused |

**The artwork blocker is gone.** The repo carried exactly *one* sprite sheet (`red.png`); every town was
empty. `scripts/import_sprites.py` pulls **all 72** out of `pret/pokered` into one atlas
(`assets/sprites/overworld.png`) + a generated `spriteart.h`. It is self-validating, and it **found a
third art shape by itself**: `16x48` — the **twenty "still" people** (nurses, guards, Mom, the Gameboy
Kid) who have **no walking art at all**, because the game never drew them a step.

⚠️ **A sprite whose picture this map has NOT loaded is outlined in amber.** That is the one the console
draws as garbage. Shown, never silently corrected.

⚠️ **The "you changed the cast" warning tracks the EDIT, not a diff against the ROM** — and the first
version got that wrong. **A real save's cast already differs from the cartridge's**, because walking
NPCs wander: Pallet's Girl is at (3, 8) in the ROM and (3, 6) in `BaseSAV`, having taken a couple of
steps. Diffing would have cried wolf on every save ever opened. Pinned by
`tst_map_sprites::npcsEdited_isQuietUntilYouActuallyChangeSomething`.

New test: **`tst_map_sprites`** (10 cases). Its keystone is `moveNpc_writesExactlyTwoBytes` — drag a
person across town and byte-diff the whole 32 KB save: **exactly `mapX` and `mapY` moved, nothing else.**

⏳ **Owed: Twilight's live pass** on the drag, the drop, the delete and the panel — a still PNG can
review none of those.

### 🧍 SPRITES — the research: the model was WRONG in four places (2026-07-13)

The next phase of the map screen is **sprites**, and the research pass found that our sprite model — a
straight port of v1's — has been **writing the opposite of what it says** into saves. Nothing is built yet;
**phase 4a exists to fix this before anything is built on it.**

| | v2 has | the truth |
|---|---|---|
| `SpriteMobility` | `Moving = 0xFF`, `NotMoving = 0xFE` | **inverted** — `STAY = $FF`, `WALK = $FE` (console: Oak reads `$FF`) |
| `load(MapDBEntrySprite*)` | `"Stay"` → `0xFE` | writes **WALK for a STAY sprite** — every `setTo()`/randomize |
| `SpriteGrass` | `InGrass = 0x00` | **inverted** — `$80` = in grass. `reset()` flags every blank sprite as in grass |
| `face` / `range` | two fields | **one byte** (movement byte 2). `face` is being written into `faceDir` — a different field in a different table |

Not modelled at all: StateData1 `a`/`b`/`c`, StateData2 `9`/`d`, and **`wToggleableObjectFlags`**
(`0x28A0`, 32 bytes) — the flags that actually decide whether a missable NPC appears. (We model only the
per-map *list* at `0x287A`, which the game rebuilds from ROM every map load — it does nothing.)

⚠️ **And the cartridge overruled a careful reading of the source.** `LoadMapHeader` appears to zero and
rebuild all sprite state from ROM on every map load — so I concluded sprite edits were being thrown away.
**They are not.** `scripts/emu/probe_sprite_persistence.py` boots the real ROM: a re-pictured, moved sprite
**and a fourth NPC invented in a three-NPC town** all survive Continue, because `MainMenu` sets
`BIT_CUR_MAP_LOADED_1` and the map header is never re-read on that path. The true statement — **which the
screen must say out loud** — is that an edited sprite is really there, and **the game restores the map's
original cast when the player leaves and re-enters**.

Everything: [`reference/sprites.md`](reference/sprites.md) → Parts 3, 5, 6.
The design: [`plans/map-screen.md`](plans/map-screen.md) → **Phase 4**, rewritten into **4a** (make the
model true) · **4b** (NPCs drawn, selectable, draggable; background squares **stop** being selectable) ·
**4c** (the Characters bar — drag in to add, drag out to delete) · **4d** (the **Details panel**, left side,
showing the map's own details when nothing is selected).

### 🗺️ The Map screen rebuild — phases 0–3 are IN (2026-07-12)

| Phase | | Status |
|---|---|---|
| **0** | Unblock the bridge | ✅ shipped — nine opaque Area children opened up; the Q_INVOKABLE returns (`WarpData*` …) registered at last; `MapsDB` deep-linked at boot |
| **1** | The chassis | ✅ shipped — identity bar · tool rail · context bar · dark canvas well · **collapsing icon dock (one panel, never stacked)** · status bar. The eviction queue and the chip bar are deleted |
| **2** | The layer system | ✅ shipped — **Guides / Meaning / Game View**, tri-state group eyes, alt-click solo, folding; **the player and the red + grey boxes are layers now**; `tst_map_layers` byte-diffs the save across every toggle |
| **3** | The map is ALIVE | ✅ shipped — the water's **rotation** and the flower's `1,1,2,3` at the console's **20/21-frame cadence**, a ▶/⏸/step transport, and **frame 0 when headless** so no test flaps. Found + fixed two long-standing inventions (see below) |
| **4–13** | objects · inspector · encounters · area state · tileset/blocks · tools · polish · verification · notes · *(optional)* walk-the-map | ⏳ next |

⚠️ **The water and the flowers were WRONG for years.** `TilesetEngine` ran the water `0,1,2,3,4,3,2,1`
(the console swings **−1..+3**, not 0..+4) and the flower `2,3,1,1` (the console runs **`1,1,2,3`** —
flower1 shows for twice as long). Both plausible, both invented, both now read out of
`UpdateMovingBgTiles` and pinned by `tst_map_animation`.
⏳ **Owed:** `tst_emu_parity` does not yet dump VRAM tiles `$14`/`$03` frame-by-frame — the animation
is verified against the *disassembly*, not yet against the *silicon*. See
[`reference/map-animation.md`](reference/map-animation.md).

### 🗺️ The plan of record — read it before touching the map screen (2026-07-12)

The map grew organs fast (blocks, tiles, meaning, palettes, the player, connections, music) and every one
was bolted onto the same screen. Twilight's verdict — *"UX is one of the highest priorities and right now
it's really bad"* — and she is right: three bars of unrelated chrome, panels that stack sideways and
**evict each other**, no real layer system, nothing editable on the canvas, and **most of the Area block
has no UI at all**.

The complete overhaul is **designed and approved**: [`plans/map-screen.md`](plans/map-screen.md). The
chassis (identity bar · tool rail · context bar · dark canvas well · **collapsing icon dock, one panel at a
time, never stacked** · status bar), a **4-group layer tree** (Guides / Meaning / Game View / Objects — the
red screen box, the accent draw area and the player are **layers**), **on-canvas object editing** (drag,
select, add, delete warps/signs/NPCs), **every byte of the Area block editable** (hack values included,
shown never rewritten), and **frame-accurate animation** ([`reference/map-animation.md`](reference/map-animation.md)).
**Thirteen phases, plus one optional** — each a full pass, finished before the next begins.

⚠️ **Two blockers sit in front of ALL of it** (phase 0, both already diagnosed):
1. `area.h` `Q_DECLARE_OPAQUE_POINTER`s **nine of its eleven children** → QML reads `area.map.*`,
   `area.warps.*`, `area.player.*` as **`undefined`**. Same bug that bit `AreaAudio`. Fix: full `#include`.
2. `DB::deepLinkAll()` never calls `MapsDB::inst()->deepLink()` (the latent landmine below).

⚠️ **And the map does not animate.** `UpdateMovingBgTiles` rotates water tile `$14`'s bytes every 20 frames
and cycles three flower tiles every 21 — we draw **frame 0, forever**. The water is dead; the console's
water is not. That is a correctness bug, not a missing garnish.

**New territory: the MAP (2026-07-12).** The app has fully resurfaced from the revival, and the first
new ground is the biggest one. The old Maps screen (a greyed-out tile → a menu of dead ends) is
**deleted**, and in its place is a **map emulator**, step 1 of several.

It rebuilds what the Game Boy rebuilds: the map ringed by its **3-block border**, the **6×5-block
scratch area** the game redraws, and the **20×18-tile screen** sliding around inside that in half-block
steps — all drawn from `.blk`/`.bst` data imported **verbatim** from `pret/pokered`, at one screen pixel
per Game Boy pixel. It is an emulation, not an impression, and it proves it: the view pointer the game
itself computed and left in the save (`0x260B`) is **reproduced byte-for-byte** from just the player's
coords and the map width, on both real saves (`tst_map::viewPointer_matchesWhatTheGameStored`). If that
ever fails, our model of the game is wrong — read it first.

Pieces: `BlocksDB` (db) → `MapEngine` + `MapProvider` (app) → `MapModel` = `brg.map` → `Map.qml`.
Domain write-up: [`reference/gen1-knowledge.md`](reference/gen1-knowledge.md) → "VERIFIED from the
disassembly". Import: `scripts/import_map_blocks.ps1` (self-validating, `-Check`).

**Every one of the 248 map ids renders** — including the glitch and half-baked ones, which are not empty
maps but *unfinished copies*: `maps.json`'s own `incomplete` field says which map of, in exact agreement
with the ROM, so we follow it and draw the map they copy (what a Game Boy actually does with those ids).
Nothing invented, no JSON changed.

**Not yet drawn:** the **NPCs** (the other 15 sprite slots), the grass-priority bit, warps/signs overlays, and
tile animation frames (frame 0 only). **Connection strips, the palettes/"contrast" and the PLAYER are DONE**
(below).

### ✅ 🗺️ The map now says what it MEANS — and it caught a save-corrupting bug (2026-07-12, 0.24.0-alpha)

The map could draw Pallet Town; it could not tell you a thing about it. A wall and a floor are just two
pictures. Now the **meaning layer** is in: **Show** chips over the map light up **Walls, Grass, Water,
Warps, Doors, Ledges (with the arrow you jump), Counters, Elevation edges, Cut trees** and the **border
ring** — each a tint *and* its own 8×8 pattern, so several stack and still read apart, and so they
survive the glitch palettes. **Off by default** (the map is the point), and a chip with nothing to show
says so rather than switching on an empty overlay. Rendered as **one image in C++** — Route 17 is 20,000+
tiles and a per-tile QML delegate would crawl.

**Click a block** → the **Blocks panel** draws it big with all 16 tiles, each labelled with what it *does*
("Grass — wild Pokémon", "Ledge — jump down"). Hover a tile there, it lights up on the map.

Domain write-up: [`reference/tiles.md`](reference/tiles.md). Import: `scripts/import_tile_traits.py`
(self-validating; re-reads every collision list **out of the cartridge** and demands a byte-for-byte
match). New DB: `TileTraitsDB`. Pinned by `tst_tile_traits` (10 cases).

#### 🔑 There is no Indoor/Cave/Outdoor byte — there's an ANIMATION byte, and it was a real bug

The tileset header's last byte is `TILEANIM_NONE / WATER / WATER_FLOWER`, saved at `0x3522`
(`sTileAnimations`, the byte before the checksum) — what `AreaTileset` calls `type`. **`tileset.json`'s
Indoor/Cave/Outdoor is a verified 1:1 rename of exactly that** (all 24 checked against the cartridge's
header table).

`Settings::previewOutdoor` was a **bool**, so it collapsed **Cave into Indoor** — every cave in the game
rendered with **dead, motionless water** when the console animates it. Now tri-state everywhere
(`previewTilesetType`), and each option says what it *does*.

#### 🐞 THREE tilesets pointed at the WRONG collision list — and it was writing that into saves

`tileset.json`'s `collPtr` was wrong for **Mart (→5971)**, **Forest (→5989)** and **Reds House 2
(→5961)**. The collision lists are **shared** between tilesets in the ROM (Red's House 1 *and* 2 are one
list; Mart *and* Poké Center are one list), and the v1 importer assumed one list each in index order — so
the chain slipped by one.

**This had teeth:** `AreaTileset::loadFromData()` writes `collPtr` into the save, so "put the player in a
Poké Mart" wrote **Red's-house collision** into it. Fixed with Twilight's go-ahead (3 lines), verified
against the cartridge, and pinned by `tst_tile_traits::derivedCollPtrs_matchTilesetJson` +
`martAndRedsHouse_areNotTheSameList` — which were **negative-controlled**: put the bug back and they fail
by name with the exact reason.

⏳ **Owed: Twilight's live pass** on the chips, the click-to-inspect, and the two new panels.

### ✅ And now the actual Game Boy checks our work (2026-07-12)

`tst_emu_parity` boots the **real ROM** in an emulator with one of our saves, reads the **console's own
RAM**, and demands `MapEngine` match it byte for byte. The verdict: the view pointer, the map's blocks, the
24×20 scratch area and the **20×18 screen tiles all MATCH** — `wTileMap` matching means the entire view
pipeline is right, with no sprites or palettes in the way.

It also immediately caught the one thing that *was* wrong — **the border ring** — which is now **fixed and
verified**: see below.

### ✅ 🎵 THE EDITOR PLAYS THE GAME'S MUSIC (2026-07-12, 0.22.0-alpha)

There is not an audio file in the repo. **`pse-audio` is a Game Boy sound chip** (`GbApu` — two squares, a
wave table, a noise LFSR, the 512 Hz frame sequencer) with **Pokémon Red's own sequencer running on top of
it** (`Gen1SoundEngine` — `engine_1.asm`, transliterated over a 256-byte state laid out exactly like the
console's `$C000`). The music data is **imported from `pret/pokered` and verified byte-for-byte against the
cartridge** (38 KB for the whole soundtrack).

On the **Map screen**, behind a **♪** toggle: the map's music (its own is selected on open), the two save
flags — **No Audio Fadeout** and **Prevent Music Change** — a **▶**, and **hover-preview**: with the music
playing, run the mouse down the list and it changes as you go. **Hover auditions; click commits** — moving a
mouse never touches the save, and a line says plainly when what you hear isn't what's stored.

⚠️ The **bank** picker offers only 2 / 8 / 31, because we measured what a real console does with anything
else: it **stops producing frames**. A save holding a bad bank is **shown, never silently rewritten**.

**And the console says it's right.** `tst_sound_parity` boots the real cartridge, photographs the engine's
entire mind (`$C000–$C0FF`) **every frame**, seeds our port from the console's own state, and demands it
reproduce every frame after — **all 46 tracks plus an inner voice, byte-for-byte: 48/48**. It found three
real bugs in the port (the pitch-slide routine **clobbers `de`**, so a slide note starts on the divide's
leftovers; `PlaySound` never restores `wSoundID` after a drum; `wSfxHeaderPointer` was never written) and
one in the test rig itself (photographing the console **mid-fade**, while `wAudioROMBank` was still the
*old* bank, produced dumps that looked plausible and were nonsense). A negative control was run: break the
note length by one and it fails on frame 7 with the exact byte.

Green: `tst_sound_parity` 48/48, `tst_audio` 10/10, `tst_qml_screens` 16/16, full `ctest` **78/78**. The
whole story: [`plans/music.md`](plans/music.md).

⏳ **Owed: Twilight's live pass.** It is sound and it is hover — a still PNG can review neither.

### 🎵 Music — the research (2026-07-12)

The next organ of the map emulator is **sound**, and it all lands on the **Map screen**: the two save flags
(**No Audio Fadeout**, **Prevent Music Change**), the map's **music track** picker, and — the real ask —
**actually playing the music**, accurately, with a ▶ per track and **hover-to-preview**.

Decided with Twilight: the music data is **imported from `pret/pokered`** (the same precedent as the map
blocks — it ships, no ROM needed), and playback is a **full port** — the real Gen 1 sequencer driving a real
**DMG APU** model in C++ (`pse-audio`), not an impression. Nothing is built yet.

The research is done and written down:
[`reference/gameboy-apu.md`](reference/gameboy-apu.md) (the chip),
[`reference/gen1-sound-engine.md`](reference/gen1-sound-engine.md) (the game's sequencer + the two flags,
**verified against the disassembly**), and the six-phase build in [`plans/music.md`](plans/music.md).

The keystone, when it's built: the engine's **entire state is 243 bytes at `$C000`**, so PyBoy can dump it
from the real cartridge every frame and our port must match **byte-for-byte, frame by frame**. Same doctrine
as the map — the console is the judge.

### 🎼 …and there is no such thing as glitch music. There are 105 extra songs. (2026-07-12)

A music header is **3 bytes per channel**, and ids are computed *by address* (`SFX_Headers_N + id × 3`) — so
a 3-channel song **eats three ids**, and the spare two parse as perfectly valid **one-channel headers
pointing at that song's channel 2 and channel 3**. **Id 186 is Pallet Town; id 187 is Pallet Town's
bassline, alone.**

Parsed out of the cartridge: across the three banks the music region holds **46 real tracks, 105 inner
voices, and zero garbage ids**. The console confirms it — with id 187 the save loads, `wChannelSoundIDs` is
`[0, 187, 0, …]` and NR51 is `$22`: **one channel, playing a melody.** Every inner voice points *into* a
stream we already intend to import, so the shipped app gets **151 pieces of audio for the price of 46** —
and, because we run the *engine*, every note is known exactly, so sheet-music export (MIDI/MusicXML) is
nearly free.

⚠️ **The BANK byte is a loaded gun, though.** `PlaySound` maps whatever bank the save names and *then*
picks an engine — so a bank that isn't 2/8/31 executes **arbitrary cartridge bytes as code, every frame**.
**Verified: the console stops producing frames the instant the map loads.** The editor will offer only
2/8/31, and will *show* (never silently rewrite) a save that holds anything else.

Everything, with the cartridge's own testimony: [`reference/glitch-music.md`](reference/glitch-music.md).
Tools: `scripts/emu/analyze_music_ids.py`, `scripts/emu/probe_glitch_music.py` (both local-only, ROM-gated).

### ✅ Connection strips — done, and the hardest part of the map engine (2026-07-12)

The border ring is not a wall of trees: the game bleeds the **connected maps' edges** into it, so Pallet
Town's ring is really Route 1's bottom rows and Route 21's top rows. Now reproduced —
**78 of 78 connections in the game, byte-for-byte against the compiled structs in the real cartridge, zero
mismatches**, and the resulting ring is byte-identical to the console's `wOverworldMap`.

⚠️ **`MapDBEntryConnect::stripSize()` is WRONG** (it branches on `fromWidth < toWidth`; the macro clamps on
`min(curW + 3 - offset, toW)`), and `maps.json`'s **`flag`** field exists only to patch that. The real game
has no flag. `MapEngine` ignores both and recomputes from the macro, so nothing is broken today — but the DB
still carries a wrong formula. **Fixing it is Twilight's call** (curated data + a public DB API).

Everything about it: [`reference/map-connections.md`](reference/map-connections.md).

### ✅ Palettes / "contrast" — the six glitch palettes render (2026-07-12)

The save's `contrast` byte (`0x2609` = `wMapPalOffset`) is **not a brightness dial**: the game **subtracts it
from a pointer** into its fade-palette table (`LoadGBPal`, `home/fade.asm`). `0/3/6/9` land on real entries —
**the four contrast levels** (0 normal, 6 the "needs FLASH" cave palette, 9 black). Everything else reads
**across the seam between two entries** — `1, 2, 4, 5, 7, 8`: **the six glitch palettes.** Exactly what
Twilight said, and now we know why.

The map is drawn **through** whichever `rBGP` that produces, so a glitch palette renders as the genuine
article, not an imitation. **All ten verified against the real console's palette registers — zero
mismatches.**

⚠️ Note contrast **1 and 2 look normal on the map** — their damage is in `rOBP0`/`rOBP1`, the **sprite**
palettes. They will show the moment the player is drawn. That is the console's behaviour, not a gap.

Everything about it: [`reference/palettes.md`](reference/palettes.md).

### ✅ The player is drawn — and the "harmless" glitch palettes stop being harmless (2026-07-12)

He is where the **console's own OAM** says he is: screen (64, 60) — his tile column, and **4 pixels above**
his tile row ("*which makes sprites appear to be in the centre of a tile*"). Facing right is drawn as facing
**left, mirrored** — there is no right-facing art in the game. Colour 0 of an object is **always
transparent** (his cut-out).

**And this is what finally makes contrast 1 and 2 bite.** They leave `rBGP` alone — the map looks perfectly
normal — and wreck `rOBP0`/`rOBP1`, the **object** palettes. With the player on screen, contrast 1 now leaves
the world untouched and ruins *him*, exactly as the cartridge does.

Everything about it (incl. the OAM hardware and Gen 1's 16-slot system):
[`reference/sprites.md`](reference/sprites.md).

The ROM is Twilight's own cartridge backup: **git-ignored, never committed, never shipped**; without it
every case SKIPs. Setup + the traps (the "has the save loaded?" trap, `wCurMapTileset` bit 7):
[`reference/emulator-verification.md`](reference/emulator-verification.md).

The big structural blocker is **solved**: the `brg.file.data.dataExpanded.*` chain works, data reads
and **persists** across every screen, and the build is fast. The other major bug class — **QML
garbage-collecting parentless C++ QObjects** (the font/name blanking and the clicking-Pokémon crash)
— is also fixed (DB entries via `DB::qmlProtect`; savefile `Q_INVOKABLE` returns via `qmlCppOwned`;
storage boxes/mons/moves self-protect from their ctors). Data flows, names render, no crash clicking
around. Recovery from the 2026-06-06 corruption is complete and confirmed runtime parity.

We are in a **UI-polish phase**. Two big screens are polished + signed off: the **Pokémon
details editor** (General / DV-EV / Moves tabs + Glance pane) and the **name editors** (full keyboard
+ quick-edit popup). The Trainer Card, Bag, Pokémon storage, Rival, and Credits/About screens have all
had cleanup/redesign passes. The recurring underlying theme is the **Qt 6 Material control-height
issue** (Qt 6.5+ taller `TextField`/`ComboBox`); the fix everywhere is proper layouts, not pixel
offsets ([`reference/qt-patterns.md`](reference/qt-patterns.md)). **Read
[`reference/ui-patterns.md`](reference/ui-patterns.md) before any UI work.**

The **Market** now does real item trading. Its Exchange tab has three sub-tabs — **Currency**
(money↔coins), **Healing**, and **Custom** — where the last two swap one item for another, priced by
each item's **buy price**, across the bag + PC storage combined, previewed live and written only on
Checkout. The give side lists what you own, the get side lists **every** item with the unaffordable
ones greyed out (which is what guarantees the two "+" buttons are never both dead). Backed by
`ItemExchangeModel` and pinned by `tst_item_exchange` (14 cases).

⚠️ **The one hard-won rule there:** an exchange is priced as **one whole trade**, not per step — the
*total* value is rounded up to a whole number of the given item and only that single leftover is
refunded. 3 Fresh Water (₽600) costs exactly 2 Potions (₽600) and refunds **nothing**. Pricing each
step separately (the original bug, caught by Twilight and fixed on 2026-07-11) invents money out of
thin air. `giveFor()` / `refundFor()` are the single pricing path shared by the preview, the "+"
gating, and `checkout()` — keep it that way.

The **full keyboard** was rebuilt on 2026-07-11 into an actual **ASDF keyboard deck** — the headline of
**0.16.x**. **47 assignable keys** (26 letters + 10 digits + the 11 punctuation keys) × **8 pages**
(255 tiles need 8 pages; Shift/Ctrl/Alt give exactly 8 chords), each cap carrying one game tile with
the key that types it printed in the corner. The tile→key map is C++ (`mvc/fontkeyboard.*` →
`brg.keyboard`) and **pinned by `tst_font_keyboard`** — every tile reachable, and the only duplicated
tiles anywhere are the two box-frame edges (the Tiles I page lays the frame glyphs out **as the box**:
`Q W E / A _ D / Z X C` draws one).

The doctrine, in one line: **a tile goes where a real keyboard would put it whenever it can, and must
never pretend.** Base layer = lowercase + digits + punctuation on their own keys; Shift = uppercase +
the real shifted symbols (`!`, `$`, `?` on `/`, `:` on `;`); a cap whose tile matches what the physical
keyboard would type **drops its corner legend**, because there's nothing left to teach. **Caps Lock
locks the Shift page** (Shift inverts it, Ctrl/Alt ignore it), and **touching a physical modifier drops
any latched page** — otherwise Shift silently does nothing on a clicked-in Uppercase page. The name row
has two explicit modes: **keyboard mode** (no text field at all — a label with a soft caret; Backspace
eats a whole tile) and **edit mode** (a real field, live-updating, keyboard faded out and dead; check
applies, cross discards). The old chip list, filter sidebar, tilemap view and `FontSearchModel` are
**deleted**. Design + the full map:
[`plans/full-keyboard-redesign.md`](plans/full-keyboard-redesign.md); conventions:
[`reference/ui-patterns.md`](reference/ui-patterns.md) → "The full keyboard's DECK".

**Next:** in-app review of the new keyboard (see "Pending rebuilds" below); an end-to-end save/reopen
verification pass; remaining per-control test depth. See [`plans/next-steps.md`](plans/next-steps.md).

## Pending rebuilds / awaiting in-app review

- **The new full keyboard — reviewed in-app across five rounds with Twilight and SHIPPED in
  0.16.6-alpha (2026-07-11).** Nothing outstanding. The live-only behaviours (typing with the caps
  flashing, held-vs-latched modifiers, Caps Lock, animated tiles, token-aware Backspace, the shake when
  a key won't fit, edit mode, Tab opening the tileset picker on the tile pages) were all exercised on
  the real build.
  ⚠️ Known environment caveat: on a Windows box with **two keyboard layouts installed**, the OS eats
  Shift+Alt / Ctrl+Shift (switch layout) and Ctrl+Alt (AltGr) — those pages are still reachable by
  clicking the modifier caps or the page strip, which is exactly why they latch.
- **File-load crash fix (`s14`) — needs a kit rebuild.** C++ changed (`savefile.cpp`,
  `filemanagement.cpp/.h`, `router.cpp`) **and** a new QML file was added
  (`screens/modal/FileError.qml`, already in `app.qrc`). After building, test: (1) a recent file whose
  path no longer exists → silently dropped from the list on next launch, never crashes; (2) a
  present-but-truncated/locked `.sav` → shows the `FileError` modal, save stays untouched. ⚠️ Not yet
  build-verified on the dev machine.
- Several recent QML/asset passes are **awaiting in-app review** (see the latest entries in
  [`sessions/`](sessions/README.md)).

> **Build reminder:** rebuild the **kit dir**
> (`projects/build/Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug`) for in-app testing — not just `build/`.
> New `.qml` files MUST be added to `app/app.qrc` or they fail at runtime ("Type X is not a type").
> Editing a `savefile` `.cpp` rebuilds the **DLL**, not the exe — verify by the DLL timestamp.

## Open issues

| Issue | Where | Status / notes |
|-------|-------|----------------|
| ~~Glitch / half-baked maps don't render~~ **RESOLVED 2026-07-12 — they all render now** | `MapEngine::sourceMap()` | They were never a data gap. `maps.json` **already models them**: every one of the 25 (22 unsized "Unused Map XX" + the 3 sized `*_Copy`) carries an `incomplete` field naming the map it is an unfinished duplicate of — and it agrees **exactly** with the ROM's header-pointer table (11 → Saffron City, 105–117 → Lance's Room, 204–206 → Rocket Hideout Elevator, 231 → Route 16 Gate 1F, 237–244 → Silph Co 2F, 69 → Trashed House, 75 → Path Entrance Route 6, 173 → Cinnabar Mart). The renderer now follows that link and draws the map they copy — which is precisely what a Game Boy loading that id puts on screen. **No JSON was changed and nothing was invented.** The screen says plainly that it's showing an unfinished copy, and of what. **All 248 ids render** (`tst_map::everyMapIdRenders`); only "Last Map" (255) is genuinely empty. |
| ~~🐞 `AreaAudio::setTo()` clobbers the track id with the bank~~ **FIXED 2026-07-12** | `savefile/…/expanded/area/areaaudio.cpp` | It was `musicBank = musicID = musicEntry->bank;`. Fixed and pinned by `tst_area::audio_setTo_keepsIdAndBankApart` (every map in the game). ⚠️ Writing that test proved the bug was **dormant**: `MapsDB` is never deep-linked at boot, so `getToMusic()` is null for every map and `setTo()` has been writing `0/0`. **The moment the deep-link landmine below is defused, this code path goes live** — which is exactly why it's fixed now. |
| **Latent landmine: map DB `getToMap()`/`getToSprite()` never resolved** | `db.cpp` `deepLinkAll()`; consumers in `WarpData`/`MapConnData`/`SpriteData`/`AreaMap` | Still dormant, and the new map screen **deliberately does not touch those accessors** (it resolves the tileset by name and looks maps up by id, so it needs no deep link). Not a crash today. **Still must be defused before map *editing* / re-enabling map randomize** — those will dereference the unresolved `to*` links → add `MapsDB::inst()->deepLink()` to `DB::deepLinkAll()` first. Confirmed safe once called, via `tst_sprite_data` (all 918 sprites resolve). |
| Randomizer: not-yet-built screens (Maps, Hall of Fame, Options) excluded | `savefileexpanded.cpp`, `worldgeneral.cpp` | **Working within scope as of 2026-06-07.** `randomizeExpansion()` runs end-to-end + is test-covered. Maps/HoF/Options calls are commented out (matching the disabled home tiles), each with a re-enable note. Re-enabling map randomize is gated mainly on calling `MapsDB::inst()->deepLink()` at boot (the type strings + per-call guards turned out to be the same deepLink landmine, not separate defects). |
| Name editors — ongoing review | `name-full/*`, `general/NameDisplay.qml` | Ongoing live tweaks. `NameEdit`/`NameDisplay` are **shared** by player/rival/nickname + the keyboard footer preview — verify all of them on each rebuild. |
| Keyboard caps are cramped at the default 750×480 window | `name-full/KeyboardDeck.qml` | By design it *scales* (key unit = min(width/13.5, height/6.0)), so it's comfortable on a resized window and tight on the default one. Multi-char code labels (`trainer`, `player`) elide at the smallest size. Revisit if Twilight wants the default window bigger, or the header/footer slimmer, to buy the deck more room. |
| Dead menu files (unused after s13z7) | `name/NameDisplayMenu.qml`, `NameDisplayMenuNoTileset.qml`, `TilesetMenu.qml` | No longer instantiated; left in place + in qrc. Safe to delete later. |

**Intentional (not bugs):** in the storage grid, Pokémon names are always visible below each icon
(dark text, no background). The player **ID** commits on Enter/focus-out (not per keystroke) —
revertible if wanted live.

## Testing

A comprehensive automated suite lives under `projects/tests/` (QtTest + CTest). **Full `ctest` is
green (75/75 on the Qt 6.11 kit).** Newest: **`tst_map`** (18 cases) — the block data, the overworld
buffer, the view maths, the renderer and `brg.map`. Its keystone is
`viewPointer_matchesWhatTheGameStored`: it recomputes the view pointer the *Game Boy* wrote into the
save and demands a byte-exact match. That test is the map emulator's foundation — if it goes red,
nothing downstream of it can be trusted. Library-layer line coverage is at/above 90% (common 100%, db
~90%, savefile ~90%; app layer is the laggard). The Linux Docker env runs four variants green
(standard / asan+ubsan / xvfb / coverage **89.98%** as of 2026-06-22). A QML-load smoke test
(`tst_qml_screens`), a real-app GUI suite (`tst_gui_*`), signal/slot (`tst_signals`), model-contract
(`tst_model_tester`), visual-regression (`tst_visual_regression`) and BDD acceptance (`tst_acceptance`)
suites gate `main`. The road to "100%" (3 gap kinds; only the reachable-fillable one is worth chasing)
is mapped in [`plans/testing.md`](plans/testing.md) → "Coverage status". A **static-analysis layer**
(clang-tidy + cppcheck +
informational qmllint, via `scripts/lint.*` and a `lint` CI workflow) was added 2026-06-22 — the
clang-tidy gate is clean (143 TUs, 0 findings) and surfaced/fixed 8 real defects (see version.md).
Strategy, coverage baseline, and remaining gaps: [`plans/testing.md`](plans/testing.md).

## Build health

| Layer | Status |
|-------|--------|
| common | ✅ Clean |
| db | ✅ Clean |
| savefile | ✅ Clean (`Q_DECLARE_OPAQUE_POINTER` only on untraversed types; `qmlownership.h` in place) |
| app | ✅ Clean |

Build speed restored s13c (over-includes trimmed). `dllimport` warning silenced via
`-Wno-ignored-attributes` in root `CMakeLists.txt`.

## Runtime health

| Area | Status |
|------|--------|
| Window / DB load / file open+save | ✅ |
| `dataExpanded.*` chain — all screens read + persist | ✅ |
| Trainer Card / Bag / Pokémon storage data | ✅ confirmed |
| Trainer + Rival name render (animated) + persist | ✅ (was QML GC of FontDBEntry) |
| Pokémon box: click → details opens; no crash | ✅ (GC crash fixed) |
| Pokémon box: hover name (+ pen icon) | ✅ |
| Combo box (Select*) popups scroll on long lists | ✅ (capped popup height) |
| Badges; Pokédex toggles | ✅ |
| Number fields (playtime / item count / PP) width + centering | ✅ |
| Trainer-card layout — centered box, compact fields, clock width | ✅ confirmed |
| Randomize name (full editor + trainer screen) | ✅ |
| Pokémon **editor** responsiveness + layout/styling | ✅ confirmed |
| Pokémon editor **Moves tab** — grouped-panel restyle + drag-to-reorder (`reorderMove`) | ✅ tests green; in-app review pending |
| Name editors (nickname / player / rival) — popup + full keyboard | ✅ iterated |
| Player/rival name + player ID — atomic commit-on-finish, no hang, no OT corruption | ✅ |
| Tileset picker — every tile clickable; no freeze on variable render | ✅ |

## Recurring non-fatal warnings (harmless)

- `'dllimport' attribute ignored` on `MapDBEntry` etc. — Qt + llvm-mingw shared-lib cosmetic;
  **silenced** via `-Wno-ignored-attributes`.
- Items "could not be deep linked" / "Values are not correct on sprite X" — pre-existing data.
- On exit: `QDxgiVSyncService not destroyed in time`, `QThreadStorage entry N destroyed…` — benign Qt
  shutdown ordering.
- Offscreen test runs: `QFontDatabase: Cannot find font directory` — benign (allowlisted in the GUI
  harness).

## The "crashes" — two different things

1. **System-wide** Qt-debugger pop-ups (also in Notepad/taskbar/Settings) — environment/Qt-install
   issue, NOT this app. Don't chase.
2. **In-app** use-after-free from QML GC'ing parentless C++ QObjects — **fixed**. A silent "terminated
   abnormally" after interaction was this. If a real in-app crash recurs, get a project-debugger stack
   trace. Details: [`reference/diagnostic-methods.md`](reference/diagnostic-methods.md),
   [`sessions/`](sessions/README.md).
