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

- `MusicPlayer` on `brg.music`: `play(id, bank)`, `stop()`, `volume`, `isPlaying`, `currentTrack`.
  `QAudioSink`, audio thread, ring buffer, no UI-thread jank.
- Map screen: ▶ per track; **hover-preview** (playing + hover a track ⇒ switch to it, with a small debounce
  so a fast sweep doesn't machine-gun the engine); stop; volume.
- Audio **never starts by itself** and never plays on app launch. Nothing steals her attention — the same
  rule as windows. (See `principles.md` → "What the App Should Feel Like".)
- Screenshot review + in-app review with Twilight.

### Phase 6 — the free stuff (later, optional)

SFX, the 19-piece drum kit, the Poké Flute, the low-health alarm, and **all 151 cries** (base cry +
per-species pitch/length, already in our Pokémon data). Nearly free once Phase 4 is real. **Not in scope
until asked.**

## 5. Risks, and what we do about them

| Risk | Mitigation |
|------|-----------|
| **The port is subtly wrong and it "sounds fine"** — the classic chiptune trap | The frame-by-frame `$C000` parity test. Ears are not evidence; the console is. |
| Qt Multimedia latency/underruns making hover-preview feel bad | Generous ring buffer; engine runs ahead; preview switches on the next engine frame, not the next sample. Measure before tuning. |
| Shipping the game's music data | Twilight's call, taken above; it is the same category as the map blocks we already ship, imported from the same public disassembly. Credited. If that ever needs to change, the import script is the single place to change it. |
| Scope creep into SFX/cries | Phase 6 is explicitly fenced off. |
| APU edge cases (zombie envelope, wave-RAM quirks) | Not implemented until the parity/PCM evidence says they're needed. Record the decision here if they are. |

## 6. Open questions for Twilight

- **Where exactly on the Map screen?** The audio controls need a home in that layout (a panel? a tab
  alongside the map view?). This is a design decision → hers, not mine.
- **Track list ordering** — game order (the id order in `music.json`) or grouped (towns / routes / battle /
  dungeons)? Hover-preview makes grouping much nicer to skim.
- **Should the Map screen preview the map's *current* music automatically** when you press play, or always
  start from the picker's selection?

## 7. Status

- ✅ Research complete; both reference docs written (2026-07-12).
- ✅ Save bytes/bits verified against the disassembly; the `setTo()` bug found.
- ⬜ Phase 1 — not started.
- ⬜ Phases 2–6 — not started.
