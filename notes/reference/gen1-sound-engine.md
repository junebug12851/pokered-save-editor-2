# Gen 1's sound engine — the program that plays the music

_Everything the game does between "play Pallet Town" and the sound chip._ The chip itself is
[`gameboy-apu.md`](gameboy-apu.md); the build plan is [`../plans/music.md`](../plans/music.md); the
two **save flags** we expose are at the bottom (§9) and are **verified against the disassembly**.

**Source of truth:** `pret/pokered` — `home/audio.asm`, `home/fade_audio.asm`, `audio/engine_1.asm`
(+ `_2`, `_3`), `audio/headers/`, `audio/music/`, `audio/sfx/`, `audio/notes.asm`,
`audio/wave_samples.asm`, `macros/scripts/audio.asm`, `constants/audio_constants.asm`,
`ram/wram.asm`. Local clone: `Documents/projects/pokered` (see `reference_pokered_clone`).

---

## 1. The one-paragraph version

Gen 1's music is a **byte-code sequencer**. Each track is a little program per channel — "duty cycle 2",
"note C, length 4", "loop back 3 times", "tempo $0100" — living in ROM. Once a frame (VBlank, ~59.7 Hz)
the game runs `UpdateMusic` on **8 software channels**, each of which counts down a delay and, when it
hits 1, executes commands until it hits the next note. Notes turn into **writes to the APU registers**.
The whole thing is bank-switched: three near-identical copies of the engine live in three ROM banks, and
the *bank* is part of a track's identity — which is exactly why the save stores **both** a music id
**and** a music bank.

**Port doctrine: transliterate, don't interpret.** Where the disassembly does something odd (and it does
— see §6, §7), copy the arithmetic, not the intent. The oddities *are* the sound.

---

## 2. The layers

```
  script / map load ──► PlayMusic / PlayDefaultMusic       (home/audio.asm)
                          │  sets wAudioROMBank, wNewSoundID
                          ▼
                        PlaySound  ──► AudioN_PlaySound     (bank-switched, audio/engine_N.asm)
                          │              initialises the 8 software channels from a header
                          ▼
   every VBlank  ──────► AudioN_UpdateMusic                 (bank-switched)
                          │  per channel: tick delay → run commands → write registers
                          ▼
                        $FF10–$FF3F                          (the APU)

   every VBlank  ──────► FadeOutAudio                        (home/fade_audio.asm)
                          rAUDVOL master-volume ramp; see §9
```

## 3. The three engines and the three banks

| Engine | ROM bank | Holds | Notes |
|--------|---------:|-------|-------|
| `Audio1` | **$02** (2) | Overworld/town/route music, most SFX | The baseline |
| `Audio2` | **$08** (8) | Battle music + battle SFX | **The only one that differs**: adds the low-health alarm hook and the cry frequency/tempo modifiers for battle SFX |
| `Audio3` | **$1F** (31) | Title, credits, Hall of Fame, Oak's Lab, dungeons, Silph, encounter themes… | |

**`engine_1.asm` and `engine_3.asm` are byte-for-byte identical apart from the leading comment**
(verified by diff, 2026-07-12). `engine_2.asm` is the same engine plus the battle extras. So **one C++
engine with an "is bank 8" flavour flag** reproduces all three — do not write three.

These are exactly the banks in our `db/assets/data/music.json` (`bank: 2 / 8 / 31`), and the `02_/08_/1F_`
prefixes the v1 editor used in its dropdown.

## 4. Eight software channels onto four hardware ones

| Software | Hardware | Role |
|----------|----------|------|
| CHAN1–CHAN3 | Pulse A, Pulse B, Wave | **Music** tone channels |
| CHAN4 | Noise | **Music** percussion (drums) |
| CHAN5–CHAN8 | Pulse A, Pulse B, Wave, Noise | **SFX** — they *pre-empt* the music channel on the same hardware |

The pre-emption is not a mixer: when an SFX channel is active, the corresponding music channel simply
**stops writing registers** (`Audio1_ApplyMusicAffects` / `..._note_pitch` check
`wChannelSoundIDs + CHAN5 + c`). The music keeps *counting* — it just goes silent — so when the SFX ends
the music is exactly where it would have been. Cries are SFX. **Drums are SFX too**: a `drum_note` on the
music noise channel calls `PlaySound` with a noise-instrument id (1–19), which plays on CHAN8.

## 5. The engine's entire mind: 243 bytes at $C000

The `"Audio RAM"` WRAM0 section is the **first** section in `ram/wram.asm`, so it starts at `$C000` and
runs to `$C0FF` (the trailing `ds 13` pads it to exactly 256). Every array is `NUM_CHANNELS` = 8 wide,
indexed by software channel.

| Addr | Symbol | Size |
|------|--------|------|
| `$C000` | `wUnusedMusicByte` | 1 |
| `$C001` | `wSoundID` | 1 |
| `$C002` | `wMuteAudioAndPauseMusic` | 1 |
| `$C003` | `wDisableChannelOutputWhenSfxEnds` | 1 |
| `$C004` | `wStereoPanning` | 1 |
| `$C005` | `wSavedVolume` | 1 |
| `$C006` | `wChannelCommandPointers` | 16 (8 × 2) |
| `$C016` | `wChannelReturnAddresses` | 16 |
| `$C026` | `wChannelSoundIDs` | 8 |
| `$C02E` | `wChannelFlags1` | 8 |
| `$C036` | `wChannelFlags2` | 8 |
| `$C03E` | `wChannelDutyCycles` | 8 |
| `$C046` | `wChannelDutyCyclePatterns` | 8 |
| `$C04E` | `wChannelVibratoDelayCounters` | 8 |
| `$C056` | `wChannelVibratoExtents` | 8 |
| `$C05E` | `wChannelVibratoRates` | 8 |
| `$C066` | `wChannelFrequencyLowBytes` | 8 |
| `$C06E` | `wChannelVibratoDelayCounterReloadValues` | 8 |
| `$C076` | `wChannelPitchSlideLengthModifiers` | 8 |
| `$C07E` | `wChannelPitchSlideFrequencySteps` | 8 |
| `$C086` | `wChannelPitchSlideFrequencyStepsFractionalPart` | 8 |
| `$C08E` | `wChannelPitchSlideCurrentFrequencyFractionalPart` | 8 |
| `$C096` | `wChannelPitchSlideCurrentFrequencyHighBytes` | 8 |
| `$C09E` | `wChannelPitchSlideCurrentFrequencyLowBytes` | 8 |
| `$C0A6` | `wChannelPitchSlideTargetFrequencyHighBytes` | 8 |
| `$C0AE` | `wChannelPitchSlideTargetFrequencyLowBytes` | 8 |
| `$C0B6` | `wChannelNoteDelayCounters` | 8 |
| `$C0BE` | `wChannelLoopCounters` | 8 |
| `$C0C6` | `wChannelNoteSpeeds` | 8 |
| `$C0CE` | `wChannelNoteDelayCountersFractionalPart` | 8 |
| `$C0D6` | `wChannelOctaves` | 8 |
| `$C0DE` | `wChannelVolumes` | 8 |
| `$C0E6` | `wMusicWaveInstrument` | 1 |
| `$C0E7` | `wSfxWaveInstrument` | 1 |
| `$C0E8` | `wMusicTempo` | 2 |
| `$C0EA` | `wSfxTempo` | 2 |
| `$C0EC` | `wSfxHeaderPointer` | 2 |
| `$C0EE` | `wNewSoundID` | 1 |
| `$C0EF` | `wAudioROMBank` | 1 |
| `$C0F0` | `wAudioSavedROMBank` | 1 |
| `$C0F1` | `wFrequencyModifier` | 1 |
| `$C0F2` | `wTempoModifier` | 1 |

> 🎯 **This table is the verification oracle.** Our C++ port keeps exactly this state; PyBoy can dump
> `$C000–$C0FF` from the real ROM every frame; a byte-for-byte, frame-by-frame match means our engine is
> *thinking the same thoughts* as the console. See [`emulator-verification.md`](emulator-verification.md)
> and [`gameboy-apu.md`](gameboy-apu.md) §7. **(Addresses derived arithmetically from `wram.asm`;
> confirm against the build's `.sym` before trusting them in a test.)**

`wChannelFlags1` bits (`audio_constants.asm`): 0 `PERFECT_PITCH`, 1 `SOUND_CALL`, 2 `NOISE_OR_SFX`,
3 `VIBRATO_DIRECTION`, 4 `PITCH_SLIDE_ON`, 5 `PITCH_SLIDE_DECREASING`, 6 `ROTATE_DUTY_CYCLE`.
`wChannelFlags2` has one bit: 0 `EXECUTE_MUSIC` (an SFX asking to be read as music).

## 6. The data formats

### Headers — how a track is found

`AudioN_PlaySound` looks a sound up in a flat table: **header address = `SFX_Headers_N` + id × 3**.
Music and SFX share the id space (music ids are the high ones — `MUSIC_PALLET_TOWN` = 186 in bank 2,
which is exactly the `id: 186, bank: 2` in our `music.json`).

Each entry is **3 bytes per channel**:

```
byte 0:  %CCcc_ssss   CC = (channel count - 1)   [only meaningful on the FIRST channel of the header]
                      ssss = software channel index (0–7)
byte 1-2: pointer to that channel's command stream (little-endian, bank-relative)
```

(From `macros/scripts/audio.asm`: `channel_count` stashes `count-1`, `channel` emits
`dn (_num_channels << 2), \1 - 1` then `dw addr`, and resets the count — so only the first channel byte
carries the count in its top two bits.)

### The command stream (`macros/scripts/audio.asm`)

| Byte | Command | Args | Meaning |
|------|---------|------|---------|
| `$10` | `pitch_sweep` | 1 | Writes `rAUD1SWEEP` directly (SFX only) |
| `$2x` | `square_note` / `noise_note` | 2–3 | Raw SFX note: length, volume+fade, frequency (noise: 1 byte) |
| `$0x–$Bx` | `note` | 0 | **High nibble = pitch (0–11 = C…B), low nibble = length − 1** |
| `$Bx` | `drum_note` | 1 | Length in low nibble; next byte = noise instrument (1–19) |
| `$Cx` | `rest` | 0 | Length − 1 in low nibble |
| `$Dx` | `note_type` / `drum_speed` | 0–1 | Low nibble = **note speed**; next byte = volume (high) + fade (low). On **channel 3** the next byte's low nibble is the **wave instrument** and bits 4–5 the level |
| `$Ex` | `octave` | 0 | Low nibble = **8 − octave** (so octave 1…8 ⇒ 7…0) |
| `$E8` | `toggle_perfect_pitch` | 0 | Adds 1 to the frequency register from then on |
| `$EA` | `vibrato` | 2 | delay, then extent (high nibble) + rate (low nibble) |
| `$EB` | `pitch_slide` | 3 | length, target octave+pitch, then a note |
| `$EC` | `duty_cycle` | 1 | 0–3 |
| `$ED` | `tempo` | 2 | **Big-endian** 16-bit. Default `$0100`. Smaller = faster |
| `$EE` | `stereo_panning` | 1 | Left mask (high nibble) + right mask (low) → `wStereoPanning` → `rAUDTERM` |
| `$EF` | `unknownmusic0xef` | 1 | Plays a sound; **never used by the game** |
| `$Fx` | `volume` | 1 | Written straight to `rAUDVOL` (master volume) |
| `$F8` | `execute_music` | 0 | Read this SFX stream as *music* |
| `$FC` | `duty_cycle_pattern` | 1 | Four 2-bit duties, **rotated every frame** |
| `$FD` | `sound_call` | 2 | Subroutine call (one level; return address per channel) |
| `$FE` | `sound_loop` | 3 | count (0 = forever), address |
| `$FF` | `sound_ret` | 0 | Return, or end the channel |

**46 music tracks** (`audio/music/`, matching `music.json`), **19 noise instruments** (the drum kit),
**10 wave instruments** (`audio/wave_samples.asm`, 16 bytes = 32 nibbles each).

### The pitch table — and the trick in it

`audio/notes.asm` is 12 words: `$F82C $F89D $F907 $F96B $F9CA $FA23 $FA77 $FAC7 $FB12 $FB58 $FB9B $FBDA`.

They look like nonsense until you read them as **signed negatives**: `$F82C` = −2004. `CalculateFrequency`
**arithmetic-shifts right by (octave − 1)** and then **adds 8 to the high byte — i.e. +$800 = +2048**:

```
freq_register = 2048 + (pitch_table[note] >>arith (octave - 1))     // then masked to 11 bits
```

So the table stores `x − 2048`, halving the magnitude each octave doubles the frequency — one table for
every octave, in about ten instructions. (C, octave 1 → x = 44 → 65.4 Hz. Octave 2 → 1046 → 130.8 Hz.
Octave 3 → 1549 → 262.7 Hz.) **Do not "clean this up" in the port.**

### Note length — the fixed-point tempo

`Audio1_note_length`, in one line:

```
delay(16-bit) = frac_part + (note_length × note_speed) × tempo
wChannelNoteDelayCounters[c]              = HIGH(delay)   // whole frames until the next note
wChannelNoteDelayCountersFractionalPart[c]= LOW(delay)    // carried into the next note
```

`note_length` = low nibble + 1 (in 16ths); `note_speed` comes from `note_type`; `tempo` defaults to
`$0100`, so at default tempo `delay_frames == length × speed` exactly. The fractional part is what keeps
a "wrong" tempo from drifting.

## 7. The quirks that must survive the port

- **Length enable is mostly OFF.** `Audio1_ApplyWavePatternAndFrequency` does `or $80` (trigger) then
  `and $c7` — which **clears bit 6**, the length-enable. The engine dutifully writes a "sound length"
  into `NRx1` and the hardware mostly ignores it. Notes end because the *engine* starts the next one.
  (The disassembly's comment says "use counter mode" — the comment is misleading; the **bytes** are the
  truth.)
- **The `+8` overflow** in `CalculateFrequency` is the `+2048` above. Its carry-out is discarded, and
  that's fine — the mask throws those bits away.
- **`toggle_perfect_pitch` has a dead branch.** `inc e` doesn't set carry, so the `jr nc` after it always
  taken — the high byte is never incremented. pokered flags this as "likely a mistake"; it doesn't affect
  any note the game actually plays. **Copy the bug.**
- **Pitch-slide borrows from the wrong byte** (`Audio1_InitPitchSlideVars`) — pokered marks it "Bug":
  the result can be `$200` too large. **Copy the bug.**
- **`drum_note_short`** (a 1-byte drum on the noise channel, instruments 1 and 3–10) exists and is
  unused; implement it anyway, it's four lines.
- **Vibrato's extent split** is `(n/2 + n%2)` above the note and `(n/2)` below — asymmetric by design.
- **Cries** rewind the command pointer by one (`GoBackOneCommandIfCry`) and stash/restore `rAUDVOL`
  (`wSavedVolume`) — they're the reason `wFrequencyModifier` / `wTempoModifier` exist.

## 8. Fade-out and bank switching (`home/audio.asm`, `home/fade_audio.asm`)

`FadeOutAudio` runs every frame. If a fade is in progress (`wAudioFadeOutControl` ≠ 0) it decrements
**both nibbles of `rAUDVOL`** every `wAudioFadeOutCounter` frames; when the volume hits 0 it stops all
music, swaps `wAudioSavedROMBank → wAudioROMBank` (**this is how a track in another bank gets started**)
and plays the queued sound. If no fade is in progress, it **forces `rAUDVOL` back to `$77`** (full) —
*unless* the No-Audio-Fade-Out flag is set (§9).

## 9. The two save flags — VERIFIED

The save→WRAM map for the main data block: **`file_offset = 0x25A3 + (wram_addr − 0xD2F7)`**
(`sMainData` copies from `wMainDataStart` = `wPokedexOwned` = `$D2F7`). Cross-checked by the byte we
already knew: `0x2609` ⇒ `$D35D` = `wMapPalOffset`, the "contrast" byte from
[`palettes.md`](palettes.md). ✅

| Save | WRAM | Symbol | What it really is |
|------|------|--------|-------------------|
| `0x2607` | `$D35B` | `wMapMusicSoundID` | The **track id** for the current map |
| `0x2608` | `$D35C` | `wMapMusicROMBank` | The **bank** (2 / 8 / 31) it lives in |
| `0x29D8` bit 1 | `$D72C` = `wStatusFlags2` | `BIT_NO_AUDIO_FADE_OUT` | **"No Audio Fadeout"** |
| `0x29DF` bit 1 | `$D733` = `wStatusFlags7` | `BIT_NO_MAP_MUSIC` | **"Prevent Music Change"** |

**`BIT_NO_AUDIO_FADE_OUT` — what it actually does.** In `FadeOutAudio`, when *no* fade is running, the
game normally slams `rAUDVOL` back to `$77` (full volume) **every single frame**. With this bit set, it
returns instead and **leaves the master volume alone**. The game sets it while the **Pokédex**
(`engine/menus/pokedex.asm`) and the **Pokémon status screen** (`engine/pokemon/status_screen.asm`) are
open — the screens that play cries, which fiddle with the master volume themselves — and clears it on
the way out. Left set in a save, the master volume simply stops being restored. It is a "don't touch my
volume" flag, and "No Audio Fadeout" is the name the constant gave it.

**`BIT_NO_MAP_MUSIC` — what it actually does.** At the end of loading a map (`home/overworld.asm`,
`LoadMapData`), the game calls `UpdateMusic6Times` + `PlayDefaultMusicFadeOutCurrent` — *unless* this bit
is set, in which case it **skips both and whatever is playing keeps playing**. It's set by
`engine/battle/core.asm` and by scripted auto-movement (`engine/overworld/auto_movement.asm`) — that's
why the escort/rival music follows you across map boundaries — and explicitly cleared by the
`HallOfFame` and `OaksLab` scripts. Left set in a save, entering maps will not change the music.

**We already read and write all four correctly** (`savefile/…/expanded/area/areaaudio.cpp`).

> 🐞 **Bug found (2026-07-12), not yet fixed:** `AreaAudio::setTo(MapDBEntry*)` says
> `musicBank = musicID = musicEntry->bank;` — it **clobbers `musicID` with the bank**, so setting a map's
> default music writes the wrong track id. (`load()`/`save()`/`randomize()` are fine.) Fix in the Phase 1
> work of [`../plans/music.md`](../plans/music.md), with a test.

## 10. What this gives us for free, later

Because the port is *the engine*, not "a music player", the same machine also plays: **every SFX**, the
**19-piece drum kit**, the **low-health alarm**, the **Poké Flute**, and — with `wFrequencyModifier` /
`wTempoModifier` — **all 151 Pokémon cries** (base cry + per-species pitch/length, which are already in
our Pokémon data). None of that is in scope now. All of it is nearly free once §5–§7 are real.
