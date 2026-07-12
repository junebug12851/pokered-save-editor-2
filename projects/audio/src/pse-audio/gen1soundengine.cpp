/*
  * Copyright 2026 Twilight
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/

/**
 * @file gen1soundengine.cpp
 * @brief `audio/engine_1.asm`, transliterated. See gen1soundengine.h.
 *
 * Every routine below keeps the disassembly's name so the two can be read side by side. Where the
 * original does something that looks wrong, the comment says so and the code does it anyway.
 */
#include "./gen1soundengine.h"

#include <cstring>

namespace pse::audio {

namespace {

// Software channels. CHAN1-4 are the music channels, CHAN5-8 the SFX ones.
enum { CHAN1 = 0, CHAN2, CHAN3, CHAN4, CHAN5, CHAN6, CHAN7, CHAN8 };

// wChannelFlags1 bits
enum {
  BIT_PERFECT_PITCH = 0, BIT_SOUND_CALL = 1, BIT_NOISE_OR_SFX = 2, BIT_VIBRATO_DIRECTION = 3,
  BIT_PITCH_SLIDE_ON = 4, BIT_PITCH_SLIDE_DECREASING = 5, BIT_ROTATE_DUTY_CYCLE = 6,
};
constexpr int BIT_EXECUTE_MUSIC = 0; // wChannelFlags2

// Register indices, as the engine addresses them: base + 1/2/3.
enum { REG_DUTY_SOUND_LEN = 1, REG_VOLUME_ENVELOPE = 2, REG_FREQUENCY_LO = 3 };

// The low byte of each hardware channel's base address (audio_constants.asm). Channel 2 has no
// sweep register, hence the phantom slot at rAUD2LEN - 1.
constexpr uint8_t HW_BASE[8] = {0x10, 0x15, 0x1A, 0x1F, 0x10, 0x15, 0x1A, 0x1F};
constexpr uint8_t HW_ENABLE[8] = {0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88};
constexpr uint8_t HW_DISABLE[8] = {0xEE, 0xDD, 0xBB, 0x77, 0xEE, 0xDD, 0xBB, 0x77};

/**
 * The pitch table (audio/notes.asm) -- and the trick in it.
 *
 * These look like nonsense until you read them as SIGNED NEGATIVES: $F82C is -2004. The engine
 * arithmetic-shifts one right by (octave - 1) and then "adds 8 to the high byte" -- which is
 * +$800 = +2048. So the table stores `x - 2048`, and halving its magnitude doubles the frequency:
 * one 12-entry table covers all eight octaves in about ten instructions.
 */
constexpr uint16_t PITCHES[12] = {
    0xF82C, 0xF89D, 0xF907, 0xF96B, 0xF9CA, 0xFA23,
    0xFA77, 0xFAC7, 0xFB12, 0xFB58, 0xFB9B, 0xFBDA,
};

/// hl = l + (a * de), 16-bit, wrapping. (Audio1_MultiplyAdd)
uint16_t multiplyAdd(uint8_t a, uint16_t de, uint8_t l)
{
  uint16_t hl = l;
  while (a) {
    if (a & 1) hl = static_cast<uint16_t>(hl + de);
    a >>= 1;
    de = static_cast<uint16_t>(de << 1);
  }
  return hl;
}

} // namespace

Gen1SoundEngine::Gen1SoundEngine(GbApu* apu) : apu(apu) {}

void Gen1SoundEngine::setData(const Bank& b2, const Bank& b8, const Bank& b31,
                              const std::vector<uint8_t>& w)
{
  banks[0] = b2;
  banks[1] = b8;
  banks[2] = b31;
  waves = w;
}

const Gen1SoundEngine::Bank* Gen1SoundEngine::bank() const
{
  return &banks[curBank];
}

uint8_t Gen1SoundEngine::romByte(uint16_t addr) const
{
  const Bank* b = bank();
  const int off = addr - 0x4000;
  if (off < 0 || off >= static_cast<int>(b->data.size()))
    return 0xFF;   // off the end of what we imported: read a sound_ret rather than crash
  return b->data[static_cast<size_t>(off)];
}

void Gen1SoundEngine::writeReg(int c, int reg, uint8_t v)
{
  apu->write(static_cast<uint16_t>(0xFF00 + HW_BASE[c] + reg), v);
}

uint8_t Gen1SoundEngine::readReg(int c, int reg) const
{
  return apu->read(static_cast<uint16_t>(0xFF00 + HW_BASE[c] + reg));
}

bool Gen1SoundEngine::isPlaying() const
{
  for (int c = 0; c < 8; ++c)
    if (ram[W_SOUND_IDS + c]) return true;
  return false;
}

// ---------------------------------------------------------------------------------- PlayMusic

void Gen1SoundEngine::playMusic(uint8_t bankNo, uint8_t id)
{
  curBank = bankNo == 2 ? 0 : (bankNo == 8 ? 1 : 2);
  // NOTE: on the console, a bank that isn't 2/8/31 executes arbitrary cartridge bytes AS CODE and
  // hangs the machine (verified -- see notes/reference/glitch-music.md). We cannot and will not
  // emulate that; the UI refuses to offer such a bank. Here we simply fall back to Audio3, which
  // is what the game's dispatch *would* have called.
  r(W_AUDIO_ROM_BANK) = bankNo;
  r(W_AUDIO_SAVED_BANK) = bankNo;
  r(W_NEW_SOUND_ID) = id;
  playSound(id);
}

void Gen1SoundEngine::stopAll()
{
  playSound(SFX_STOP_ALL_MUSIC);
}

void Gen1SoundEngine::playSound(uint8_t id)
{
  r(W_SOUND_ID) = id;

  // ---- SFX_STOP_ALL_MUSIC
  if (id == SFX_STOP_ALL_MUSIC) {
    apu->write(0xFF26, 0x80);  // sound hardware on
    apu->write(0xFF1A, 0x80);  // wave playback on
    apu->write(0xFF25, 0x00);  // no output
    apu->write(0xFF1C, 0x00);  // mute channel 3
    apu->write(0xFF10, 0x08);  // sweep off
    apu->write(0xFF12, 0x08);
    apu->write(0xFF17, 0x08);
    apu->write(0xFF21, 0x08);
    apu->write(0xFF14, 0x40);
    apu->write(0xFF19, 0x40);
    apu->write(0xFF23, 0x40);
    apu->write(0xFF24, 0x77);

    r(W_UNUSED) = 0;
    r(W_DISABLE_OUT) = 0;
    r(W_MUTE) = 0;
    r(W_MUSIC_TEMPO + 1) = 0;
    r(W_SFX_TEMPO + 1) = 0;
    r(W_MUSIC_WAVE) = 0;
    r(W_SFX_WAVE) = 0;
    std::memset(ram + W_CMD_PTRS, 0, 0xA0);
    std::memset(ram + W_NOTE_DELAY, 1, 0x18);
    r(W_MUSIC_TEMPO) = 1;
    r(W_SFX_TEMPO) = 1;
    r(W_STEREO_PANNING) = 0xFF;
    return;
  }

  const bool asMusic = id > bank()->maxSfxId && id != 0xFF;

  if (asMusic) {
    // ---- .playMusic: a full re-initialisation of every channel
    r(W_UNUSED) = 0;
    r(W_DISABLE_OUT) = 0;
    r(W_MUSIC_TEMPO + 1) = 0;
    r(W_MUSIC_WAVE) = 0;
    r(W_SFX_WAVE) = 0;

    std::memset(ram + W_CMD_PTRS, 0, 16);
    std::memset(ram + W_RET_ADDRS, 0, 16);
    for (int off : {W_SOUND_IDS, W_FLAGS1, W_DUTY, W_DUTY_PAT, W_VIB_DELAY, W_VIB_EXTENT,
                    W_VIB_RATE, W_FREQ_LO, W_VIB_DELAY_RELOAD, W_FLAGS2, W_PS_LEN_MOD, W_PS_STEP,
                    W_PS_STEP_FRAC, W_PS_CUR_FRAC, W_PS_CUR_HI, W_PS_CUR_LO, W_PS_TGT_HI,
                    W_PS_TGT_LO})
      std::memset(ram + off, 0, 4);   // NUM_MUSIC_CHANS -- only the music channels
    for (int off : {W_LOOP_CNT, W_NOTE_DELAY, W_NOTE_SPEED})
      std::memset(ram + off, 1, 4);
    r(W_MUSIC_TEMPO) = 1;
    r(W_STEREO_PANNING) = 0xFF;

    apu->write(0xFF24, 0x00);
    apu->write(0xFF10, 0x08);   // AUD1SWEEP_DOWN -- sweep off
    apu->write(0xFF25, 0x00);
    apu->write(0xFF1A, 0x00);
    apu->write(0xFF1A, 0x80);
    apu->write(0xFF24, 0x77);
  } else {
    // ---- .playSfx: only the channels this header names are reset
    const uint16_t hdr = static_cast<uint16_t>(0x4000 + id * 3);
    const int count = ((romByte(hdr) & 0xC0) >> 6) + 1;
    for (int k = 0; k < count; ++k) {
      const int e = romByte(static_cast<uint16_t>(hdr + k * 3)) & 0x0F;
      setPtr(W_RET_ADDRS, e, 0);
      setPtr(W_CMD_PTRS, e, 0);
      for (int off : {W_SOUND_IDS, W_FLAGS1, W_DUTY, W_DUTY_PAT, W_VIB_DELAY, W_VIB_EXTENT,
                      W_VIB_RATE, W_FREQ_LO, W_VIB_DELAY_RELOAD, W_PS_LEN_MOD, W_PS_STEP,
                      W_PS_STEP_FRAC, W_PS_CUR_FRAC, W_PS_CUR_HI, W_PS_CUR_LO, W_PS_TGT_HI,
                      W_PS_TGT_LO, W_FLAGS2})
        ram[off + e] = 0;
      for (int off : {W_LOOP_CNT, W_NOTE_DELAY, W_NOTE_SPEED})
        ram[off + e] = 1;
      if (e == CHAN5)
        apu->write(0xFF10, 0x08); // sweep off
    }
  }

  // ---- .playSoundCommon: point each channel at its stream
  const uint16_t hdr = static_cast<uint16_t>(0x4000 + r(W_SOUND_ID) * 3);
  const int count = ((romByte(hdr) & 0xC0) >> 6) + 1;
  for (int k = 0; k < count; ++k) {
    const uint16_t e = static_cast<uint16_t>(hdr + k * 3);
    const int c = romByte(e) & 0x0F;
    ram[W_SOUND_IDS + c] = r(W_SOUND_ID);
    if (c >= CHAN4)
      ram[W_FLAGS1 + c] |= (1 << BIT_NOISE_OR_SFX);
    setPtr(W_CMD_PTRS, c,
           static_cast<uint16_t>(romByte(static_cast<uint16_t>(e + 1)) |
                                 (romByte(static_cast<uint16_t>(e + 2)) << 8)));
  }
}

// ---------------------------------------------------------------------------- UpdateMusic

void Gen1SoundEngine::updateFrame()
{
  for (int c = CHAN1; c <= CHAN8; ++c) {
    if (ram[W_SOUND_IDS + c] == 0)
      continue;
    if (c < CHAN5 && r(W_MUTE) != 0) {
      if (r(W_MUTE) & 0x80)
        continue;
      r(W_MUTE) |= 0x80;
      apu->write(0xFF25, 0);
      apu->write(0xFF1A, 0);
      apu->write(0xFF1A, 0x80);
      continue;
    }
    applyMusicAffects(c);
  }
}

void Gen1SoundEngine::applyMusicAffects(int c)
{
  uint8_t& delay = ram[W_NOTE_DELAY + c];
  if (delay == 1) {
    playNextNote(c);
    return;
  }
  --delay;

  // A music channel whose SFX counterpart is busy keeps COUNTING but stops writing registers --
  // which is why the music resumes exactly where it would have been when the SFX ends.
  if (c < CHAN5 && ram[W_SOUND_IDS + CHAN5 + c] != 0)
    return;

  if (ram[W_FLAGS1 + c] & (1 << BIT_ROTATE_DUTY_CYCLE))
    applyDutyCyclePattern(c);

  if (!(ram[W_FLAGS2 + c] & (1 << BIT_EXECUTE_MUSIC)) &&
      (ram[W_FLAGS1 + c] & (1 << BIT_NOISE_OR_SFX)))
    return;

  if (ram[W_FLAGS1 + c] & (1 << BIT_PITCH_SLIDE_ON)) {
    applyPitchSlide(c);
    return;
  }

  if (ram[W_VIB_DELAY + c] != 0) {
    --ram[W_VIB_DELAY + c];
    return;
  }

  const uint8_t extent = ram[W_VIB_EXTENT + c];
  if (extent == 0)
    return;

  uint8_t& rate = ram[W_VIB_RATE + c];
  if ((rate & 0x0F) != 0) {
    --rate;
    return;
  }
  rate = static_cast<uint8_t>(rate | (rate >> 4));   // reload the counter from the rate

  const uint8_t e = ram[W_FREQ_LO + c];
  uint8_t a;
  if (ram[W_FLAGS1 + c] & (1 << BIT_VIBRATO_DIRECTION)) {
    ram[W_FLAGS1 + c] &= static_cast<uint8_t>(~(1 << BIT_VIBRATO_DIRECTION));
    const uint8_t d = extent & 0x0F;             // the extent BELOW the note
    a = (e < d) ? 0 : static_cast<uint8_t>(e - d);
  } else {
    ram[W_FLAGS1 + c] |= (1 << BIT_VIBRATO_DIRECTION);
    const uint8_t d = static_cast<uint8_t>((extent & 0xF0) >> 4);  // and ABOVE it
    const int sum = e + d;
    a = (sum > 0xFF) ? 0xFF : static_cast<uint8_t>(sum);
  }
  writeReg(c, REG_FREQUENCY_LO, a);
}

void Gen1SoundEngine::playNextNote(int c)
{
  ram[W_VIB_DELAY + c] = ram[W_VIB_DELAY_RELOAD + c];
  ram[W_FLAGS1 + c] &= static_cast<uint8_t>(
      ~((1 << BIT_PITCH_SLIDE_ON) | (1 << BIT_PITCH_SLIDE_DECREASING)));
  runCommands(c);
}

uint8_t Gen1SoundEngine::nextMusicByte(int c)
{
  const uint16_t p = ptr(W_CMD_PTRS, c);
  const uint8_t v = romByte(p);
  setPtr(W_CMD_PTRS, c, static_cast<uint16_t>(p + 1));
  return v;
}

// ------------------------------------------------------------------ the command dispatcher

void Gen1SoundEngine::runCommands(int c)
{
  // Guard against a stream with no notes in it (a glitch id can be a pure loop). The console would
  // simply hang here; we refuse to.
  for (int guard = 0; guard < 4096; ++guard) {
    const uint8_t d = nextMusicByte(c);
    const uint8_t hi = d & 0xF0;

    // ---- sound_ret ($FF)
    if (d == 0xFF) {
      if (ram[W_FLAGS1 + c] & (1 << BIT_SOUND_CALL)) {
        ram[W_FLAGS1 + c] &= static_cast<uint8_t>(~(1 << BIT_SOUND_CALL));
        setPtr(W_CMD_PTRS, c, ptr(W_RET_ADDRS, c));
        continue;
      }
      if (c >= CHAN4) {
        ram[W_FLAGS1 + c] &= static_cast<uint8_t>(~(1 << BIT_NOISE_OR_SFX));
        ram[W_FLAGS2 + c] &= static_cast<uint8_t>(~(1 << BIT_EXECUTE_MUSIC));
        if (c == CHAN7) {
          apu->write(0xFF1A, 0x00);   // restart hardware channel 3
          apu->write(0xFF1A, 0x80);
          if (r(W_DISABLE_OUT) != 0) {
            r(W_DISABLE_OUT) = 0;
            disableChannelOutput(c);
          }
        }
      } else {
        disableChannelOutput(c);
      }
      ram[W_SOUND_IDS + c] = 0;
      return;
    }

    // ---- sound_call ($FD)
    if (d == 0xFD) {
      const uint8_t lo = nextMusicByte(c);
      const uint8_t hib = nextMusicByte(c);
      setPtr(W_RET_ADDRS, c, ptr(W_CMD_PTRS, c));
      setPtr(W_CMD_PTRS, c, static_cast<uint16_t>(lo | (hib << 8)));
      ram[W_FLAGS1 + c] |= (1 << BIT_SOUND_CALL);
      continue;
    }

    // ---- sound_loop ($FE)
    if (d == 0xFE) {
      const uint8_t count = nextMusicByte(c);
      if (count != 0) {
        if (ram[W_LOOP_CNT + c] == count) {
          ram[W_LOOP_CNT + c] = 1;
          nextMusicByte(c);   // skip the pointer
          nextMusicByte(c);
          continue;
        }
        ++ram[W_LOOP_CNT + c];
      }
      const uint8_t lo = nextMusicByte(c);
      const uint8_t hib = nextMusicByte(c);
      setPtr(W_CMD_PTRS, c, static_cast<uint16_t>(lo | (hib << 8)));
      continue;
    }

    // ---- note_type / drum_speed ($Dx)
    if (hi == 0xD0) {
      ram[W_NOTE_SPEED + c] = d & 0x0F;
      if (c == CHAN4)
        continue;                     // drum_speed: no parameter byte
      uint8_t p = nextMusicByte(c);
      if (c == CHAN3 || c == CHAN7) {
        ram[(c == CHAN3) ? W_MUSIC_WAVE : W_SFX_WAVE] = p & 0x0F;
        p = static_cast<uint8_t>((p & 0x30) << 1);   // channel 3 has no envelope, only a level
      }
      ram[W_VOLUME + c] = p;
      continue;
    }

    if (d == 0xE8) {  // toggle_perfect_pitch
      ram[W_FLAGS1 + c] ^= (1 << BIT_PERFECT_PITCH);
      continue;
    }

    if (d == 0xEA) {  // vibrato
      const uint8_t delay = nextMusicByte(c);
      ram[W_VIB_DELAY + c] = delay;
      ram[W_VIB_DELAY_RELOAD + c] = delay;
      const uint8_t p = nextMusicByte(c);
      const uint8_t n = static_cast<uint8_t>((p & 0xF0) >> 4);
      // Deliberately asymmetric: (n/2 + n%2) above the note, (n/2) below.
      const uint8_t above = static_cast<uint8_t>((n >> 1) + (n & 1));
      const uint8_t below = static_cast<uint8_t>(n >> 1);
      ram[W_VIB_EXTENT + c] = static_cast<uint8_t>((above << 4) | below);
      const uint8_t rate = p & 0x0F;
      ram[W_VIB_RATE + c] = static_cast<uint8_t>((rate << 4) | rate);
      continue;
    }

    if (d == 0xEB) {  // pitch_slide
      ram[W_PS_LEN_MOD + c] = nextMusicByte(c);
      const uint8_t p = nextMusicByte(c);
      const uint16_t de = calculateFrequency(p & 0x0F, static_cast<uint8_t>((p & 0xF0) >> 4));
      ram[W_PS_TGT_HI + c] = static_cast<uint8_t>(de >> 8);
      ram[W_PS_TGT_LO + c] = static_cast<uint8_t>(de & 0xFF);
      ram[W_FLAGS1 + c] |= (1 << BIT_PITCH_SLIDE_ON);
      noteLength(c, nextMusicByte(c));   // the next byte is a note
      return;
    }

    if (d == 0xEC) {  // duty_cycle
      ram[W_DUTY + c] = static_cast<uint8_t>((nextMusicByte(c) & 0x03) << 6);
      continue;
    }

    if (d == 0xED) {  // tempo
      const uint8_t a = nextMusicByte(c);
      const uint8_t b = nextMusicByte(c);
      if (c >= CHAN5) {
        r(W_SFX_TEMPO) = a;
        r(W_SFX_TEMPO + 1) = b;
        std::memset(ram + W_NOTE_DELAY_FRAC + 4, 0, 4);
      } else {
        r(W_MUSIC_TEMPO) = a;
        r(W_MUSIC_TEMPO + 1) = b;
        std::memset(ram + W_NOTE_DELAY_FRAC, 0, 4);
      }
      continue;
    }

    if (d == 0xEE) {  // stereo_panning
      r(W_STEREO_PANNING) = nextMusicByte(c);
      continue;
    }

    if (d == 0xEF) {  // unknownmusic0xef -- the game never uses it
      const uint8_t s = nextMusicByte(c);
      playSound(s);
      if (r(W_DISABLE_OUT) == 0) {
        r(W_DISABLE_OUT) = ram[W_SOUND_IDS + CHAN8];
        ram[W_SOUND_IDS + CHAN8] = 0;
      }
      continue;
    }

    if (d == 0xFC) {  // duty_cycle_pattern
      const uint8_t p = nextMusicByte(c);
      ram[W_DUTY_PAT + c] = p;
      ram[W_DUTY + c] = p & 0xC0;
      ram[W_FLAGS1 + c] |= (1 << BIT_ROTATE_DUTY_CYCLE);
      continue;
    }

    if (d == 0xF0) {  // volume -- straight into the master volume register
      apu->write(0xFF24, nextMusicByte(c));
      continue;
    }

    if (d == 0xF8) {  // execute_music
      ram[W_FLAGS2 + c] |= (1 << BIT_EXECUTE_MUSIC);
      continue;
    }

    if (hi == 0xE0) {  // octave (whatever is left of $Ex)
      ram[W_OCTAVE + c] = d & 0x0F;
      continue;
    }

    // ---- sfx_note ($2x) -- only on CHAN4..CHAN8, and only when not executing music
    if (hi == 0x20 && c >= CHAN4 && !(ram[W_FLAGS2 + c] & (1 << BIT_EXECUTE_MUSIC))) {
      noteLength(c, d);
      uint8_t v = static_cast<uint8_t>(ram[W_NOTE_DELAY + c] | ram[W_DUTY + c]);
      writeReg(c, REG_DUTY_SOUND_LEN, v);
      writeReg(c, REG_VOLUME_ENVELOPE, nextMusicByte(c));
      const uint8_t lo = nextMusicByte(c);
      uint8_t hib = 0;
      if (c != CHAN8)   // the noise channel has a single frequency register, so one byte less
        hib = nextMusicByte(c);
      const uint16_t de = static_cast<uint16_t>(lo | (hib << 8));
      applyDutyCycleAndSoundLength(c);
      enableChannelOutput(c);
      applyWavePatternAndFrequency(c, de);
      return;
    }

    // ---- pitch_sweep ($10) -- SFX channels only
    if (d == 0x10 && c >= CHAN5 && !(ram[W_FLAGS2 + c] & (1 << BIT_EXECUTE_MUSIC))) {
      apu->write(0xFF10, nextMusicByte(c));
      continue;
    }

    // ---- drum_note ($Bx) on the music noise channel: it PLAYS AN SFX (a noise instrument)
    if (c == CHAN4 && hi == 0xB0) {
      const uint8_t instrument = nextMusicByte(c);
      if (r(W_DISABLE_OUT) == 0) {
        const uint8_t savedId = r(W_SOUND_ID);
        playSound(instrument);
        r(W_SOUND_ID) = savedId;
      }
      noteLength(c, d);
      return;
    }
    if (c == CHAN4 && hi < 0xB0 && hi != 0x20) {
      // The 1-byte drum: instrument in the high nibble, length - 1 in the low. Unused by the game.
      const uint8_t instrument = static_cast<uint8_t>(hi >> 4);
      if (r(W_DISABLE_OUT) == 0) {
        const uint8_t savedId = r(W_SOUND_ID);
        playSound(instrument);
        r(W_SOUND_ID) = savedId;
      }
      noteLength(c, static_cast<uint8_t>(d & 0x0F));
      return;
    }

    // ---- anything else is a note or a rest
    noteLength(c, d);
    return;
  }
}

// ------------------------------------------------------------------------------ note_length

void Gen1SoundEngine::noteLength(int c, uint8_t d)
{
  const uint8_t length = static_cast<uint8_t>((d & 0x0F) + 1);   // in 16ths
  uint16_t hl = multiplyAdd(ram[W_NOTE_SPEED + c], length, 0);   // length x speed

  uint16_t tempo;
  if (c >= CHAN5) {
    if (c == CHAN8) {
      tempo = 0x0100;
    } else {
      // SetSfxTempo: a cry gets the cry's tempo modifier; everything else gets $0100.
      if (isCry()) {
        const int t = r(W_TEMPO_MOD) + 0x80;
        r(W_SFX_TEMPO + 1) = static_cast<uint8_t>(t & 0xFF);
        r(W_SFX_TEMPO) = static_cast<uint8_t>(t > 0xFF ? 1 : 0);
      } else {
        r(W_SFX_TEMPO + 1) = 0;
        r(W_SFX_TEMPO) = 1;
      }
      tempo = static_cast<uint16_t>((r(W_SFX_TEMPO) << 8) | r(W_SFX_TEMPO + 1));
    }
  } else {
    tempo = static_cast<uint16_t>((r(W_MUSIC_TEMPO) << 8) | r(W_MUSIC_TEMPO + 1));
  }

  // The delay is 16-bit fixed point: 8 bits of whole frames, 8 bits of fraction carried forward.
  const uint16_t de = multiplyAdd(static_cast<uint8_t>(hl & 0xFF), tempo,
                                  ram[W_NOTE_DELAY_FRAC + c]);
  ram[W_NOTE_DELAY_FRAC + c] = static_cast<uint8_t>(de & 0xFF);
  ram[W_NOTE_DELAY + c] = static_cast<uint8_t>(de >> 8);

  if (!(ram[W_FLAGS2 + c] & (1 << BIT_EXECUTE_MUSIC)) &&
      (ram[W_FLAGS1 + c] & (1 << BIT_NOISE_OR_SFX)))
    return;   // an SFX/noise channel: the length was all it wanted

  notePitch(c, d);
}

void Gen1SoundEngine::notePitch(int c, uint8_t d)
{
  const uint8_t hi = d & 0xF0;

  if (hi == 0xC0) {   // ---- rest
    if (c < CHAN5 && ram[W_SOUND_IDS + CHAN5 + c] != 0)
      return;
    if (c == CHAN3 || c == CHAN7) {
      disableChannelOutput(c);
      return;
    }
    writeReg(c, REG_VOLUME_ENVELOPE, 0x08);   // fade the sound out
    writeReg(c, REG_FREQUENCY_LO + 1, 0x80);  // and restart it (into silence)
    return;
  }

  const uint8_t note = static_cast<uint8_t>(hi >> 4);
  uint16_t de = calculateFrequency(note, ram[W_OCTAVE + c]);

  if (ram[W_FLAGS1 + c] & (1 << BIT_PITCH_SLIDE_ON))
    initPitchSlideVars(c, de);

  if (c < CHAN5 && ram[W_SOUND_IDS + CHAN5 + c] != 0)
    return;   // an SFX has this hardware channel: stay quiet, but keep counting

  writeReg(c, REG_VOLUME_ENVELOPE, ram[W_VOLUME + c]);
  applyDutyCycleAndSoundLength(c);
  enableChannelOutput(c);

  uint8_t e = static_cast<uint8_t>(de & 0xFF);
  if (ram[W_FLAGS1 + c] & (1 << BIT_PERFECT_PITCH)) {
    ++e;
    // (The original also means to carry into the high byte, but `inc e` doesn't set carry, so it
    // never does. pokered calls it "likely a mistake". We keep the mistake.)
  }
  ram[W_FREQ_LO + c] = e;
  de = static_cast<uint16_t>((de & 0xFF00) | e);
  applyWavePatternAndFrequency(c, de);
}

uint16_t Gen1SoundEngine::calculateFrequency(uint8_t note, uint8_t octave) const
{
  int16_t hl = static_cast<int16_t>(PITCHES[note & 0x0F ? (note % 12) : 0]);
  hl = static_cast<int16_t>(PITCHES[note % 12]);

  uint8_t a = octave;
  while (a != 7) {          // shift right (octave - 1) times -- see PITCHES
    hl = static_cast<int16_t>(hl >> 1);   // arithmetic: the value is negative
    ++a;
  }
  uint8_t d = static_cast<uint8_t>((static_cast<uint16_t>(hl) >> 8) & 0xFF);
  const uint8_t e = static_cast<uint8_t>(static_cast<uint16_t>(hl) & 0xFF);
  d = static_cast<uint8_t>(d + 8);   // +$800 = +2048: the table stores x - 2048
  return static_cast<uint16_t>((d << 8) | e);
}

// -------------------------------------------------------------------------------- registers

void Gen1SoundEngine::applyDutyCycleAndSoundLength(int c)
{
  uint8_t d = ram[W_NOTE_DELAY + c];   // the note delay doubles as the sound length
  if (c != CHAN3 && c != CHAN7)
    d = static_cast<uint8_t>((d & 0x3F) | ram[W_DUTY + c]);
  writeReg(c, REG_DUTY_SOUND_LEN, d);
}

void Gen1SoundEngine::applyWavePatternAndFrequency(int c, uint16_t de)
{
  if (c == CHAN3 || c == CHAN7) {
    const int inst = ram[(c == CHAN3) ? W_MUSIC_WAVE : W_SFX_WAVE];
    // The wave-pointer table has 9 entries: waves 0-4, then .wave5 four times. And .wave5 has NO
    // DATA in the ROM -- it reads whatever follows the table, which differs in every bank. That is
    // what Lavender Town and Pokemon Tower are built on. (See the importer.)
    const int idx = (inst < 5) ? inst : (5 + curBank);
    apu->write(0xFF1A, 0x00);    // stop channel 3 while we swap its waveform
    for (int i = 0; i < 16; ++i) {
      const size_t o = static_cast<size_t>(idx) * 16 + static_cast<size_t>(i);
      apu->write(static_cast<uint16_t>(0xFF30 + i), o < waves.size() ? waves[o] : 0);
    }
    apu->write(0xFF1A, 0x80);
  }

  uint8_t d = static_cast<uint8_t>(de >> 8);
  const uint8_t e = static_cast<uint8_t>(de & 0xFF);
  d = static_cast<uint8_t>((d | 0x80) & 0xC7);
  // ⚠️ `and $c7` CLEARS bit 6 -- the length-enable. So the sound length written just above is
  // ignored by the hardware, and notes end because the ENGINE starts the next one. Do not "fix".
  writeReg(c, REG_FREQUENCY_LO, e);
  writeReg(c, REG_FREQUENCY_LO + 1, d);
}

void Gen1SoundEngine::enableChannelOutput(int c)
{
  const uint8_t term = apu->read(0xFF25);
  uint8_t d = static_cast<uint8_t>(term | HW_ENABLE[c]);

  const bool noSfxOnThisHardware = (c < CHAN5) && (ram[W_SOUND_IDS + CHAN5 + c] == 0);
  if (c == CHAN8 || noSfxOnThisHardware) {
    const uint8_t panned = static_cast<uint8_t>(r(W_STEREO_PANNING) & HW_ENABLE[c]);
    d = static_cast<uint8_t>((term & HW_DISABLE[c]) | panned);
  }
  apu->write(0xFF25, d);
}

void Gen1SoundEngine::disableChannelOutput(int c)
{
  apu->write(0xFF25, static_cast<uint8_t>(apu->read(0xFF25) & HW_DISABLE[c]));
}

void Gen1SoundEngine::applyDutyCyclePattern(int c)
{
  uint8_t p = ram[W_DUTY_PAT + c];
  p = static_cast<uint8_t>((p << 2) | (p >> 6));   // rlca rlca -- rotate the 4 duties round
  ram[W_DUTY_PAT + c] = p;
  const uint8_t d = p & 0xC0;
  const uint8_t cur = readReg(c, REG_DUTY_SOUND_LEN);
  writeReg(c, REG_DUTY_SOUND_LEN, static_cast<uint8_t>((cur & 0x3F) | d));
}

// ------------------------------------------------------------------------------ pitch slide

void Gen1SoundEngine::initPitchSlideVars(int c, uint16_t de)
{
  ram[W_PS_CUR_HI + c] = static_cast<uint8_t>(de >> 8);
  ram[W_PS_CUR_LO + c] = static_cast<uint8_t>(de & 0xFF);

  int lenMod = ram[W_NOTE_DELAY + c] - ram[W_PS_LEN_MOD + c];
  if (lenMod < 0) lenMod = 1;
  ram[W_PS_LEN_MOD + c] = static_cast<uint8_t>(lenMod);

  uint8_t e = static_cast<uint8_t>(de & 0xFF);
  uint8_t dHi = static_cast<uint8_t>(de >> 8);

  int lo = e - ram[W_PS_TGT_LO + c];
  const int borrow = (lo < 0) ? 1 : 0;
  e = static_cast<uint8_t>(lo & 0xFF);
  int hi = dHi - borrow - ram[W_PS_TGT_HI + c];

  uint8_t d;
  if (hi >= 0) {
    d = static_cast<uint8_t>(hi);
    ram[W_FLAGS1 + c] |= (1 << BIT_PITCH_SLIDE_DECREASING);
  } else {
    // The target is higher. Take target - current...
    dHi = ram[W_PS_CUR_HI + c];
    e = ram[W_PS_CUR_LO + c];
    int l2 = ram[W_PS_TGT_LO + c] - e;
    const int borrow2 = (l2 < 0) ? 1 : 0;
    e = static_cast<uint8_t>(l2 & 0xFF);

    // 🐞 BUG, faithfully kept (pokered marks it): it borrows from the CURRENT frequency's high
    // byte instead of the TARGET's, so the result is $200 too large when the low bytes cross.
    const uint8_t dTmp = static_cast<uint8_t>((dHi - borrow2) & 0xFF);
    d = static_cast<uint8_t>((ram[W_PS_TGT_HI + c] - dTmp) & 0xFF);

    ram[W_FLAGS1 + c] &= static_cast<uint8_t>(~(1 << BIT_PITCH_SLIDE_DECREASING));
  }

  // ...then divide the distance by the length, by repeated subtraction.
  const uint8_t divisor = ram[W_PS_LEN_MOD + c];
  int b = 0;
  int eWork = e;
  int dWork = d;
  for (;;) {
    ++b;
    eWork -= divisor;
    if (eWork >= 0) continue;
    if (dWork == 0) break;
    --dWork;
    eWork &= 0xFF;
  }
  const uint8_t remainder = static_cast<uint8_t>((eWork + divisor) & 0xFF);
  ram[W_PS_STEP + c] = static_cast<uint8_t>(b);
  ram[W_PS_STEP_FRAC + c] = remainder;
  ram[W_PS_CUR_FRAC + c] = remainder;
}

void Gen1SoundEngine::applyPitchSlide(int c)
{
  const bool decreasing = (ram[W_FLAGS1 + c] & (1 << BIT_PITCH_SLIDE_DECREASING)) != 0;

  int e = ram[W_PS_CUR_LO + c];
  int d = ram[W_PS_CUR_HI + c];
  const int step = ram[W_PS_STEP + c];

  if (!decreasing) {
    int v = ((d << 8) | e) + step;
    int frac = ram[W_PS_CUR_FRAC + c] + ram[W_PS_STEP_FRAC + c];
    if (frac > 0xFF) { frac &= 0xFF; ++v; }
    ram[W_PS_CUR_FRAC + c] = static_cast<uint8_t>(frac);
    e = v & 0xFF;
    d = (v >> 8) & 0xFF;

    if (ram[W_PS_TGT_HI + c] < d ||
        (ram[W_PS_TGT_HI + c] == d && ram[W_PS_TGT_LO + c] < e)) {
      ram[W_FLAGS1 + c] &= static_cast<uint8_t>(
          ~((1 << BIT_PITCH_SLIDE_ON) | (1 << BIT_PITCH_SLIDE_DECREASING)));
      return;
    }
  } else {
    int v = ((d << 8) | e) - step;
    int frac = ram[W_PS_STEP_FRAC + c] * 2;   // `add a` -- the original doubles it, then borrows
    ram[W_PS_STEP_FRAC + c] = static_cast<uint8_t>(frac & 0xFF);
    if (frac > 0xFF) --v;
    e = v & 0xFF;
    d = (v >> 8) & 0xFF;

    if (d < ram[W_PS_TGT_HI + c] ||
        (d == ram[W_PS_TGT_HI + c] && e < ram[W_PS_TGT_LO + c])) {
      ram[W_FLAGS1 + c] &= static_cast<uint8_t>(
          ~((1 << BIT_PITCH_SLIDE_ON) | (1 << BIT_PITCH_SLIDE_DECREASING)));
      return;
    }
  }

  ram[W_PS_CUR_LO + c] = static_cast<uint8_t>(e);
  ram[W_PS_CUR_HI + c] = static_cast<uint8_t>(d);
  writeReg(c, REG_FREQUENCY_LO, static_cast<uint8_t>(e));
  writeReg(c, REG_FREQUENCY_LO + 1, static_cast<uint8_t>(d));
}

bool Gen1SoundEngine::isCry() const
{
  const int id = ram[W_SOUND_IDS + CHAN5];
  return id >= CRY_SFX_START && id < CRY_SFX_END;
}

} // namespace pse::audio
