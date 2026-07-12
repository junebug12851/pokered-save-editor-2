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

namespace pse::audio {

/**
 * @brief The Game Boy's sound chip.
 *
 * A model of the DMG APU and nothing else: it knows about square waves, an LFSR and a wave table.
 * It has never heard of Pokémon, of notes, or of tracks. Everything above it (Gen1SoundEngine)
 * talks to it the only way the real game can -- by writing bytes into $FF10-$FF3F.
 *
 * That seam is the whole design. See notes/reference/gameboy-apu.md.
 *
 * Usage:
 * @code
 *   apu.write(0xFF11, 0x80);   // exactly what the game writes, when it writes it
 *   apu.tick(70224);           // advance the hardware by one Game Boy frame
 *   apu.render(buf, frames);   // 48 kHz stereo int16 out
 * @endcode
 */
class AUDIO_AUTOPORT GbApu
{
public:
  static constexpr int CPU_HZ = 4194304;    ///< The Game Boy's clock.
  static constexpr int SAMPLE_HZ = 48000;   ///< What we hand the sound card.
  static constexpr int CYCLES_PER_FRAME = 70224; ///< One LCD frame -- the engine's heartbeat.

  GbApu();

  void reset();                         ///< Power-cycle the chip.
  void write(uint16_t addr, uint8_t v); ///< Write an APU register ($FF10-$FF3F).
  [[nodiscard]] uint8_t read(uint16_t addr) const; ///< Read one back, with the hardware's masks.

  void tick(int cycles);                ///< Advance the hardware by @p cycles CPU cycles.

  /// Pull up to @p frames stereo samples. Returns how many were actually available.
  int render(int16_t* out, int frames);

  [[nodiscard]] int available() const { return static_cast<int>(fifo.size() / 2); }
  void clearAudio() { fifo.clear(); }

private:
  struct Pulse {
    uint8_t duty = 0, lengthLoad = 0, envVol = 0, envDir = 0, envPace = 0;
    uint16_t period = 0;
    bool lengthEnable = false, enabled = false, dacOn = false;
    int timer = 0, dutyPos = 0, volume = 0, envTimer = 0, lengthTimer = 0;
    // channel 1 only
    uint8_t sweepPace = 0, sweepDir = 0, sweepShift = 0;
    int sweepTimer = 0;
    uint16_t sweepShadow = 0;
    bool sweepOn = false;
  };

  struct Wave {
    bool dacOn = false, enabled = false, lengthEnable = false;
    uint8_t level = 0;
    uint16_t period = 0;
    int timer = 0, pos = 0, lengthTimer = 0;
    uint8_t ram[16] = {};
  };

  struct Noise {
    uint8_t envVol = 0, envDir = 0, envPace = 0, shift = 0, divisor = 0;
    bool width7 = false, lengthEnable = false, enabled = false, dacOn = false;
    int timer = 0, volume = 0, envTimer = 0, lengthTimer = 0;
    uint16_t lfsr = 0x7FFF;
  };

  void trigger1(); void trigger2(); void trigger3(); void trigger4();
  void stepFrameSequencer();
  void stepLength();
  void stepEnvelopes();
  void stepSweep();
  uint16_t sweepCalc(bool& overflow);
  void mixSample();

  uint8_t regs[0x30] = {};   // $FF10-$FF3F, as written
  Pulse ch1, ch2;
  Wave  ch3;
  Noise ch4;

  bool powered = true;
  int fsCounter = 0;   // counts CPU cycles up to 8192 (512 Hz)
  int fsStep = 0;

  // Box-averaged downsample: sum every APU output between sample points. Naive decimation
  // aliases audibly on the 12.5% duty and on the noise channel.
  int sampleAcc = 0;      // cycles since the last emitted sample (x1000, fixed point)
  long long accL = 0, accR = 0;
  int accN = 0;

  std::vector<int16_t> fifo; // interleaved L,R
};

} // namespace pse::audio
