# pokered-save-editor-2 — AI Context

Pokemon Red & Blue save file editor. Qt 6 C++/QML desktop app. Open source, built by Twilight.
Originally built 2017-2020, revived in 2026.

## Start Here

Read `notes/status.md` first — it has current build health, what's working, and what to do next.

The full notes system is in `notes/`. Everything is organized by topic:

| File | What's in it |
|------|-------------|
| `notes/status.md` | **Current state** — build/runtime health, open issues, immediate actions. Start here |
| `notes/sessions/` | **Session logs**, one file per day grouped in month folders (`YYYY-MM/YYYY-MM-DD.md`) — the day-by-day story of what changed and why. `sessions/README.md` defines the system; `revival-s13-series.md` holds the undated pre-corruption revival narrative |
| `notes/version.md` | **Changelog** — the plain-English, one-entry-per-commit history (index; months under `notes/version/`). NOT the version-number scheme (that's `reference/versioning.md`) |
| `notes/context/project.md` | What the project is and its goals |
| `notes/context/collaboration.md` | **Working with Twilight** — the consolidated standing preferences, working rules & cross-cutting feedback (who she is, content/spelling rules, data/JSON, git & MANUAL releases, tooling, credits, never-change list). The canonical home for what used to live in AI "memory" |
| `notes/context/architecture.md` | Codebase structure, build system, key patterns |
| `notes/context/principles.md` | Project philosophy — what the app must/must not do |
| `notes/context/origins.md` | The 2019–2020 pre-revival story — three rewrites, the JS detour, the library/DB refactor |
| `notes/context/history.md` | How the project was revived — what was broken, what was fixed |
| `notes/systems/overview.md` | **System map** — in-depth macro/micro architecture: layers, boot, data flow, byte-fidelity. Per-layer deep-dives in `notes/systems/{common,savefile,db,app,qml}.md`. Start here to understand the machine |
| `notes/reference/fix-patterns.md` | Compiler/runtime error → fix lookup table |
| `notes/reference/qt-patterns.md` | **The Qt/QML reference** — the project-lifetime catalog of every Qt/QML landmine (2019→2026) + the detailed Qt 5→6 patterns (with code) + case studies. (Merged: was `qt6-patterns.md` + `qt-gotchas.md` + `player-name-hang.md`) |
| `notes/reference/gen1-knowledge.md` | Gen 1 Red/Blue **save-format + gameplay** domain knowledge (offsets, checksum, badges, trade status, retroactive natures/shininess, randomizer rules); box-recovery deep-dive in `notes/reference/box-recovery-research.md` |
| `notes/reference/diagnostic-methods.md` | How to find and fix systemic problems (truncation, hangs, QML chain failures, transcript recovery) |
| `notes/reference/ui-patterns.md` | **UI/QML conventions** — layouts, borderless combos, ⋮ buttons, editor popups, sliders, drag & drop, View All drawers. Read before UI work |
| `notes/reference/screenshots.md` | **UI screenshot capture** — the headless `screenshooter` tool + capture scripts that render the live UI to `tmp/screenshots/` as **still PNGs** (offscreen, no save writes). How it's driven + the font/backend gotchas. (No automated GIFs — animated GIFs are added manually, one at a time) |
| `notes/reference/map-connections.md` | **Connection strips** — the border ring carries the *neighbouring maps'* edges, not the border block. The macro's clamp (one signed offset → two numbers), the two different loop shapes, the ROM-**pointer** source. **78/78 verified against the cartridge.** ⚠️ Records that `MapDBEntryConnect::stripSize()` is **wrong** and `maps.json`'s `flag` is a patch for it |
| `notes/reference/tiles.md` | **Tiles / blocks / tilesets** — the *meaning* layer of the map. The three layers and which field belongs to which. The big one: there is **no Indoor/Cave/Outdoor byte** — the save's `type` (`0x3522`) is `sTileAnimations` (**NONE / WATER / WATER+FLOWER**), and `tileset.json`'s names are a **verified 1:1 rename** of it. **Collision is not in the save**: `collPtr` points at a ROM list of *passable* tiles, so "wall" is derived (and the lists are **shared** — which is how 3 `collPtr`s in `tileset.json` came to be wrong; fixed + pinned). `outOfBoundsBlock` is a **BLOCK**; `boulderIndex`/`boulderColl` are **Strength-push scratch**, not map data. Plus every per-tileset semantic the ROM hands us free (warps, doors, ledges+direction, water, counters, bookshelves, elevation edges, cut trees). **Read before any map/tile UI work** |
| `notes/reference/sprites.md` | **Sprites** — the OAM hardware (40 objects, the (16,8) bias, colour 0 always transparent, the attribute bits) and Gen 1's 16-slot overworld system on top of it. The traps: **there is no right-facing sprite** (it's left, mirrored), **no second walking frame** (it's the same one, flipped), and sprites sit **4 px above** their tile row (measured off the console's OAM). Where the NPC/grass-priority work starts |
| `notes/reference/palettes.md` | **Palettes + "contrast"** — the save's `contrast` byte is `wMapPalOffset`, which the game **subtracts from a pointer** into its fade-palette table: `0/3/6/9` = the 4 real levels, everything else reads across a seam = **the 6 glitch palettes**. All ten verified against the console's palette registers |
| `notes/reference/gameboy-apu.md` | **The Game Boy sound chip** — 4 channels (2 pulse + wave + noise/LFSR), every register `$FF10`–`$FF3F`, the 512 Hz frame sequencer, the frequency maths (`131072/(2048-x)`; the wave channel an octave below), DAC-off = channel dead, trigger semantics, and the `GbApu` we will build. Read before any audio work |
| `notes/reference/gen1-sound-engine.md` | **The game's sequencer** — the byte-code that drives that chip: the full `$10`–`$FF` command set, the header format, the 3 banks (2/8/31 — and **engine 1 == engine 3**, so build ONE engine), the pitch table's signed-negative trick, the load-bearing bugs to copy verbatim, the **243-byte state at `$C000`** (the verification oracle), and the **two save flags verified**: `0x29D8` bit 1 = `BIT_NO_AUDIO_FADE_OUT`, `0x29DF` bit 1 = `BIT_NO_MAP_MUSIC`. Plan: `plans/music.md` |
| `notes/reference/glitch-music.md` | **Glitch music ids** — verified on the cartridge. Most "glitch" ids are NOT garbage: a header is 3 bytes per channel, so a 3-channel song eats 3 ids and the spares are its **inner voices** (id 187 = Pallet Town's bassline, alone). **105 of them**, all playable from the data we already import → **151 tracks for the price of 46**. SFX ids play a sound effect once; id 0 fades out then hits a drum; id 255 is silence. ⚠️ The **BANK** byte is a loaded gun: an invalid bank executes arbitrary ROM as code — **the console hangs**. Tools: `scripts/emu/analyze_music_ids.py`, `scripts/emu/probe_glitch_music.py` |
| `notes/reference/map-animation.md` | **The map, ANIMATED** — what the console does every frame, verified against the disassembly: `UpdateMovingBgTiles`, the **one save byte** (`0x3522`) that drives it, the **20/21-frame cadence**, the fact that **water has no frames — its tile's 16 bytes are bit-ROTATED** (right ×4, left ×4), the flower's `1,1,2,3`, the coupled counters, what **hack values** do (odd → water-only, even → water+flower; **0 breaks Surf**), the sprite traps, and the **determinism rule** (renderer takes a frame number; screenshots/tests render frame 0). **Read before any map animation work** |
| `notes/reference/warps.md` | **Warps** — the doors, and the twelve bytes around them. **The linchpin**: `LoadMainData` *sets* `BIT_NO_PREVIOUS_MAP` on the saved tileset byte, so `LoadMapHeader` **bails out on Continue** and the save's own warp list is the one the console runs on (the game restores the map's original doors on re-entry — the sprite rule, again). The traps: **`wStatusFlags3` is ZEROED on every load** (it shares a byte with `wCableClubDestinationMap`, which `SpecialEnterMap` clears) so `scriptedWarp`/`isDungeonWarp` — and `npcsFaceAway`, `tradeCenterSpritesFaced`, `isBattle`, `isTrainerBattle` — **can never survive a save**; `wWarpedFromWhichWarp`/`Map` are **written and never read** (dead); and **two destination bytes are loaded guns** (`wDestinationMap` has **13** legal values, the dungeon map+hole pair has **12**, neither is bounds-checked) — which `AreaWarps::setTo()`/`randomize()` currently fill with **random illegal values**. The two bytes that actually matter (`wLastMap`, `wLastBlackoutMap`) live in `WorldGeneral`, not `AreaWarps`. **All verified on the cartridge** (`scripts/emu/probe_warp_persistence.py`). Plan: `plans/map-screen.md` → Phase 5 |
| `notes/reference/player-state.md` | **The Player** — position, facing, and the **26 bytes of map state** in `AreaPlayer` (the source of v1's Direction/Coords/HMs/Battle/Warps/Other page). The headline: **ten are rewritten the instant the save loads** — `wPlayerDirection` is **forced to DOWN** every Continue, `wStatusFlags3` (`isBattle`/`isTrainerBattle`) is **zeroed** whole, `strengthOutsideBattle` is **reset** (unless the battle-over bit is set — a real interaction the probe caught), `battleEndedOrBlackout`/`usingLinkCable`/the door + ledge move-bits are **cleared**, `Jumping Y` is **zeroed**; **three are dead** (`x/yOffsetSinceLastSpecialWarp`, `usedCardKey` = `BIT_UNUSED_CARD_KEY`, `; never checked`); and several are **misnamed** (`isBattle`=`BIT_TALKED_TO_TRAINER`, `flyOutofBattle`=`BIT_USED_FLY`, `finalLedgeJumping`=`BIT_LEDGE_OR_FISHING`). Byte offsets/bits are all **correct** — it's a truth-in-labelling fix, its own phase before UI. **All 26 verified on the cartridge** (`scripts/emu/probe_player_state.py`; ⚠️ `wMovementFlags` clears bits per-routine, NOT whole-byte — a read of the asm alone gets it wrong). Plan: `plans/map-screen.md` → Phase 5e |
| `notes/reference/area-map-state.md` | **The "Map" page** — the seven leftover `AreaMap` state bytes from v1's Map/Area page (**Current Script, the two UL-corner view pointers, Run cur map script instead, force bike ride, to blackout dest, card key door X/Y**). Two are durable/editable (`wCurMapScript`, `BIT_ALWAYS_ON_BIKE`), one is **derived and trusted-on-load** (`wCurrentTileBlockMapViewPointer` — editing coords desyncs it; show + recompute, never a raw address), three are **rewritten every load** (`wMapViewVRAMPointer`→`$9800`, `wCardKeyDoorX/Y`→0, `BIT_USE_CUR_MAP_SCRIPT` transient), and **"to blackout dest" is a ghost** — already moved to `AreaWarps::escapeWarp`. ⚠️ The probe overturned **two** source-reads (the view pointer is *kept* not recomputed → drew garbage from `$FFFF`; `BIT_USE_CUR_MAP_SCRIPT` *survives* on a quiet map). No corruption bug, no gun — naming + one derived-value discipline. **Verified on the cartridge** (`scripts/emu/probe_area_map_state.py`). Plan: `plans/map-screen.md` → Phase 8 (Area State) |
| `notes/reference/npc-character-state.md` | **Character-state flags** (v1's "NPC" page → the right-hand **Character panel**) — nine **map-global** (not per-NPC) NPC/control/battle bits: `npcsFaceAway`, scripted-movement init/running, ignore controls, scripted controls, `runningTestBattle`, `trainerWantsBattle`, `wTrainerHeaderPtr` (+ hidden trade-center bit). **Verified against the disassembly** (`WRAM = save+0xAD54`: `0x29D9`=`wStatusFlags3`, `0x29DA`=`wStatusFlags4`, `0x29DC`=`wStatusFlags5`, `0x29DF`=`wStatusFlags7`, `0x2CDC`=`wTrainerHeaderPtr`). **All nine are transient script/battle/link scratch**; three v1 labels are **wrong** ("Scripted Battle" = the **debug** `BIT_TEST_BATTLE`; "Scripted Controls" = `BIT_SCRIPTED_MOVEMENT_STATE`; "Face Away" = `BIT_NO_NPC_FACE_PLAYER` = *don't turn to face*); v1's `areanpc.{h,cpp}` carried all the wrong names — **renamed + doc'd 2026-07-15** (byte-identical output; `tst_area_npc` pins offsets + proves fidelity). **Persistence console-verified** (`scripts/emu/probe_npc_character_state.py`): **4 rewritten on load** (both Sprites bits zeroed; `disableJoypad`/`scriptedMovementActive` cleared) — the source-read guess that "all transient bits vanish" was **wrong**, the probe caught it; **5 kept** (`initScriptedMovement`, `scriptedNpcMoving`, `testBattle`, `trainerBattle`, `wTrainerHeaderPtr`). Plan: `plans/map-screen.md` → **Phase 9** — 9a research ✅ · 9b model-fix ✅ · 9c probe ✅ · 9d panel ✅ (`CharacterStatePanel.qml`, awaiting Twilight's live pass). **No hidden fields** (Twilight 2026-07-15). |
| `notes/reference/wild-encounter-cooldown.md` | **The post-battle wild-encounter cooldown** — v2's `pauseMons3Steps` (save `0x29D8` bit 0) is `wStatusFlags2` bit 0 = **`BIT_WILD_ENCOUNTER_COOLDOWN`**: the console **sets it after every battle**, and on the next map entry (`EnterMap`, which the **Continue path reaches** via `SpecialEnterMap`) a set bit loads `wNumberOfNoRandomBattleStepsLeft = 3` ("minimum steps between battles"), decremented per step until it **self-clears**. So its save-editing effect is exact: **3 encounter-free steps on the next Continue**. **DURABLE** — kept on load (no `!`), **console-verified** (`scripts/emu/probe_wild_encounter_cooldown.py`: bit kept + steps-left 3; cleared control → 0). ⚠️ **`BaseSAV` already carries it set.** Same byte as the audio-fade flag (bit 1) — no collision. Renamed `pauseMons3Steps` → **`wildEncounterCooldown`** (2026-07-15); on the **map details page** as "3-step wild encounter cooldown". |
| `notes/reference/wild-encounters.md` | **Wild Pokémon** — the map's grass + water encounter tables. `wGrassRate` `0x2B33` + 10 slots `0x2B34`; `wWaterRate` `0x2B50` + 10 slots `0x2B51`; **rate 0 = none**. ⚠️ Each slot on disk is **`LEVEL, then SPECIES`** (pokered `db level, species`; `BaseSAV` `0x2B35` = 165 = RATTATA) — the model read species-first through 0.39.x, **inverting every real save; FIXED** (pinned by `tst_area_pokemon::wildTables_byteOrderIsLevelThenSpecies`). Species is the **internal index** (== `PokemonDBEntry::ind`), not the Pokédex number (the DB's `pokedex` is **0-based** — the box's `+1` icon trick). A slot's rarity is its **position** (pokered `WildMonEncounterSlotChances`: 19.9/19.9/15.2/9.8×3/5.1×2/4.3/1.2), so reordering re-weights it. **Live on Continue** — `LoadWildData` is inside `LoadMapHeader` *after* the `BIT_NO_PREVIOUS_MAP` early-return, the **same linchpin as warps/signs** (restored on re-entry). UI: `WildPokemonPanel`/`WildMonList` (Phase 8). Plan: `plans/map-screen.md` → Phase 8 |
| `notes/reference/gym-safari-state.md` | **Gym & Safari minigame state** — the six bytes for the planned **Maps** panel: `wFirstLockTrashCanIndex` `0x29EF` / `wSecondLockTrashCanIndex` `0x29F0` (Vermilion trash puzzle), `wOpponentAfterWrongAnswer` `0x2CE4` (Cinnabar "next wrong answer" trainer), `wSafariSteps` `0x29B9` (**2‑byte BIG‑endian**, 502), `wSafariZoneGameOver` `0x2CF2`, `wNumSafariBalls` `0x2CF3` (map `WRAM=file+0xAD54`). ⚠️ **Twilight's premise was inverted:** these are **NOT** map/Area state, **NOT** per‑map, and **not** scratch — they're **global Main‑Data** event bytes, one each save‑wide, and this **is** the persistent copy (v1 filed them under `AreaPlayer`/`AreaPuzzle` by theme, which is what suggested "map state"). **Armed vs inert:** durable but only consulted mid‑activity (trash mid‑Vermilion‑puzzle, opponent mid‑Cinnabar‑quiz, safari counters only inside the zone — re‑set to 502/30 on gate entry); **`wSafariZoneGameOver` is transient** (rewritten every step → inert, amber‑! group). Traps: the step counter is big‑endian; **`wGymTrashCanIndex` `0xCD0D` is a decoy — NOT saved**. Want a **global** model home, not `Area*`. ✅ **console‑verified** (`probe_gym_safari_state.py`): **5 of 6 survive Continue** (addresses + big‑endian pinned); **`wSafariZoneGameOver` is zeroed on load** (OverworldLoop → SafariZoneCheck every frame) → truly inert, amber‑! group. |
| `notes/reference/signs.md` | **Signs** — the placards, and the map text behind them. Save layout (`wNumSigns` `0x275C`, `wSignCoords` Y,X `0x275D`, `wSignTextIDs` `0x277D`; **16 max**, `MAX_BG_EVENTS`), verified against `wram.asm`. **The model is already correct** — the rare pass with **no bug to fix first** (contrast sprites/warps). Rides the **same persistence linchpin as warps** (`.loadSignData` is inside `LoadMapHeader`, behind `BIT_NO_PREVIOUS_MAP`): an edited sign is live on Continue, restored on re-entry. **The text id is a 1-based index into the map's `def_text_pointers` table** → resolves through `text_far` to the real string; a map's text table is **shared** by signs (`bg_event`), people (`object_event`) and script-only entries, which is why the picker **groups** them. `maps.json` ships only the id today — the **words must be extracted from `pret/pokered`** (Phase 6a). Plan: `plans/map-screen.md` → Phase 6 |
| `notes/reference/sprite-sets.md` | **Sprite sets** (v1's "cached sprites") — the Game Boy holds only **11** overworld sprite pictures (9 walking + 2 still), so every outdoor map names one of **10 sets** (Pallet & Viridian, Pewter & Cerulean, … Fuchsia); 12 big routes **split** on a dividing line. The save caches the 11 pictures + the set id at `0x2649–0x2654` — and **the game throws it away**: `LoadMapData` zeroes the id on every map load (CONTINUE included) and `InitOutsideMapSprites` recomputes it. ⚠️ A **split id (`$F1`–`$FC`) is never stored** — the console resolves it first (was a real bug in our `loadSpriteSet`, fixed 2026-07-13) |
| `notes/reference/emulator-verification.md` | **The actual Game Boy checks our work** — `tst_emu_parity` boots the real ROM (PyBoy, headless) with one of our saves, reads the console's own RAM + framebuffer, and demands `MapEngine` match byte-for-byte. Setup, the ROM rules (git-ignored, never committed/shipped; SKIPs without it), licensing, and the traps (the "has the save loaded?" trap; `wCurMapTileset` bit 7). **Read before any map/render work** — it is the oracle |
| `notes/reference/dev-harness.md` | **Debug automation harness + fast-dev loop** — DEBUG-only launch flags (`--sav`/`--screen`/`--hot`/`--shot`), the `127.0.0.1:8766` live TCP control channel, and QML hot-reload. How to launch straight to the screen under edit with a save loaded and preview edits live |
| `notes/reference/i18n.md` | **Translations** — Qt Linguist pipeline (`qsTr`/`tr` → `.ts`/`.qm` at `:/i18n`, `QTranslator` in boot), how to add a language; language switching deferred until a 2nd locale + Options screen exist |
| `notes/reference/documentation.md` | **Docs** — generating the Doxygen site, the comment house-style, and the doc-pass progress ledger (all merged here) |
| `notes/reference/git-workflow.md` | **Git standards** — the fairyfox **git-flow** model (`main` = `--no-ff` tagged releases, `dev` integration, `feature/`/`release/`/`hotfix/` branches; PATCH releases direct, MINOR/MAJOR via `release/*`), no history rewriting, commit-message style, hard safety rules. Read before any git op |
| `notes/reference/cross-project-sync.md` | **Fairyfox mesh** — how this project shares standards with the fairyfox.io hub: git-only, read-only, on-request sync (no submodules/automation). The model behind "check the fairyfox system for updates" |
| `notes/reference/versioning.md` | **Version-number scheme** — SemVer, single source of truth (`VERSION`), how it propagates (CMake → `pse_version.h` → app/About/.exe), how to bump, release/tag process. (The *changelog* is `notes/version.md`) |
| `notes/reference/deployment.md` | **Releases / deployment** — the GitHub Actions `release.yml` pipeline: builds Windows portable+zip+installer, Linux AppImage+tar.gz, Doxygen docs zip, screenshots zip, and publishes a GitHub Release on each `main` commit that bumped `VERSION` (tag-gated). Toolchain mirrors `tests.yml`; first-run shakeout points noted |
| `notes/decisions/architecture.md` | Key structural choices and why |
| `notes/decisions/rejected.md` | Things tried that failed — do not repeat |
| `notes/plans/next-steps.md` | Ordered task list |
| `notes/plans/music.md` | **Music** — the six-phase plan to put the audio flags, the track picker and **real, accurate music playback** (hover-to-preview) on the **Map screen**: import the music data from `pret/pokered`, port the Gen 1 sequencer + a DMG APU into a new `pse-audio` library, verify it frame-by-frame against the console. Decisions, risks, and the open design questions for Twilight |
| `notes/plans/map-screen.md` | **The Map screen** — the complete UI/UX overhaul (approved 2026-07-12): the research (Tiled / Photoshop / Aseprite — what we take, what we reject), the **chassis** (identity bar · tool rail · context bar · dark canvas well · collapsing icon **dock**, one panel at a time, never stacked · status bar), the **4-group layer tree** (Guides / Meaning / Game View / Objects — the red screen box, the accent draw area and the player are LAYERS), **on-canvas object editing** (warps/signs/NPCs/connections: select, drag, add, delete), **frame-accurate animation + the optional walk simulation**, and the **field→home table for every byte of the Area block**. **Thirteen deep phases + one optional** — read it before ANY map-screen work |
| `notes/plans/testing.md` | **Testing** — the suite (QtTest/CTest, GUI harness, Docker variants, coverage) and remaining gaps. Live, not a blueprint — full `ctest` is green |
| `notes/plans/future.md` | Longer-term ambitions |

## USE THE GAME'S OWN FILE FORMATS (a standing rule, 2026-07-13, Twilight)

> **Where `pret/pokered` has a format, WE USE THAT FORMAT. By default, without asking.**

Not "a format inspired by theirs", not "our own JSON of the same data" — **theirs**. Their bytes, their
text, their macro names, their directory shape. It is the format the game's own source is written in, it
is what every other tool in this scene reads and writes, and every hour spent inventing a parallel one is
an hour spent building a thing that can only talk to itself.

| Data | The format | Why |
|---|---|---|
| Map blocks | `.blk` — **raw block bytes, exactly as `pret/pokered` stores them** | Already done. Byte-identical; anyone's `.blk` drops straight in. |
| Music | **the game's own `.asm` sheet music**, parsed **line by line** | ✅ Done (0.34.0-alpha). We ship their `audio/**.asm` **verbatim** (376 files, in the qrc) and `Gen1MusicAsm` reads it — a **line parser**, because their macro names *are* the command names (`note C_, 8`, `octave 4`). It assembles to the game's own bytes, and `tst_music_asm` proves that **byte-identical to a real cartridge**. The `.bin` blobs we used to ship are gone. |
| Sprites | one loose `.png` per sprite | Not an atlas. You can open one and look at it. |
| Everything else | **whatever `pret/pokered` uses** | The default. If you are about to invent a format, stop and go and look at what they do first. |

**The rule for anything new:** before designing a data format, **open `pret/pokered` and find theirs.**
Deviating is a decision that has to be argued for and written down in
[`notes/decisions/architecture.md`](notes/decisions/architecture.md) — not a thing that happens because
nobody looked.

Full write-up: [`notes/reference/file-formats.md`](notes/reference/file-formats.md).

## RESEARCH LANDS IN THE NOTES — every time, by default (a standing rule, 2026-07-14, Twilight)

> **If you understood something you did not understand before, WRITE IT DOWN — in `notes/`, in the same
> session, without being asked.**

Not "if it seems important". Not "at the end of the project". **By default, in every chat.** A new or
expanded understanding — a save byte's real name, what a routine actually does, a value the console
can't survive, a field that turns out to be dead, a bug the model has been carrying — is the most
perishable thing this project produces, and it is worth more than the code that came out of it. Code
can be rewritten from notes. Notes cannot be rewritten from code.

**The shape of a research pass, and none of these steps is optional:**

1. **Go to the primary source.** `pret/pokered` for the game, the cartridge for the truth. Not memory,
   not v1, not what the field is *called*.
2. **Ask the console when it matters.** `scripts/emu/` — a careful reading of the assembly has been
   **wrong before** (the sprite persistence pass, 2026-07-13) and the emulator caught it. If a
   conclusion is load-bearing, **probe it**, and commit the probe.
3. **Write the reference note** — `notes/reference/<topic>.md`: real names, addresses, ranges, who
   writes it, who reads it, what a console does with a hack value, and the traps. Plain English, so a
   person who does not already know the domain can learn it from the file.
4. **Say what it means for OUR code.** Every research pass so far has found real bugs in the model
   (sprites: four; tilesets: three wrong collision pointers; warps: seven). List them, and fix them in
   a phase of their own **before** any UI is built on top.
5. **Wire it up** — a row in this file's notes table, a `\subpage` in `notes/_nav.dox`, a line in
   `notes/status.md`, an entry in today's session log, and the plan it feeds.

**Then, and only then, design the UI.** A screen built on a field whose name is a guess is a screen
that will have to be built twice.

## Critical Things Not to Get Wrong

- **Do NOT put `load()` in DB constructors** — causes Qt 6 static-init deadlock. See `decisions/architecture.md`.
- **Do NOT use `qt_add_qml_module()`** — conflicts with `app.qrc`, hangs the app. See `decisions/architecture.md`.
- **Do NOT remove the parameter from `SaveFile::dataExpandedChanged`** — the signal is correct. See `decisions/rejected.md`.
- **Do NOT change `(dexInd+1)` arithmetic in Pokedex.qml** — 0-indexed, +1 is correct. See `decisions/rejected.md`.
- **Do NOT call `new XxxDB()`** — all DB classes have private constructors. Use `XxxDB::inst()`.
- **Do NOT access DB entry fields directly** — all fields are protected. Use getters (`entry->getName()` not `entry->name`).
- **Do NOT `Q_DECLARE_OPAQUE_POINTER` a QObject type you traverse in QML** — it forces `IsPointerToTypeDerivedFromQObject = false`, so QML reads the whole `brg.file.data.dataExpanded.*` chain as `undefined`. Fully `#include` the type's header instead. This (not missing `qRegisterMetaType`) was the real cause of the long-standing "undefined chain" bug, fixed in session 13. See `notes/reference/qt-patterns.md`.
- **Do wrap any `Q_INVOKABLE` that returns a QObject in `qmlCppOwned()`** (`pse-savefile/qmlownership.h`). Q_INVOKABLE returns of a parentless QObject default to JavaScriptOwnership and get garbage-collected by QML mid-session → dangling pointer → use-after-free crash. (Q_PROPERTY returns are safe; Q_INVOKABLE returns are NOT.) All existing `…At()` methods were fixed in session 13h. See `notes/reference/qt-patterns.md`.
- **Do NOT write any save-file bit or byte you weren't explicitly instructed to change.** Bit- and byte-exact fidelity is a top-tier project value: the editor changes *only* the exact bits and bytes for the edit and leaves every other bit and byte of the save totally untouched — even unused/unallocated bits are precious; a single unintended bit flip is unacceptable (this has been verified over many hours of manual testing). Never "rewrite/normalize the whole save," never reorder/repack, never touch checksums/regions you weren't told to. Corrupting a save is among the worst possible outcomes. See `notes/context/principles.md` → "Save File Integrity Is Sacred".
- **No hacks, no temporary fixes, no bad fallbacks.** The quality bar here is high — UX is the #1 priority and there is no room for clunky/janky/interrupting behavior. Prefer the correct, clean solution even when it's the longer route; if you can only see a hacky path, surface it and ask rather than commit it. See `notes/context/principles.md` → "What the App Should Feel Like".
- **Do the LONG WORK on every component — no rough-in, no "clean it up later" (2026-07-12, Twilight; mandatory).** Big features are built as **phases, one body of work at a time**, and a phase is *finished* — designed, built, screenshot-reviewed, tested, documented — before the next begins. There is no later. A phase that is 90% done is not done. When work is deep and complex (the map screen is the standing example), **plan it across more phases rather than fewer**, and put the hours into each one. Getting it comprehensive and right outranks getting it soon. See `notes/plans/map-screen.md` → "The programme".
- **The MAP screen has a design of record — follow it.** `notes/plans/map-screen.md` is the approved design: the **collapsing icon dock** (one panel at a time — panels never stack out and never evict each other), the **4-group layer tree** (Guides / Meaning / Game View / Objects; the red screen box, the accent draw area and the player are **layers**, grouped and toggleable), on-canvas **object editing** (drag/select/add/delete warps, signs, NPCs), and **every byte of the Area block editable — hack and glitch values included, shown and never silently rewritten**. Nothing about the map screen gets designed ad hoc in a chat; it gets designed in that file first.
- **DON'T BUILD WHAT TWILIGHT HASN'T BRIEFED — adjacency is not a brief (2026-07-14, Twilight).** *"I'd hate to have to undo a lot of work because it was done before I explained anything."* A feature gets **its own conversation first**, then research, then a design written into the plan, then code. **A phase does NOT get to absorb a neighbouring feature because the data happens to sit next to it in the save.** Signs nearly rode into the warps phase on exactly that logic (same ROM block, same shape) — they were cut. The same guard applies to **connections/"connecting routes", wild Pokémon/encounters, area state, and the tileset deep pass**: all un-briefed, all listed in `notes/plans/map-screen.md` → **§12b "NOT YET BRIEFED"**. Sketches written from the *save layout* are a map of what bytes exist, not of what she wants a person to be able to *do* — they carry **no authority**. When a briefed feature genuinely needs an un-briefed one, it **reads** it; it does not build a UI for it. If in doubt, **ask before building, not after**.
- **A derived byte is kept IN SYNC by default; power users can break sync (clarified 2026-07-15, Twilight).** When the save holds a value the game *computes* from another (the map view pointer from the player's coords; the tileset pointers; the music bank), the editor **keeps it correct automatically by default** — most people editing that area want that, and it is bad UX to let a novice break their map by not hand-editing a derived field they had no reason to touch. But **raw-byte editing is always available**, and a derived value **may be deliberately desynced** — via a *break-sync* toggle, by entering a different value (which raises an **alert offering to break sync**), or (the view box) by **dragging it on the canvas**. Byte fidelity still holds — an edit writes only the bytes its action implies — so this is the *opposite* of silent corruption, not an exception to it. See `notes/plans/map-screen.md` → "The doctrine".

## Build System

> **YOU CAN ACTUALLY BUILD/TEST/RUN/GIT — you are NOT limited to a sandbox.** The PowerShell
> terminal tool has real access to the local Windows machine, where the full Qt 6.11 llvm-mingw
> toolchain is installed. From it you can directly **configure, build, run the tests, launch the
> app, and `git add`/`commit`/`push`/fast-forward `main`** — by default, without asking. Prior Qt
> command history from other chats and from Qt Creator is available to crib exact invocations from.
> **Do NOT open a session claiming "I can't build, no Qt tools in the sandbox"** — that's wrong and
> do not repeat this misconception. (The Cowork *bash* sandbox is a separate, weaker tool with
> stale-read issues — use **PowerShell** for anything real here, not bash.) The exact commands,
> paths, and gotchas are below and in the Default Workflow section.

Toolchain (Qt Creator kit `Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug`), all on the local
Windows machine via the PowerShell terminal:

- Compiler/runtime: `C:\Qt\Tools\llvm-mingw1706_64\bin` (clang++, llvm-cov, llvm-profdata, llvm-nm)
- Qt: `C:\Qt\6.11.0\llvm-mingw_64`; cmake `C:\Qt\Tools\CMake_64\bin\cmake.exe`; Ninja generator
- **Two build dirs — do not mix them up:**
  - `build/` (repo root, Ninja) — the **test** build the automated loop uses (`cmake -S projects -B build`).
  - `projects/build/Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug/` (Makefiles) — the **app** kit dir
    actually runs. Siblings: `asan/`, `coverage/`. **Rebuild THIS for in-app testing**, not just `build/`.
  - `build-cov/` — coverage build (`-fprofile-instr-generate -fcoverage-mapping`, `PSE_SHARED_APPCORE=ON`).
- Every PowerShell call must prepend PATH with the llvm-mingw + Qt `bin` dirs and set `$env:CC=clang;
  $env:CXX=clang++`, or clang++ isn't found. The PowerShell transport caps ~60s — **run long builds
  detached** (`Start-Process` writing a log) and poll the log. **Always redirect stdout AND stderr to a
  log file** so the output is readable (`… > build.log 2>&1`). Before running any test/app exe, set
  crash-fast error mode (`SetErrorMode(0x0003)` via a P/Invoke `Add-Type`) so a crash fails fast instead
  of hanging on the qtcdebugger dialog.
- `PokeredSaveEditor.exe` links `savefile.dll` via its import lib, so editing a `savefile` `.cpp`
  rebuilds the **DLL** but does NOT relink the exe (exe mtime stays put) — fine, it loads the new DLL at
  runtime; verify by the **DLL** timestamp, not the exe.
- **Linux build/test via Docker (`docker/`).** A containerized Linux toolchain (Qt 6.11 + clang, baked
  once) runs the full suite four ways: `.\docker\dtest.ps1 [standard|asan|xvfb|coverage|all]`. This is
  where **ASan/UBSan actually work** (broken on the llvm-mingw kit) and where llvm-cov coverage runs on
  Linux. It rsyncs the repo into a persistent ext4 volume (fast, ccache-cached) rather than building over
  the WSL bind mount. First run (2026-06-13): all four green (66/66; ASan clean; 89.73% line cov). See
  `notes/plans/testing.md` → "Local Linux container (Docker)" and `docker/README.md`.

## Default Workflow — Do These By Default (a standing instruction)

After making changes, run this loop **without being asked** (established 2026-06-10). Route all build/test
output to logs (`> log 2>&1`) so it's readable; builds run detached + polled (PowerShell ~60s cap).

> **EVERYTHING RUNS IN THE BACKGROUND — mandatory, by default (2026-07-12, Twilight).** Builds, tests,
> screenshot captures, emulator runs, git, installs: all of it runs **hidden/headless** (`Start-Process
> -WindowStyle Hidden`, `QT_QPA_PLATFORM=offscreen`, detached + polled via a log). **Nothing steals her
> screen or her focus** *while you are working*. A window appearing unbidden mid-work is an interruption,
> and interruptions are not free. If you can capture it headless and *show her the image instead*, do that.
>
> **THE OTHER HALF (2026-07-12, Twilight): when it's ready for her to LOOK at it, OPEN IT — in the
> foreground, already on the right screen, without being asked.** Don't finish by saying "it's ready for
> your live pass" and leave her to launch the app and navigate to the feature herself. **Take the
> opportunity the moment it presents itself:** launch it in front of her, with the save loaded, landed
> directly on the screen under review —
> `PokeredSaveEditor.exe --sav assets\saves\natural-clean\BaseSAV.sav --screen <name> [--select ...]`
> (see `notes/reference/dev-harness.md`). Background while building; **foreground the second it's worth
> her time.**

0. **Track the work with TASKS — early, often, and comprehensively (by default, 2026-07-11, Twilight).**
   Open a task list at the *start* of anything with more than one step — not retroactively, not "if it
   gets complicated". Break the work down properly (a real breakdown, not three vague buckets), keep
   statuses live as you go (`in_progress` when you start it, `completed` the moment it's done), and add
   new tasks the instant new work surfaces mid-flight. **Whenever you learn something worth keeping —
   a finding, a gotcha, a decision, a constraint — offer to record it on a task by default** (and write
   it into the task's description), so nothing learned evaporates between steps. The task list is a
   working artifact Twilight watches live, not a formality. (This does NOT replace the `notes/` — durable
   knowledge still lands in the notes; tasks carry the in-flight state and the trail that gets it there.)
1. **Build + launch (on any C++/qrc change).** Rebuild the **kit dir**
   (`cmake --build "projects\build\Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug" --target PokeredSaveEditor`)
   and **launch the app** so it can be tested in-app immediately. (Pure edits to existing QML hot-reload —
   no rebuild; **new** QML files still need adding to `app/app.qrc` + a rebuild.)
   - **Fast-dev loop for UI work (by default, 2026-07-10).** Use the DEBUG automation harness instead of
     the rebuild-relaunch cycle: launch **once** straight to the screen under edit with the default save
     loaded and hot-reload on — `PokeredSaveEditor.exe --hot --sav assets\saves\natural-clean\BaseSAV.sav
     --screen <name> [--select party:N]` — then edit QML and let it **hot-reload live**; only rebuild +
     relaunch when C++ / `.qrc` / a **new** `.qml` changes. Drive/inspect the running app over the live
     TCP channel `127.0.0.1:8766` (`screen`/`sav`/`get`/`set`/`click`/`shot`/`reload`/`list`), and grab
     screenshots with `--shot`/`shot` (framebuffer grab → works while backgrounded, no focus stealing).
     All DEBUG-only (release builds don't contain it). Full reference: `notes/reference/dev-harness.md`.
   - **Manual screenshot review is MANDATORY on ANY UI / QML / screen / layout change — by default,
     every time (established 2026-07-10, after a missed overlap).** Capture the affected screen(s) with
     the headless `screenshooter` (`scripts/capture_screenshots.ps1` → `tmp/screenshots/`) and then
     **actually look at the image and scrutinise it yourself** — crop/zoom into the changed area. Check
     for **overlaps** (e.g. a field overlapping the trainer art), misalignment, uneven spacing, anything
     **clipping past a card/panel border**, whether groupings read cleanly, and overall polish. A glance
     is not a review. Assume "there's a lot to pay attention to" and proactively fix layout problems
     (overlaps, cramping, clipping) **even when not explicitly asked**. Do not call UI work done, and do
     not tell Twilight it's ready, until this pass is complete. (The trainer-card clock overlapped the
     artwork and shipped because this step was skipped — that must not recur.) See
     `notes/reference/screenshots.md` and `notes/reference/ui-patterns.md`.
2. **Test.** Run the **affected** test(s) per change for speed (build `build/`, run `build\tst_x.exe`);
   run the **full `ctest`** suite before releasing `dev → main`. Only proceed past a **green** result.
   **On ANY QML/screen change (or a new `.qml` added to `app.qrc`), the QML screen smoke test
   `tst_qml_screens` MUST be green before the release merge to `main`.** It loads every registered screen through the
   real engine and fails on any QML warning/error (FINAL overrides → "Component is not ready", binding
   `TypeError`s, missing types/providers, anchor-on-null). The C++ ctest suite never instantiates QML,
   so it cannot catch this class — this is what let a non-opening Credits screen reach `main` on
   2026-06-13. `tst_qml_screens` is registered with CTest (and `tests_all`), so a full `ctest` run
   already includes it; for a fast QML-only check, build + run just `build\tst_qml_screens.exe` with
   `QT_QPA_PLATFORM=offscreen`. Details: `notes/plans/testing.md` → "QML screen smoke test".
3. **Debug / profile.** If anything **crashes**, rebuild via the **`asan/`** (or debugger) sibling build,
   capture a **real stack trace** (output routed to a log), and diagnose from that — never guess.
   Do **periodic profiling** passes when touching hot paths. Always redirect std+err to a log to read it.
4. **Commit + push often on `dev`; release `main` ONLY on an explicit "ship it" (manual releases).**
   Releases are **manual** as of 2026-07-10 (Twilight): commit early/often and `git push origin dev`
   after each commit **by default**, but **NEVER** cut a release (merge/FF `dev → main`) on your own —
   even when everything is green. **Wait for Twilight to say "ship", "ship it", or similar**; that word
   is the trigger. Green is necessary but no longer sufficient. When work is done, finish on `dev`,
   verify (build/test/CI), and **tell Twilight it's ready to ship — then stop and leave `main` alone.**
   (This supersedes the earlier "fully automatic green-gated release" default.) When Twilight *does* say
   ship, release the git-flow way: `main` advances only by `--no-ff` tagged-release merges (PATCH
   straight from `dev`; MINOR/MAJOR via a `release/*` branch). See `notes/reference/git-workflow.md`:
   - **Changelog rides inside the commit (write it BEFORE committing).** For any substantive change,
     write its plain-English entry at the top of the current month's file in `notes/version/` and stage
     it in the **same commit** as the change — one commit carries both. Inline entries take **no
     `<!-- commit: hash -->` marker and no short-hash byline** (a commit can't hold its own hash; `git
     blame` the entry line to find its commit). **Never** make a separate "document the last commit"
     commit, and **never** give a changelog/notes-only maintenance commit its own entry — that recursion
     (commit → entry → commit → …) is exactly what this rule prevents. See `notes/version.md` →
     "How this is kept updated (the inline rule)".
   - **Keep `VERSION` current — bump it inside the same commit when a change warrants it.** **PATCH**
     for a bug fix / small change, **MINOR** for a feature / notable change (you decide). **NEVER bump
     MAJOR** (`→ 1.0.0`) — that's the project leaders' (Twilight's) call only. Docs / notes / test /
     CI-only commits don't move the number. See `notes/reference/versioning.md`.
   - Commit early/often on **`dev`** with focused `type: summary` messages, **staging specific files only**
     (never `git add -A`/`.`), and `git push origin dev` after each commit.
   - When the **full suite is green AND Twilight has said "ship it"**, release `dev → main` the git-flow
     way (a **PATCH** goes direct; a **MINOR/MAJOR** milestone goes through a `release/X.Y.0` branch —
     see git-workflow.md). Without an explicit ship command, do **not** run this even if green:
     `git checkout main && git merge --no-ff dev && git push origin main && git checkout dev`.
     The `--no-ff` merge commit is the release; **do not manually `git tag`** — `release.yml`
     creates the `v<VERSION>` tag and publishes (the recorded CI-owns-tagging divergence). A
     hand-pushed tag would make the release run skip itself.
   - **"Green" now includes the GitHub Actions CI, not just local `ctest` (standing request).** Before
     fast-forwarding `main`, confirm the remote **`tests`** workflow passed on the `dev` HEAD being
     merged — `gh run list --branch dev -L 1` / `gh run view <id>` (the GitHub CLI is installed +
     authed). If that CI run is still in progress, wait for it; if it failed, treat it exactly like a
     local red and do **not** FF. Local `ctest` green is necessary but no longer sufficient.
   - **After releasing to `main`, watch the `release` run** — pushing `main` triggers `release.yml`,
     which (when `VERSION` was bumped → tag `v<VERSION>` is new) builds + publishes the GitHub Release. Monitor it
     with `gh run watch` / `gh run view --log-failed`; a failed build leaves NO tag/release, so fix
     forward and the next `main` push retries the same version. See `notes/reference/deployment.md`.
   - **After releasing to `main`, rebuild the Doxygen docs by default** — `doxygen Doxyfile` from the
     repo root — so the generated `docs/html/` (git-ignored) always tracks `main`. See
     `notes/reference/documentation.md`.
   - **Also after releasing to `main`, refresh the UI screenshots by default** —
     `pwsh -File scripts/capture_screenshots.ps1` (Linux/CI: `scripts/capture_screenshots.sh`) — so the
     `tmp/screenshots/` **still PNGs** (git-ignored; never committed) always track `main`. It builds +
     runs the headless `screenshooter` tool (renders only, never writes a save byte). No automated GIFs
     — animated GIFs are added manually, one at a time. See `notes/reference/screenshots.md`.
   - **Hard safety rules still absolute:** never `push --force`/force-with-lease, never rewrite pushed
     history, never `reset --hard`/`rebase`/`clean -fd`/delete a branch without an explicit request.
     Inspect `git status` before and after, every time. Full standards: `notes/reference/git-workflow.md`.

## GitHub Is Part of Default Management (a standing instruction)

The GitHub CLI (`gh`) is installed + authenticated (account `junebug12851`), so GitHub state is part of
the normal workflow — not something to wait to be asked about. The cadence is **event-based, not a
calendar** (Twilight's call): the trigger is **preparing `main` for shipment**, not a timed ping.

- **Whenever prepping `main` for shipment** (i.e. about to release `dev → main`), do a quick GitHub check as part of
  the same step: `gh run list` (CI/release health — must be green; see Default Workflow step 4), plus
  `gh issue list` and `gh pr list`. If there are **open/new/changed issues or PRs**, surface them to
  Twilight as a short summary and **ask whether to work on them now or later** — don't silently start.
- **Non-trivial issues / PR reviews are usually their own chat.** Offer to spin one up rather than
  derailing the shipment; a quick "these are open — now or later?" is the default, not diving in.
- **No timed/scheduled pings** unless Twilight later asks for one — the check rides on the
  shipment-prep event. (If wanted, a scheduled digest can be added with the scheduled-tasks tools.)
- **Never auto-act on issues/PRs** (no closing issues, merging PRs, or pushing to PR branches) without an
  explicit go-ahead — surfacing + asking is the default, acting is opt-in. Hard git safety rules apply.

### Release & publishing policy (standing rules)

- **GitHub Releases are for SOFTWARE releases ONLY — never an images-only or otherwise non-software
  release.** The versioned `release.yml` release (Windows installer/portable, Linux AppImage/tar.gz, docs
  zip, screenshots zip) is the only kind. Don't create side releases just to host files.
- **Every Release gets a well-written title + description by default** — clear, concise, informative,
  structured/organized (a downloads table, the prerelease/unsigned note, etc.). `release.yml` composes
  this automatically (`Compose release notes` step → `body_path`) with the auto "What's Changed" appended;
  keep that quality bar if you touch it.
- **The GitHub Pages site (docs + screenshots) is deployed by `pages.yml`, not git or a release.** On
  every `main` push it builds the Doxygen docs + captures the screenshots and deploys one Pages site:
  the **Doxygen home is the root** (`…github.io/pokered-save-editor-2/`) with **Screenshots + GitHub**
  custom nav tabs (injected via a generated `DoxygenLayout.xml`, README untouched), and the images live
  at `…/screenshots/<name>` (no `frames/`) — zero repo-size growth, no third-party host. README + docs
  embed the absolute `https://junebug12851.github.io/pokered-save-editor-2/screenshots/<name>` URLs. See
  `notes/reference/deployment.md`.

## Maintaining the Notes — Your Responsibility

**The notes system is a living document. Keep it updated as you work — do not wait to be asked.**

**These `notes/` are THE memory of this project — use them by default: read them at the start of every
session and write to them regularly as you work.** Any standing instruction, preference, decision, or
piece of feedback from Twilight goes **into the right notes file** (and, when it's a workflow rule, into
this `CLAUDE.md`) — that is the single source of truth. **Do NOT stash project knowledge in any
external/personal "memory" instead of the notes** (established 2026-07-10, Twilight); default to the
notes, not a side channel. If a fact doesn't fit an existing file, create the right one (see below).

As things happen during a session, update the appropriate file on the spot:

| Trigger | Action |
|---------|--------|
| Did work worth recording this session | Append an entry to today's `notes/sessions/YYYY-MM/YYYY-MM-DD.md` (create the file — and the `YYYY-MM/` month folder if it's a new month — if it's the first entry today; newest on top). **If you CREATE a new day file, add its `\subpage` to `notes/_nav.dox`** (newest-first under the month hub; a new month → add a month-hub page too) or it floats to the Doxygen root. See `notes/sessions/README.md` |
| Fixed a compiler or runtime error | Add a row to `notes/reference/fix-patterns.md` |
| Hit a Qt 5 → Qt 6 difference or any Qt/QML landmine | Add a section/row to `notes/reference/qt-patterns.md` |
| Used a diagnostic technique to find a problem | Add/update `notes/reference/diagnostic-methods.md` |
| **Learned ANYTHING new about the game, the save format, or the hardware** | **Write it into the right `notes/reference/` file — create one if the topic is new. Not optional, not deferred.** See "RESEARCH LANDS IN THE NOTES" above |
| **Verified something against the real cartridge** | Commit the probe under `scripts/emu/` and quote the console's own output in the note. A claim the console checked is worth ten that it didn't |
| **Found that a save field's name/behaviour in our model is WRONG** | Record it in the reference note **and** open a phase to fix the model *before* any UI is built on it (the sprite/tileset/warp precedent) |
| Made a structural decision | Add to `notes/decisions/architecture.md` |
| Tried something that failed | Add to `notes/decisions/rejected.md` |
| Completed a task or unblocked something | Update `notes/plans/next-steps.md` |
| Build health changes | Update `notes/status.md` |
| Made any substantive commit | Write its changelog entry inline in `notes/version/YYYY-MM.md` and stage it in the **same** commit (no marker; never a separate doc commit). See Default Workflow step 4 + `notes/version.md` |
| Something significant about the project's history changes | Update `notes/context/history.md` |
| A new contributor/tool/service/asset/AI helps the project | Add them to `projects/db/assets/data/credits.json` (see "Keep the Credits Screen Living" below) |
| A change warrants a new version (keep `VERSION` current by default) | Bump the one line in repo-root `VERSION` in the **same commit** — **PATCH** for a fix/small change, **MINOR** for a feature; **never MAJOR** (leaders-only). Reconfigure to apply; tag `vX.Y.Z` on release (on request). Never hardcode a version. See `notes/reference/versioning.md` |
| Created ANY new Markdown note in the Doxyfile `INPUT` (a session day, a new month folder, a `reference/`/`decisions/`/etc. page, a new changelog month) | Add its `\subpage` to `notes/_nav.dox` under the right hub, **same commit** — a page with no entry floats flat to the top of "Related Pages" on the Doxygen/Pages site instead of nesting. New month/folder → add its hub page too. (This bites easily — today's session + new reference docs all floated until fixed, 2026-06-15.) See the hard rule atop `notes/_nav.dox` |

Also: if something comes up that doesn't fit any existing file, create a new file in the right folder.
The structure is meant to grow. Don't stuff things into the wrong place to avoid creating a new file.

The goal is that any AI opening this project cold can read the notes and be fully oriented —
with no information trapped in a human's head or lost between sessions.

## Keep the Credits Screen Living

> **ENFORCED, BY DEFAULT — check credits on every substantive change (re-stated 2026-07-11 by
> Twilight, who is the first to admit she forgets them).** Credits are not a release-time chore you
> remember if you happen to think of it: **before you commit**, ask "did this change bring in anyone
> or anything new?" — a new data source, library, tool, service, asset/icon/artwork, or AI helper —
> and if so, add it to `credits.json` **in that same commit** (and regenerate `credits.md`). Treat a
> missing credit exactly like a missing test: the work is not done. This is part of the Default
> Workflow, not an optional extra.

The in-app **Credits / About** screen is a living document — keep it current **by default,
without being asked** (a standing instruction). Whenever someone or something new
contributes — a person, framework, tool, service, icon/asset source, or an AI assistant
(e.g. Claude, ChatGPT) — add them to `projects/db/assets/data/credits.json` under the right
section. Sections are read, in display order, by `CreditDBEntry::process()`
(`projects/db/src/pse-db/entries/creditdbentry.cpp`): **Project Leaders, Data Sources,
Framework, AI Assistance, Tools Used, Services Used, Icons, Wallpapers**. Entry fields:
`name`, `url`, `note`, `license`, `mandated` (all optional except a name). Adding a brand-new
**section** also requires a matching read in `process()`. The JSON is baked into `db.qrc`, so
**any credits change needs a rebuild** to show in-app (editing existing entries = rebuild;
new section = rebuild + the C++ read). No hardcoded credit counts exist in the tests, so
adding entries won't break them.

**Also regenerate `projects/db/assets/data/credits.md` whenever you edit `credits.json`** — it's the
human-readable Markdown rendering of the same data (linked from the root `README.md` and built into
the Doxygen docs under "Project & Repository"). Keep the two in sync: edit the JSON, then rewrite the
`.md` from it (same sections/entries, each `name` linked to its `url`, with note/license/mandated
shown). The JSON is the source of truth; the `.md` is a generated view.

## Cross-project standards & checking the fairyfox system for updates

This project is a **node in the fairyfox system** (the hub mesh at fairyfox.io): it
pulls shared standards from the system on request and keeps its own committed copies
under `notes/reference/`. The model is in
[`notes/reference/cross-project-sync.md`](notes/reference/cross-project-sync.md):
communication is git-only, one-directional per flow, read-only on the far side, and
happens **only on explicit request** (so the repos can never set each other off in a
loop).

**When Twilight asks you to check *the fairyfox system* for updates** — to sync the
standards, get the latest version, or pull a particular standard/runbook — treat it as
the check-for-updates flow. **To invoke it the request must carry the word "fairyfox"**
— normally **"the fairyfox system"**, or a *fairyfox*-prefixed variant ("fairyfox.io",
"fairyfox standards") — *paired with* an update/sync intent (check for updates · what
changed · sync · refresh · pull the latest · get the newest). Generic handles — "the
hub", "the mesh", "the standards", a runbook name, a bare "system", or an update verb
alone — do **not** qualify; the word *fairyfox* must be present, or don't assume this
flow.

The default is **check, report, then wait**: refresh the read-only system clone under
`assets/references/fairyfox.io/` (git-ignored), diff it against what this project has
adopted, and **report what changed + what adopting it would touch — then stop.** Apply
nothing until Twilight clearly says go ahead; applying is a separate, confirmed act.
Full procedure: the shared `adopting-updates` runbook (in the hub's `hub/standards/`).

**Exception — pre-authorized changes.** The system keeps an express-authorization
ledger (`assets/references/fairyfox.io/hub/authorizations.yml`), read out of the same
read-only clone. If an active entry there `covers` the change being adopted, Twilight
**already gave the go-ahead at the system** — apply it directly, skipping the "wait"
pause. Skip *only* that redundant pause: still reconcile (don't clobber a deliberate
local divergence — re-prompt if you would), still write the process report, still
commit as a reviewable act. Nothing in the ledger covers it → fall back to
check-report-wait. (Reading the ledger is read-only and on-request — it lets a node
skip a prompt, never lets the system act on this repo, so anti-recursion holds.)

**Pre-authorization skips the confirmation pause — it never skips safety.** Whenever
a change is applied automatically or under an express-authorization (and *especially*
then), every safety net still runs, comprehensively and at every level: reconcile
without clobbering local divergence (re-prompt if unsure), run the relevant
build/tests, run the standards **compliance / `## Verify`** checks, and confirm the
change stays within all project constraints (save-file fidelity, no hacks, UX bar,
git safety rules) before and after. Checks, tests, and compliance are run regularly
and thoroughly throughout — bypassing the *pause* must never become bypassing the
*verification*. If full verification can't be completed, do not auto-apply; fall back
to check-report-wait.

**Guardrails (don't break these):** on-request only — never auto-pull or schedule
cross-repo syncs (anti-recursion); the reference clone is read-only and git-ignored
(the authorization ledger included — reading it lets you skip a prompt, it never lets
the system act on this repo); never apply changes or rewrite history without an
explicit go-ahead (an active `authorizations.yml` entry that covers the change *is*
that go-ahead, given at the system); reconcile with local edits, don't clobber them.

> Naming: Twilight calls it **the fairyfox system** in conversation; the public website
> calls it the **hub**. Both name the same fairyfox.io mesh.

## Project Preferences

- UI/UX decisions are a design decision — do not independently change QML appearance
- Debug builds show error dialogs; release builds degrade gracefully and clearly (never silently swallow errors, and never at the cost of save data)
- The app should feel like polished software, not a dev tool
- **This is a heavily THEMED app — design for it.** Nothing may look like an old-fashioned desktop tool: no menu bars, no grey 3-D bevels, no classic toolbars-with-separators, no tiny system icons. The app's language is flat, rounded, chip-and-segment based (`SegSel`/`SegBtn`/`FlatToggle`/`IconButtonSquare`, accent bars, titled group boxes). Borrow the **ergonomics** of professional editors, never their chrome.
- **Clutter is a bug.** Anything not in use collapses away: docks collapse to an icon rail, groups collapse to a title, advanced fields collapse behind a disclosure. Panels do not stack out beside each other, and controls never wrap into a second row of chrome. If a screen is running out of room, the layout is wrong — do not shrink the content to make room for the furniture.
- **Every value the save holds is editable, including the hack/glitch ones** — full byte range, flagged in words when it's a value no real game would hold, and never refused or rewritten behind the user's back.

