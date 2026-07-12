# The Game Boy APU — the sound hardware

_The machine underneath the music._ This file documents the **DMG's sound chip** — what it is, what
its registers do, and exactly what a faithful software model has to reproduce. It says nothing about
Pokémon; the *game's* sound engine (which drives this chip) is
[`gen1-sound-engine.md`](gen1-sound-engine.md), and the build plan is [`../plans/music.md`](../plans/music.md).

Read this one first. The Gen 1 engine is, in the end, just a program that writes bytes into these
registers 60 times a second. **If we model the chip, the music comes out on its own.**

> ⚠️ **Status: researched, not yet verified.** Every number below is from the standard hardware
> documentation (Pan Docs / gbdev) and is written here as the *target* for the implementation. As with
> the map, nothing here counts as true until the **console agrees** — see "Verification" at the bottom
> and [`emulator-verification.md`](emulator-verification.md).

---

## 1. The shape of it

The DMG APU is **four independent sound channels** feeding a **two-channel (stereo) mixer**:

| # | Channel | What it is | Can it change pitch? | Envelope? |
|---|---------|-----------|----------------------|-----------|
| 1 | Pulse A | Square wave, 4 duty cycles, **+ frequency sweep** | yes | yes |
| 2 | Pulse B | Square wave, 4 duty cycles (no sweep) | yes | yes |
| 3 | Wave | Plays a **32-sample, 4-bit waveform** out of Wave RAM | yes | **no** (4 fixed volume levels) |
| 4 | Noise | Pseudo-random bits from an **LFSR** | (noise "pitch" = clock rate) | yes |

Everything is driven from the CPU clock, **4 194 304 Hz**. There is no floating point anywhere: it is
counters, shift registers and a 4-bit DAC per channel.

Two clocks matter:

- **The frame sequencer** — a 512 Hz tick that drives length counters, envelopes and the sweep.
- **The channel frequency timers** — per-channel counters that advance the waveform.

Pokémon's sound engine sits *above* all of this at the **~59.7 Hz VBlank rate**: once a frame it decides
what the registers should say. Nothing in the game code knows the frame sequencer exists.

---

## 2. The registers ($FF10–$FF26, + Wave RAM $FF30–$FF3F)

Names in the left column are the modern gbdev names; the ones in brackets are what **pokered** calls
them (which is what we'll see in the engine code).

### Channel 1 — pulse with sweep

| Addr | Reg | Bits |
|------|-----|------|
| $FF10 | NR10 `[rAUD1SWEEP]` | `-PPP DSSS` — sweep **P**ace (0–7), **D**irection (0 = up, 1 = down), **S**hift (0–7) |
| $FF11 | NR11 `[rAUD1LEN]` | `DDLL LLLL` — **D**uty (0–3), **L**ength load (6 bits, write-only) |
| $FF12 | NR12 `[rAUD1ENV]` | `VVVV DPPP` — initial **V**olume (0–15), **D**irection (0 = decrease), envelope **P**ace (0–7) |
| $FF13 | NR13 `[rAUD1LOW]` | Period (frequency) **low 8 bits** — write-only |
| $FF14 | NR14 `[rAUD1HIGH]` | `TL-- -PPP` — **T**rigger (restart), **L**ength enable, period **high 3 bits** |

### Channel 2 — pulse

$FF16/$FF17/$FF18/$FF19 = NR21/22/23/24 `[rAUD2LEN/ENV/LOW/HIGH]`. Identical to channel 1 minus the
sweep register. (Note $FF15 does not exist — this is why pokered's `HW_CH2_BASE` is `LOW(rAUD2LEN) - 1`:
the engine indexes registers as `base + 1/2/3`, so it needs a phantom slot where channel 1's sweep is.)

### Channel 3 — wave

| Addr | Reg | Bits |
|------|-----|------|
| $FF1A | NR30 `[rAUD3ENA]` | `E--- ----` — DAC **E**nable. **0 kills the channel dead** (not just silent). |
| $FF1B | NR31 `[rAUD3LEN]` | Length load, full 8 bits |
| $FF1C | NR32 `[rAUD3LEVEL]` | `-VV- ----` — output level: **0 = mute, 1 = 100%, 2 = 50%, 3 = 25%** (i.e. shift right by 0/1/2) |
| $FF1D | NR33 `[rAUD3LOW]` | Period low 8 bits |
| $FF1E | NR34 `[rAUD3HIGH]` | Trigger / length enable / period high 3 bits |
| $FF30–$FF3F | Wave RAM | **16 bytes = 32 samples**, 4 bits each, **high nibble first** |

Channel 3 has **no volume envelope** — only those four coarse levels. This is why Gen 1's `note_type`
command stores a *wave instrument index* for channel 3 where the other channels store volume+fade.

### Channel 4 — noise

| Addr | Reg | Bits |
|------|-----|------|
| $FF20 | NR41 `[rAUD4LEN]` | `--LL LLLL` — length load (6 bits) |
| $FF21 | NR42 `[rAUD4ENV]` | Same envelope layout as NR12 |
| $FF22 | NR43 `[rAUD4POLY]` | `SSSS WDDD` — clock **S**hift, **W**idth (0 = 15-bit, 1 = 7-bit), **D**ivisor code |
| $FF23 | NR44 `[rAUD4GO]` | `TL-- ----` — Trigger / Length enable |

### The mixer

| Addr | Reg | Bits |
|------|-----|------|
| $FF24 | NR50 `[rAUDVOL]` | `-LLL -RRR` — master volume left/right (0–7). *(Bit 7/3 are the VIN mixes; unused on the GB.)* |
| $FF25 | NR51 `[rAUDTERM]` | `4321 4321` — high nibble = which channels go **left**, low nibble = **right** |
| $FF26 | NR52 `[rAUDENA]` | `E--- 4321` — bit 7 = **APU power**; bits 0–3 = per-channel "is playing" (read-only) |

**NR51 is how Gen 1 turns channels on and off.** The engine never disables a channel properly — it
just clears that channel's bits in `rAUDTERM` so it stops reaching the speakers. Hence pokered's
`HW_CHn_ENABLE_MASK` / `DISABLE_MASK` tables (`%00010001`, `%00100010`, …): one mask sets/clears both
the left and right bit for a channel at once. Our mixer must honour that, per side.

---

## 3. The maths a synth has to get right

**Pulse (ch 1 & 2).** The 11-bit register value `x` is a *period*, not a frequency:

```
frequency (Hz) = 131072 / (2048 - x)
```

Implementation-wise: a timer reloads with `(2048 - x)` and is clocked at **1 048 576 Hz** (every 4 CPU
cycles); each expiry advances an 8-step **duty pointer**. The duty patterns (one bit per step):

| Duty | Pattern | Sound |
|------|---------|-------|
| 0 | `00000001` | 12.5 % |
| 1 | `10000001` | 25 % |
| 2 | `10000111` | 50 % (a true square) |
| 3 | `01111110` | 75 % |

**Wave (ch 3).** Same 11-bit period, but clocked at **2 097 152 Hz** (every 2 cycles) and stepping
through **32** nibbles, so:

```
frequency (Hz) = 65536 / (2048 - x)
```

i.e. **one octave below** a pulse channel at the same register value — which is exactly why Gen 1 can
share one pitch table across all three tone channels and still have the wave channel sit where it does
in the mix.

**Noise (ch 4).** The LFSR is clocked at

```
frequency (Hz) = 262144 / (divisor × 2^shift)
```

where `divisor` comes from the 3-bit code via the table **{8, 16, 32, 48, 64, 80, 96, 112}** (code 0 is
8, *not* 0). On each clock: `bit = (LFSR ^ (LFSR >> 1)) & 1`, shift right, put `bit` into bit 14; in
**7-bit width mode** also put it into bit 6, which shortens the sequence into a metallic buzz. The
output is `~LFSR & 1`.

**Volume envelope (ch 1, 2, 4).** Volume 0–15, stepped up or down by 1 every `pace` ticks of the 64 Hz
envelope clock. `pace = 0` means **off** (no stepping), not "fastest".

**Sweep (ch 1 only).** Every `pace` ticks of the 128 Hz sweep clock:
`new = old ± (old >> shift)`. If `new > 2047` the channel is **disabled**. (The DMG's sweep has a famous
overflow-check quirk on trigger; it must be modelled — Gen 1 *does* write `rAUD1SWEEP`, both to disable
it (`AUD1SWEEP_DOWN`) and, for SFX, via the `pitch_sweep` command.)

**DAC.** Each channel's 0–15 output goes through its own 4-bit DAC. A channel whose DAC is off is
**silent and off**, regardless of NR51: for pulse/noise the DAC is off when the envelope register's
volume is 0 *and* the direction is "decrease" (`NRx2 & $F8 == 0`); for wave it's NR30 bit 7.

**Mixing.** Per side: sum the DACs of the channels enabled for that side in NR51, then scale by
`(NR50 side volume + 1) / 8`. There is no clipping headroom to speak of — four channels at full tilt is
the loudest the machine gets.

---

## 4. The frame sequencer

A 512 Hz counter (CPU / 8192), 8 steps, wrapping:

| Step | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|------|---|---|---|---|---|---|---|---|
| Length (256 Hz) | ✔ | | ✔ | | ✔ | | ✔ | |
| Sweep (128 Hz) | | | ✔ | | | | ✔ | |
| Envelope (64 Hz) | | | | | | | | ✔ |

**Length counters** only do anything when that channel's "length enable" bit (NRx4 bit 6) is set.
⚠️ **Gen 1 mostly leaves length enable OFF** (see the `and $c7` in `Audio1_ApplyWavePatternAndFrequency`,
[`gen1-sound-engine.md`](gen1-sound-engine.md) §6) — so the length values the engine writes into NRx1
mostly *don't* stop notes. Notes end because the *engine* starts the next one. Do not "fix" this.

---

## 5. Triggering (NRx4 bit 7)

Writing a 1 to bit 7 **restarts** the channel: the frequency timer reloads, the envelope reloads from
NRx2 (volume + pace), the length counter reloads if it was 0, the LFSR refills with 1s, and the wave
channel's position resets to 0. This — not the length counter — is what makes each Gen 1 note *attack*.
Getting trigger semantics right is most of getting the music right.

---

## 6. What we must build

A `GbApu` class that is a **write-only register sink plus a sample generator**:

```
apu.write(0xFF10..0xFF3F, byte)   // exactly what the game writes, when it writes it
apu.tick(cycles)                  // advance the hardware by N CPU cycles
apu.render(buffer, frames)        // 48 kHz stereo float/int16 out
```

The Gen 1 engine port calls `write()`; a frame loop calls `tick(70224)` (one Game Boy frame's worth of
cycles) per 1/59.7 s; the audio thread pulls `render()`. **Nothing above the APU should know what a
"note" is.** That separation is the whole design — see [`../plans/music.md`](../plans/music.md).

### Deliberately in scope

- Duty pointer, wave position, LFSR — cycle-driven, not "close enough" oscillators.
- Envelope/sweep/length on the real frame sequencer.
- NR51 per-side enable and NR50 master volume (Gen 1's stereo panning command depends on it).
- DAC-off = channel dead.
- Downsample 4.194 MHz → 48 kHz with a proper filter (naive decimation aliases audibly on the 12.5 %
  duty and on the noise channel).

### Deliberately out of scope (for now, and to be recorded here if that changes)

- CGB double-speed, CGB wave-RAM read quirks, the "zombie mode" envelope glitch, the obscure
  length-counter-on-trigger edge cases. **Unless the console says we need them** — verification decides,
  not taste.

---

## 7. Verification — the console is the judge

Same doctrine as the map: internal consistency proves nothing.

1. **Engine parity (the strong one).** The Gen 1 engine keeps its **entire state in 243 bytes of WRAM at
   `$C000`**. Boot the real ROM in PyBoy, play a track, and dump `$C000–$C0FF` **every frame**; run our
   C++ port for the same number of frames and demand a **byte-for-byte match, frame by frame**. If our
   engine's mind matches the console's mind at every frame, it is writing the same registers.
2. **Register parity.** Log the APU register writes the emulator makes each frame and compare with ours.
3. **Audio parity (the weak one).** Compare rendered PCM against the emulator's own output — useful as a
   smell test, not a gate (resampling/filters differ legitimately).

(1) is the one that matters, and it is cheap: we already boot the ROM with a save for
`tst_emu_parity`. See [`emulator-verification.md`](emulator-verification.md).

---

## Sources

- **Pan Docs** (gbdev) — Audio Registers / APU behaviour. The reference for everything in §2–§5.
- **pret/pokered** — `constants/audio_constants.asm`, `audio/engine_1.asm`: how the game actually pokes
  this hardware.
- **The cartridge** — the only source that gets the final word.
