# Glitch music — what a bad music id (or bank) actually does

The save holds a music **id** (`0x2607`) and a music **bank** (`0x2608`), and **nothing in the game
validates either**. So what happens when they're wrong?

The answer turns out to be one of the nicest findings in this project:

> **There is almost no such thing as a garbage music id in Gen 1.**
> Most "glitch" ids are a real song's **inner voices** — one channel of it, playing alone.
> There are **105 of them**, they are perfectly deterministic, and they are *music*.
>
> The **bank**, on the other hand, is a loaded gun: an invalid bank **hangs the console**.

Everything below is **verified against the real cartridge** — statically with
`scripts/emu/analyze_music_ids.py` (which parses all 256 ids × 3 banks out of the ROM and
disassembles their command streams) and on the console with `scripts/emu/probe_glitch_music.py`
(which patches the id/bank into a real save, fixes the checksum, boots the game, and reads its APU and
audio RAM back out). Both are local-only and skip without a ROM. Background:
[`gen1-sound-engine.md`](gen1-sound-engine.md), [`emulator-verification.md`](emulator-verification.md).

---

## 1. Why the glitch ids are music: the id *is* an address

From `constants/music_constants.asm`, first line: *"Song ids are calculated by address to save space."*

```
id = (header_address - SFX_Headers_1) / 3
header_address = SFX_Headers_N + id * 3
```

And a header is **3 bytes per channel**. So **a 3-channel song occupies three consecutive ids** — but
only the first one is the song. What are the other two?

They are its **channel 2 and channel 3 entries**, read as headers in their own right. And they parse
perfectly: the channel-count bits (the top 2 of byte 0) are zero on a non-first channel entry, so the
header reads as *one channel*, and its pointer is that song's channel-2 (or -3) command stream.

**Pallet Town is id 186. Id 187 is Pallet Town's bassline, alone.**

Header base is `$4000` in each of the three audio banks — **confirmed**: parsing from there resolves all
46 real tracks and every one of their channel pointers exactly.

## 2. The whole 256-id space, per bank (from the cartridge)

| Bank | Engine | Music region | Real tracks | **Inner voices** | Everything below the music region |
|------|--------|--------------|------------:|-----------------:|-----------------------------------|
| **2** | Audio1 | ids 186–254 | 20 | **49** | ids 0–185 = SFX headers |
| **8** | Audio2 | ids 234–254 | 7 | **14** | ids 0–233 = SFX headers |
| **31** | Audio3 | ids 195–254 | 19 | **42** | ids 0–194 = SFX headers |

**In the entire music region there is not one garbage id.** Every single one is either a real track or
one of its inner voices. (The lone exception, id `$FF` = 255, is `SFX_STOP_ALL_MUSIC` — special-cased
before the table is ever read. Our `music.json` already calls it `None`.)

**105 inner voices.** Examples, with the note counts the disassembler found:

| id | is | notes | ends how |
|----|----|------:|----------|
| 187 | Pallet Town **ch2** | 72 | loops forever |
| 188 | Pallet Town **ch3** | 42 | loops forever |
| 196–198 (bank 31) | Title Screen **ch2 / ch3 / ch4** | 102 / 148 / 159 | loop forever |
| 213 | Lavender **ch2** | **4** | loops forever — a four-note drone |
| 235–236 (bank 8) | Gym Leader Battle **ch2 / ch3** | 177 / 251 | loop forever |
| 252–254 | Indigo Plateau **ch2 / ch3 / ch4** | 66 / 66 / 190 | loop forever |
| 235 (bank 31) | Dungeon 3 **ch4** | **0** | ends — a track that plays nothing |

Full machine-generated table: run the analyser (it writes `tmp/music_ids.json`).

> 🐞 **Two disassembler bugs were in the first pass of these numbers** (found 2026-07-12 by the
> cartridge cross-check in `scripts/import_music.py`, and now fixed in both tools). They inflated the
> **note counts** — the structure above (which ids are inner voices, and of what) was never affected:
>
> 1. **`$2x` is only an SFX note on CHAN4–CHAN8.** On the three music tone channels it is an ordinary
>    one-byte **note**. Reading it as a 4-byte `square_note` walks off the beat and turns the rest of
>    the song into nonsense.
> 2. **`sound_loop 0` is a terminator.** It never falls through — so walking past it wanders straight
>    into the *next song's* bytes and counts them too.

### The console agrees

`probe_glitch_music.py`, on the real cartridge, walking onto the map with the id patched in:

| Save's id | `wChannelSoundIDs` | NR51 (which hw channels reach the speakers) | pitch values seen |
|-----------|--------------------|---------------------------------------------|-------------------|
| 186 (real Pallet Town) | `[186, 186, 186, 0, …]` | `$77` — channels 1+2+3, both sides | ch1: 8, ch2: 11, ch3: 7 |
| **187 (the "glitch")** | `[0, **187**, 0, 0, …]` | `$22` — **channel 2 only** | ch2: 10 |
| **213 (Lavender ch2)** | `[0, **213**, 0, 0, …]` | `$22` — **channel 2 only** | ch2: 7 |

One software channel loaded, one hardware channel enabled, the pitch moving. That is not a glitch. That
is a solo.

## 3. The other id classes

**SFX ids (below the music region).** `PlaySound` branches on `id <= MAX_SFX_ID_N` → the SFX path. Set
one as a map's music and the map plays **a sound effect, once**, and is then silent. Verified: id 30 →
`wChannelSoundIDs` all zero once it's over; nothing sustains. This class includes the **19 noise
instruments** (a single drum hit) and the **38 cry slots** (a Pokémon cry as your map's "music" —
`AudioN_IsCry` even applies the cry's frequency/tempo modifiers to it).

**Id 255 (`$FF`) = `SFX_STOP_ALL_MUSIC`.** Silence, by design. Verified: NR51 = `$00`, nothing plays.

**Id 0 — the one genuinely strange one.** The header table's first entry is a dummy `$FF $FF $FF`. But
the map path never reads it: `PlayDefaultMusicFadeOutCurrent` sets `wAudioFadeOutControl = 10` **before**
calling `PlaySound`, and `PlaySound` bails out immediately when the new sound id is 0 — leaving that 10
sitting there. So `FadeOutAudio` fades the current music to nothing and then, at `.fadeOutComplete`,
plays sound id **10** — which is **noise instrument 10**. Your map's music fades out and you get a drum
hit.
**The console backs this up exactly**: with id 0, the only hardware channel that ever reaches the
speakers is NR51 `$88` — **channel 4, the noise channel, alone.**

## 4. ⚠️ The bank byte is not the same kind of thing at all

`PlaySound` switches the ROM bank to **whatever the save says**, and *then* decides which engine to call:

```asm
ld a, [wAudioROMBank]
ldh [hLoadedROMBank], a
ld [rROMB], a            ; <-- the save's byte is now the mapped bank
cp BANK(Audio1_PlaySound)   ; 2?
jr nz, .checkForAudio2
...
.audio3
call Audio3_PlaySound       ; <-- otherwise: call Audio3's ADDRESS, with the wrong bank mapped
```

If the bank isn't 2 or 8, the game calls `Audio3_PlaySound`'s **address** — but the bank now mapped in
is the save's. So it executes **whatever bytes happen to live at that address in that bank, as code**.
Not glitch music. **Arbitrary code execution**, and `UpdateMusic6Times` does the same thing again every
frame.

**Verified on the console (2026-07-12):** with `bank = 5`, the emulator ran the front end normally
(720 frames in 0.1 s), and then, **the moment the save loaded and the map came up, it stopped producing
frames entirely.** The console is dead. Not corrupted audio — dead.

> **Editor consequence.** The bank picker must only ever offer **2, 8, 31**. If a save arrives holding
> anything else we **show it, we never silently rewrite it** (save fidelity is sacred), and we say
> plainly what it means: *this save hangs the real game.*

## 5. What this means for us — the perk

**Everything in the music region plays with the data we were already going to import.** Every inner
voice points *into* a real track's command stream. Nothing in the music region points at arbitrary ROM.
So a shipped copy of the editor, with no cartridge anywhere near it, can faithfully play:

- the **46 real tracks**, and
- **105 inner voices** — the individual channels of those songs, exactly as the console plays them.

**151 pieces of playable audio, for the price of 46.** They are not curiosities: Pallet Town's bassline,
the Title Screen's four voices taken apart, Lavender Town's four-note drone. And they are *reachable in
a real save*, which is the whole reason we care — a player can put one of these in their game.

Add the SFX data (already needed for drums, Phase 6) and the SFX/cry ids become playable too. **No id in
any valid bank needs anything outside the audio data.** That is luck, and we should take it.

### Sheet music

Because we are running the *engine*, not analysing a recording, we know each note **exactly** — pitch,
octave, length, duty, volume, tempo, and every ornament. No pitch detection, no guessing. A
`Transcriber` sink on the engine (planned: [`../plans/music.md`](../plans/music.md) → Phase 7) can dump
any of the 151 tracks as **MIDI** and as real notation (**MusicXML / LilyPond**), following a track until
it ends or its loop point comes back around — which the analyser already detects (`sound_loop` with
count 0 → "loops forever", i.e. the repeat mark).

The inner voices are the ones that *want* this: a single monophonic line is exactly what a stave is for.

## 6. The tools

| Script | What it does | Needs the ROM? |
|--------|--------------|----------------|
| `scripts/emu/analyze_music_ids.py` | Parses all 256 ids × 3 banks straight out of the cartridge the way `PlaySound` does, disassembles every channel's command stream, follows calls/loops, and classifies each id. Writes `tmp/music_ids.json`. | Yes (exits 2 without it) |
| `scripts/emu/probe_glitch_music.py` | Patches an id/bank into a real save (**fixing the checksum**, or the game rejects it), boots the cartridge, walks onto the map, and reads back `wChannelSoundIDs`, `wAudioROMBank`, NR51/NR52 and the engine's own pitch bytes. | Yes (exits 2 without it) |

Both are local-only and never ship. The ROM (`assets/references/backup.gb`) is Twilight's own cartridge
backup: git-ignored, never committed, never distributed.
