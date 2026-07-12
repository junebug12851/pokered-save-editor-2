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
#pragma once

#include <cstdint>
#include <vector>

#include "./audio_autoport.h"
#include "./gbapu.h"

namespace pse::audio {

/**
 * @brief Gen 1's sound engine, ported line for line.
 *
 * This is `audio/engine_1.asm` from pret/pokered, transliterated -- **not** reinterpreted. Its
 * entire state is @ref ram, a 256-byte block laid out exactly like the game's own audio RAM at
 * `$C000`, so it can be compared with the console's byte for byte, frame by frame. That is the
 * whole point: it makes the port *checkable*.
 *
 * **Transliterate, don't interpret.** The disassembly is full of load-bearing oddities -- the pitch
 * table's signed negatives, the length-enable bit cleared by `and $c7`, the pitch-slide borrow bug,
 * the dead branch in perfect-pitch. They are not mistakes to fix. They are the sound.
 *
 * `engine_1.asm` and `engine_3.asm` are byte-for-byte identical; `engine_2.asm` (bank 8, battle) is
 * the same plus a low-health alarm and cry modifiers. So this is one class, not three.
 *
 * See notes/reference/gen1-sound-engine.md.
 */
class AUDIO_AUTOPORT Gen1SoundEngine
{
public:
  // ---- the audio RAM map ($C000). Offsets, not addresses -- see gen1-sound-engine.md §5.
  enum : int {
    W_UNUSED = 0x00, W_SOUND_ID = 0x01, W_MUTE = 0x02, W_DISABLE_OUT = 0x03,
    W_STEREO_PANNING = 0x04, W_SAVED_VOLUME = 0x05,
    W_CMD_PTRS = 0x06, W_RET_ADDRS = 0x16,
    W_SOUND_IDS = 0x26, W_FLAGS1 = 0x2E, W_FLAGS2 = 0x36,
    W_DUTY = 0x3E, W_DUTY_PAT = 0x46,
    W_VIB_DELAY = 0x4E, W_VIB_EXTENT = 0x56, W_VIB_RATE = 0x5E,
    W_FREQ_LO = 0x66, W_VIB_DELAY_RELOAD = 0x6E,
    W_PS_LEN_MOD = 0x76, W_PS_STEP = 0x7E, W_PS_STEP_FRAC = 0x86,
    W_PS_CUR_FRAC = 0x8E, W_PS_CUR_HI = 0x96, W_PS_CUR_LO = 0x9E,
    W_PS_TGT_HI = 0xA6, W_PS_TGT_LO = 0xAE,
    W_NOTE_DELAY = 0xB6, W_LOOP_CNT = 0xBE, W_NOTE_SPEED = 0xC6, W_NOTE_DELAY_FRAC = 0xCE,
    W_OCTAVE = 0xD6, W_VOLUME = 0xDE,
    W_MUSIC_WAVE = 0xE6, W_SFX_WAVE = 0xE7,
    W_MUSIC_TEMPO = 0xE8, W_SFX_TEMPO = 0xEA, W_SFX_HDR_PTR = 0xEC,
    W_NEW_SOUND_ID = 0xEE, W_AUDIO_ROM_BANK = 0xEF, W_AUDIO_SAVED_BANK = 0xF0,
    W_FREQ_MOD = 0xF1, W_TEMPO_MOD = 0xF2,
  };

  enum : uint8_t { SFX_STOP_ALL_MUSIC = 0xFF };
  enum : int { CRY_SFX_START = 20, CRY_SFX_END = 134 };

  /// One audio bank's data: the header table at $4000 plus every stream it can reach.
  struct Bank {
    std::vector<uint8_t> data;   ///< Starts at $4000.
    int maxSfxId = 0;            ///< MAX_SFX_ID_N -- above this, PlaySound takes the music path.
  };

  explicit Gen1SoundEngine(GbApu* apu);

  /// Hand it the imported data: three banks (2, 8, 31) and the 8 wave instruments.
  void setData(const Bank& bank2, const Bank& bank8, const Bank& bank31,
               const std::vector<uint8_t>& waves);

  void stopAll();                          ///< SFX_STOP_ALL_MUSIC.
  void playMusic(uint8_t bank, uint8_t id); ///< PlayMusic: pick the bank, then the sound.
  void updateFrame();                      ///< AudioN_UpdateMusic -- call once per VBlank.

  [[nodiscard]] bool isPlaying() const;

  /// The engine's whole mind: 256 bytes, laid out exactly like the console's $C000-$C0FF.
  /// This is what the parity test compares.
  uint8_t ram[0x100] = {};

private:
  // ---- ram helpers
  uint8_t& r(int off) { return ram[off]; }
  [[nodiscard]] uint8_t r(int off) const { return ram[off]; }
  [[nodiscard]] uint16_t ptr(int base, int c) const {
    return static_cast<uint16_t>(ram[base + c * 2] | (ram[base + c * 2 + 1] << 8));
  }
  void setPtr(int base, int c, uint16_t v) {
    ram[base + c * 2] = v & 0xFF;
    ram[base + c * 2 + 1] = (v >> 8) & 0xFF;
  }

  // ---- the current bank's bytes
  [[nodiscard]] const Bank* bank() const;
  [[nodiscard]] uint8_t romByte(uint16_t addr) const;

  // ---- the port
  void playSound(uint8_t id);
  void applyMusicAffects(int c);
  void playNextNote(int c);
  void runCommands(int c);            // Audio1_sound_ret -- the command dispatcher
  void noteLength(int c, uint8_t d);  // Audio1_note_length
  void notePitch(int c, uint8_t d);   // Audio1_note_pitch
  uint8_t nextMusicByte(int c);
  uint16_t calculateFrequency(uint8_t note, uint8_t octave) const;
  /// Returns the CLOBBERED de -- the caller uses it as the frequency. See the .cpp.
  uint16_t initPitchSlideVars(int c, uint16_t de);
  void applyPitchSlide(int c);
  void applyDutyCyclePattern(int c);
  void applyDutyCycleAndSoundLength(int c);
  void applyWavePatternAndFrequency(int c, uint16_t de);
  void enableChannelOutput(int c);
  void disableChannelOutput(int c);
  [[nodiscard]] bool isCry() const;

  void writeReg(int c, int reg, uint8_t v);
  [[nodiscard]] uint8_t readReg(int c, int reg) const;

  GbApu* apu;
  Bank banks[3];                 // 2, 8, 31
  std::vector<uint8_t> waves;    // 5 base waves + one .wave5 per bank (see the importer)
  int curBank = 0;               // index into banks[]
};

} // namespace pse::audio
