# Music — the plan

_Making the editor **play the game's music**, accurately, and putting the music controls where they
belong: on the **Map screen**._

Background you must read first:

- [`../reference/gameboy-apu.md`](../reference/gameboy-apu.md) — the sound chip.
- [`../reference/gen1-sound-engine.md`](../reference/gen1-sound-engine.md) — the game's sequencer, the
  data formats, and the **two save flags** (verified).
- [`../reference/emulator-verification.md`](../reference/emulator-verification.md) — the oracle.
- [`../reference/ui-patterns.md`](../reference/ui-patterns.md) — before any UI work.

---

## 1. What we're building

1. **The two flags, exposed.** `No Audio Fadeout` and `Prevent Music Change` — checkboxes, on the Map
   screen. (v1 had them; v2 reads/writes them already but shows neither.)
2. **The map's music, editable.** The track picker (id + bank), on the Map screen, next to the flags.
3. **The music, actually playing.** A **play button next to each track**, and — the thing Twilight
   actually asked for — **hover-to-preview**: with playback on, running the mouse down the track list
   switches the music as you go, so you can skim the soundtrack.
4. **Completely accurate.** Not "sounds like Pokémon". The real sequencer, driving a real APU model,
   proved right against the real cartridge.

**Where it lives: the Map screen.** This is another organ of the map emulator we are slowly growing —
the same screen that already draws the map, the border ring, the palettes and the player. Music is part
of what a map *is* in the save; it does not get its own screen. (This supersedes v1's separate
"Area → Audio" card.)

## 2. The decisions (taken 2026-07-12, with Twilight)

| Decision | Choice | Why |
|---|---|---|
| **Where the music data comes from** | **Import from `pret/pokered`** into our own data files | Exactly the precedent already set by the map `.blk`/`.bst` blocks: imported verbatim, self-validating script, committed. Music then works for **everyone**, with no ROM. ~46 tracks of sequencer bytes (tens of KB) — *data*, not audio. |
| **How faithful** | **Full port: the Gen 1 sound engine + a DMG APU emulator, in C++** | Anything less is an impression. This is also the only version that is *verifiable*, and it gives us SFX/cries later for free. |
| **What we build first** | **Research + notes + plan** (this document), then stop | The build is long; the plan gets read before a line of it is written. |

**The ROM stays out of it.** `assets/references/backup.gb` remains Twilight's own cartridge backup:
git-ignored, never committed, never shipped, tests SKIP without it. It is used **only** as the *oracle*
in tests — never as a runtime data source.

## 3. Architecture

A new library, **`pse-audio`**, sitting beside `common` / `db` / `savefile` / `app`. It has **no Qt-QML
dependency** and knows nothing about save files — it is a Game Boy that only does sound.

```
   MusicDB (db)            ── track name → { id, bank }            [exists already]
   MusicDataDB (db)        ── track id+bank → the command bytes    [NEW, imported]
        │
        ▼
   pse-audio ───────────────────────────────────────────────────────────┐
        │                                                               │
        │  Gen1SoundEngine    the transliterated engine_1/_2 (§5–§7 of  │
        │    ├── 243-byte state, exactly the $C000 WRAM block           │
        │    ├── updateFrame()  ← called ~59.7×/s                       │
        │    └── emits register writes ──────────┐                      │
        │                                        ▼                      │
        │  GbApu              4 channels + frame sequencer + mixer      │
        │    ├── write(reg, val)                                        │
        │    ├── tick(cycles)                                           │
        │    └── render() → 48 kHz stereo PCM                           │
        └───────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
   app:  MusicPlayer (QObject)  → QAudioSink on its own thread, ring buffer
                                → brg.music  (QML)
                                    │
                                    ▼
   qml:  Map.qml → the audio panel: flags, track picker, ▶ per track, hover-preview
```

**Rules of the design**

- The engine emits **register writes**, nothing else. It must not know what a speaker is.
- The APU knows **nothing** about notes, tracks, or Pokémon.
- The player thread owns timing; the UI thread never blocks on audio.
- **Transliterate, don't interpret.** The engine port copies the disassembly's arithmetic — including its
  bugs (see the reference §7). Every "that can't be right" in that file is load-bearing.

## 4. The phases

Each phase is independently shippable and independently *green*.

### Phase 1 — the save half (small, immediate value, no sound)

- Expose **No Audio Fadeout** + **Prevent Music Change** on the **Map screen**.
- Expose the **music track picker** (through `MusicDB`, showing real names, not `02_BA`).
- **The bank guard** (see Phase 7): only 2 / 8 / 31 are offerable; a save holding any other bank is shown
  as-is, never rewritten, and warned about — it hangs the real game.
- 🐞 **Fix `AreaAudio::setTo()`** — it currently does `musicBank = musicID = musicEntry->bank;`, clobbering
  the id with the bank. Pin it with a test.
- Tests: `tst_area` extensions (round-trip the two bits and the two bytes; `setTo()` correctness);
  `tst_qml_screens` must stay green.
- **Mandatory screenshot review** of the Map screen after the layout change.

### Phase 2 — the data import

- `scripts/import_music.ps1`, modelled on `import_map_blocks.ps1`: reads `pret/pokered`'s
  `audio/headers/*.asm` + `audio/music/*.asm` + `audio/sfx/noise_instrument*.asm` +
  `audio/wave_samples.asm`, resolves labels, and emits our own compact format
  (`db/assets/data/music/*.bin` + an index) — **self-validating, with a `-Check` mode**.
- The assembler-level truth check: the imported bytes must **match the bytes in the ROM** at the address
  the header points to (uses the local `backup.gb`, dev-only; skips without it).
- Credits: add/confirm **pret/pokered** as a data source in `credits.json` (+ regenerate `credits.md`).

### Phase 3 — the APU (`pse-audio`, part 1)

- `GbApu`: channels 1–4, frame sequencer, envelopes, sweep, LFSR, wave RAM, NR50/NR51 mixing, DAC-off,
  proper downsample to 48 kHz.
- Tests: register-level unit tests + golden PCM snapshots for hand-written register sequences.

### Phase 4 — the engine (`pse-audio`, part 2) — **the hard one**

- `Gen1SoundEngine`: the literal port. State = the 243-byte block. One `updateFrame()`.
- **`tst_sound_parity`** — the whole point: PyBoy boots the real ROM, plays track *N*, and dumps
  `$C000–$C0FF` **every frame** for *K* frames; our engine runs the same track for *K* frames; **demand a
  byte-for-byte match, frame by frame, for all 46 tracks.** This is the `viewPointer_matchesWhatTheGameStored`
  of the audio work — if it goes red, nothing downstream can be trusted.
- A second, weaker gate: compare the **APU register writes** per frame.

### Phase 5 — playback + the UI

- `MusicPlayer` on `brg.music`: `play(id, bank)`, `preview(id, bank)`, `stop()`, `volume`, `isPlaying`,
  `previewing`, `currentTrack`. `QAudioSink`, audio thread, ring buffer, no UI-thread jank.
- The Map screen's **Music panel** — full spec in §6.
- Audio **never starts by itself** and never plays on app launch. Nothing steals her attention — the same
  rule as windows. (See `principles.md` → "What the App Should Feel Like".)
- Screenshot review + in-app review with Twilight.

### Phase 6 — the free stuff (later, optional)

SFX, the 19-piece drum kit, the Poké Flute, the low-health alarm, and **all 151 cries** (base cry +
per-species pitch/length, already in our Pokémon data). Nearly free once Phase 4 is real. **Not in scope
until asked.**

### Phase 7 — the glitch tracks + sheet music (the perk)

Researched 2026-07-12 and **verified on the cartridge** —
[`../reference/glitch-music.md`](../reference/glitch-music.md). The headline:

- **Every id in the music region is a real track or one of its INNER VOICES.** A header is 3 bytes per
  channel, so a 3-channel song eats 3 ids — and the extra ids are that song's channel 2 / channel 3,
  read as one-channel headers. **Id 187 is Pallet Town's bassline, alone.** There are **105 of them.**
- They need **no extra data**: every inner voice points *into* a stream we already import. So the shipped
  app plays **46 real tracks + 105 inner voices = 151 pieces of audio, for the price of 46.**
- Because we run the *engine*, every note is known exactly (pitch, octave, length, duty, volume, tempo,
  ornaments) — no pitch detection. A **`Transcriber`** sink on the engine can export any of the 151 as
  **MIDI** and as real notation (**MusicXML / LilyPond**), following a track to its end or its loop point
  (the analyser already finds loop points: `sound_loop` count 0 = the repeat mark).

Work: (a) surface the inner voices in the track list (a disclosure under each track: *"channel 2 · 59
notes · loops"*), (b) `Transcriber` + exporters, (c) a score view, eventually.

⚠️ **And the guard rail this research forces into Phase 1:** the **bank** byte is not a glitch, it is a
loaded gun. An invalid bank makes the game execute arbitrary cartridge bytes as code, every frame —
**verified: the console stops producing frames the instant the map loads.** The bank picker offers only
**2 / 8 / 31**; a save that already holds something else is **shown, never silently rewritten**, with a
plain warning that it hangs the real game.

## 5. Risks, and what we do about them

| Risk | Mitigation |
|------|-----------|
| **The port is subtly wrong and it "sounds fine"** — the classic chiptune trap | The frame-by-frame `$C000` parity test. Ears are not evidence; the console is. |
| Qt Multimedia latency/underruns making hover-preview feel bad | Generous ring buffer; engine runs ahead; preview switches on the next engine frame, not the next sample. Measure before tuning. |
| Shipping the game's music data | Twilight's call, taken above; it is the same category as the map blocks we already ship, imported from the same public disassembly. Credited. If that ever needs to change, the import script is the single place to change it. |
| Scope creep into SFX/cries | Phase 6 is explicitly fenced off. |
| APU edge cases (zombie envelope, wave-RAM quirks) | Not implemented until the parity/PCM evidence says they're needed. Record the decision here if they are. |

## 6. The UI — decided (2026-07-12, with Twilight)

Twilight: *"figure out a place — I'm going to redo the UI/UX later anyway. Hover preview: nice. The map's
current music should be selected by default."*

So placement is **mine and provisional**, which sets one hard constraint: **the whole thing must be one
self-contained file** (`ui/app/screens/non-modal/map/MusicPanel.qml`, driven by `brg.music` + `brg.map`) so a
future redesign moves *one* component, not a dozen bindings scattered through `Map.qml`.

### 6.1 Where it goes

The Map screen today is: a **header info strip** (map name / tileset / size / player / contrast), the **map**
filling everything, and a **footer** (legend + zoom).

The music lives in a **collapsible right-hand panel**, because a 46-track list with hover-preview needs
*vertical* space and the map is the star and must not shrink permanently.

- A **♪ Music** toggle button joins the header strip (next to Contrast).
- Toggling slides the panel in on the right (~260 px). The map keeps its centre; zoom is untouched.
- The panel is **closed by default**, and closing it **stops playback** (no invisible audio).

### 6.2 What's in it, top to bottom

1. **Now playing** — the track name, big; and, when auditioning, a quiet second line saying *"previewing —
   the save still holds **X**"*. This line is the whole safety of hover-preview: it is always obvious when
   what you're hearing is not what's saved.
2. **Transport** — ▶ / ■ and a volume slider. Playing starts on the **selected** track (see 6.4).
3. **The two flags** — `No Audio Fadeout` and `Prevent Music Change`, as checkboxes, each with a tooltip that
   says what it *actually does* (per [`../reference/gen1-sound-engine.md`](../reference/gen1-sound-engine.md)
   §9), not what its name implies:
   - *No Audio Fadeout* → "The game normally forces the volume back to full every frame. With this set, it
     leaves the volume alone."
   - *Prevent Music Change* → "Entering a map won't start that map's music — whatever is playing keeps
     playing."
4. **The track list** — all 46, scrollable, **grouped** (Towns & Cities · Routes · Places · Battle ·
   Encounters · Special), **game order within each group** (the `music.json` id order). Grouping is what
   makes a hover-sweep worth doing; a flat 46-row list is just a wall.
   Each row: the track name, a dim `bank·id`, a ▶ that appears on hover, and a **check on the selected row**.
5. **The odd one out** — if the save holds an id/bank that isn't in `music.json` (a glitch value, or `0`),
   it gets its **own pinned row at the top**: *"Unknown — id $XX, bank $YY"*, selected. We never silently
   round a strange save to a nice-looking one. (It is un-previewable; the ▶ is disabled with a tooltip
   saying why.)

### 6.3 Hover-preview — the exact behaviour

The rule: **hover auditions, click commits.** The save is never touched by moving a mouse.

| | |
|---|---|
| **Armed only while playing** | If nothing is playing, hovering does **nothing**. Audio never begins because a cursor drifted somewhere. ▶ is always a deliberate act. |
| **Settle, don't sweep** | A row must be hovered for **~120 ms** before it plays. Dragging the cursor down the list at speed doesn't machine-gun the engine — only the row you *settle* on sounds. |
| **Instant, because it's cheap** | Switching = re-running the game's own `PlaySound` init and re-pointing the sequencer. No file to load, no decode. It lands on the **next engine frame (~17 ms)** — a clean cut, exactly like the game switching tracks, not a crossfade. |
| **Always from the top** | A previewed track starts at its beginning, every time. |
| **Leaving the list snaps back to the truth** | Move off the list and, after a **~400 ms** grace (so crossing a gap between rows doesn't trigger it), playback returns to the **selected** track — the one actually in the save. You always end up back on what's real, never on silence. |
| **Click = select** | Clicking a row writes `musicID` + `musicBank` to the save and makes it the selected row; playback continues on it, and the "previewing" line disappears. |
| **▶ on a row = pin the audition** | Same as hovering it, but it *stays* — the mouse leaving the list won't snap away from a pinned track. Hovering another row un-pins. |
| **Keyboard parity** | ↑/↓ move the highlight and audition it under the same 120 ms rule; **Enter** selects; **Esc** un-pins/snaps back. Hover-only UI is not acceptable. |
| **It stops when it should** | Leaving the Map screen, closing the panel, closing the file, or loading another save → **stop**. Nothing keeps humming behind another screen. |

### 6.4 Default selection

**The map's current music is the selected track** the moment the panel opens — read straight from
`AreaAudio` (`0x2607`/`0x2608`), scrolled into view, checked. ▶ therefore plays *this map's music* with no
clicks. (This is also why `AreaAudio::setTo()`'s id/bank bug has to die in Phase 1: it is the code path that
answers "what is this map's music?")

## 7. Status

- ✅ Research complete; three reference docs written (2026-07-12) — the APU, the sound engine, and the
  **glitch ids** (the last one verified on the cartridge).
- ✅ Save bytes/bits verified against the disassembly; the `setTo()` bug found.
- ✅ UI decided and specced (§6) — placement provisional, deliberately one file.
- ⬜ Phase 1 — not started.
- ⬜ Phases 2–6 — not started.
