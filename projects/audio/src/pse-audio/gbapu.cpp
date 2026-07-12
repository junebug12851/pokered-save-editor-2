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
 * @file gbapu.cpp
 * @brief The DMG sound chip. See gbapu.h, and notes/reference/gameboy-apu.md.
 */
#include "./gbapu.h"

#include <algorithm>
#include <cstring>

namespace pse::audio {

namespace {

// Duty patterns, one bit per step of the 8-step duty pointer.
constexpr uint8_t DUTY[4] = {0b00000001, 0b10000001, 0b10000111, 0b01111110};

// The noise channel's divisor code -> divisor. Code 0 is 8, NOT 0.
constexpr int NOISE_DIVISOR[8] = {8, 16, 32, 48, 64, 80, 96, 112};

// What each register gives back when you READ it. The game DOES read some of these back
// (rAUDTERM, rAUDVOL, and NRx1 in the duty-rotation code), so these masks are not cosmetic:
// NR11 reads its length bits as 1s, which is why rotating the duty cycle also writes a
// sound length of 63. That is the hardware's behaviour, so it is ours.
constexpr uint8_t READ_MASK[0x30] = {
    0x80, 0x3F, 0x00, 0xFF, 0xBF,        // FF10-FF14
    0xFF, 0x3F, 0x00, 0xFF, 0xBF,        // FF15-FF19
    0x7F, 0xFF, 0x9F, 0xFF, 0xBF,        // FF1A-FF1E
    0xFF, 0xFF, 0x00, 0x00, 0xBF,        // FF1F-FF23
    0x00, 0x00, 0x70,                    // FF24-FF26
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // FF27-FF2F (unused)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       // FF30-FF37 wave RAM
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       // FF38-FF3F
};

} // namespace

GbApu::GbApu()
{
  fifo.reserve(SAMPLE_HZ);  // a second of headroom
  reset();
}

void GbApu::reset()
{
  std::memset(regs, 0, sizeof(regs));
  ch1 = Pulse{};
  ch2 = Pulse{};
  ch3 = Wave{};
  ch4 = Noise{};
  powered = true;
  fsCounter = fsStep = 0;
  sampleAcc = 0;
  accL = accR = 0;
  accN = 0;
  fifo.clear();
}

uint8_t GbApu::read(uint16_t addr) const
{
  if (addr < 0xFF10 || addr > 0xFF3F)
    return 0xFF;
  const int i = addr - 0xFF10;

  if (addr == 0xFF26) { // NR52 -- the only register with live status bits
    uint8_t v = powered ? 0x80 : 0x00;
    if (ch1.enabled) v |= 0x01;
    if (ch2.enabled) v |= 0x02;
    if (ch3.enabled) v |= 0x04;
    if (ch4.enabled) v |= 0x08;
    return v | READ_MASK[i];
  }
  return static_cast<uint8_t>(regs[i] | READ_MASK[i]);
}

void GbApu::write(uint16_t addr, uint8_t v)
{
  if (addr < 0xFF10 || addr > 0xFF3F)
    return;
  const int i = addr - 0xFF10;

  // NR52 bit 7 is the master switch. With the APU off, everything else ignores writes.
  if (addr == 0xFF26) {
    const bool on = (v & 0x80) != 0;
    if (!on && powered) {
      const uint8_t wave[16] = {}; // wave RAM survives a power-off on DMG; the rest doesn't
      (void)wave;
      uint8_t keep[16];
      std::memcpy(keep, ch3.ram, 16);
      reset();
      std::memcpy(ch3.ram, keep, 16);
      powered = false;
    }
    powered = on;
    regs[i] = v & 0x80;
    return;
  }
  if (!powered && addr < 0xFF30)
    return;

  regs[i] = v;

  switch (addr) {
    // ---- channel 1 ------------------------------------------------------------------
    case 0xFF10:
      ch1.sweepPace = (v >> 4) & 0x07;
      ch1.sweepDir = (v >> 3) & 0x01;
      ch1.sweepShift = v & 0x07;
      break;
    case 0xFF11:
      ch1.duty = (v >> 6) & 0x03;
      ch1.lengthLoad = v & 0x3F;
      ch1.lengthTimer = 64 - ch1.lengthLoad;
      break;
    case 0xFF12:
      ch1.envVol = (v >> 4) & 0x0F;
      ch1.envDir = (v >> 3) & 0x01;
      ch1.envPace = v & 0x07;
      ch1.dacOn = (v & 0xF8) != 0;   // volume 0 + "decrease" == DAC off == the channel is dead
      if (!ch1.dacOn) ch1.enabled = false;
      break;
    case 0xFF13:
      ch1.period = static_cast<uint16_t>((ch1.period & 0x700) | v);
      break;
    case 0xFF14:
      ch1.period = static_cast<uint16_t>(((v & 0x07) << 8) | (ch1.period & 0xFF));
      ch1.lengthEnable = (v & 0x40) != 0;
      if (v & 0x80) trigger1();
      break;

    // ---- channel 2 ------------------------------------------------------------------
    case 0xFF16:
      ch2.duty = (v >> 6) & 0x03;
      ch2.lengthLoad = v & 0x3F;
      ch2.lengthTimer = 64 - ch2.lengthLoad;
      break;
    case 0xFF17:
      ch2.envVol = (v >> 4) & 0x0F;
      ch2.envDir = (v >> 3) & 0x01;
      ch2.envPace = v & 0x07;
      ch2.dacOn = (v & 0xF8) != 0;
      if (!ch2.dacOn) ch2.enabled = false;
      break;
    case 0xFF18:
      ch2.period = static_cast<uint16_t>((ch2.period & 0x700) | v);
      break;
    case 0xFF19:
      ch2.period = static_cast<uint16_t>(((v & 0x07) << 8) | (ch2.period & 0xFF));
      ch2.lengthEnable = (v & 0x40) != 0;
      if (v & 0x80) trigger2();
      break;

    // ---- channel 3 (wave) -----------------------------------------------------------
    case 0xFF1A:
      ch3.dacOn = (v & 0x80) != 0;
      if (!ch3.dacOn) ch3.enabled = false;
      break;
    case 0xFF1B:
      ch3.lengthTimer = 256 - v;
      break;
    case 0xFF1C:
      ch3.level = (v >> 5) & 0x03;
      break;
    case 0xFF1D:
      ch3.period = static_cast<uint16_t>((ch3.period & 0x700) | v);
      break;
    case 0xFF1E:
      ch3.period = static_cast<uint16_t>(((v & 0x07) << 8) | (ch3.period & 0xFF));
      ch3.lengthEnable = (v & 0x40) != 0;
      if (v & 0x80) trigger3();
      break;

    // ---- channel 4 (noise) ----------------------------------------------------------
    case 0xFF20:
      ch4.lengthTimer = 64 - (v & 0x3F);
      break;
    case 0xFF21:
      ch4.envVol = (v >> 4) & 0x0F;
      ch4.envDir = (v >> 3) & 0x01;
      ch4.envPace = v & 0x07;
      ch4.dacOn = (v & 0xF8) != 0;
      if (!ch4.dacOn) ch4.enabled = false;
      break;
    case 0xFF22:
      ch4.shift = (v >> 4) & 0x0F;
      ch4.width7 = (v & 0x08) != 0;
      ch4.divisor = v & 0x07;
      break;
    case 0xFF23:
      ch4.lengthEnable = (v & 0x40) != 0;
      if (v & 0x80) trigger4();
      break;

    default:
      if (addr >= 0xFF30 && addr <= 0xFF3F)
        ch3.ram[addr - 0xFF30] = v;
      break;
  }
}

void GbApu::trigger1()
{
  ch1.enabled = ch1.dacOn;
  ch1.timer = (2048 - ch1.period) * 4;
  ch1.volume = ch1.envVol;
  ch1.envTimer = ch1.envPace;
  if (ch1.lengthTimer == 0) ch1.lengthTimer = 64;
  ch1.sweepShadow = ch1.period;
  ch1.sweepTimer = ch1.sweepPace ? ch1.sweepPace : 8;
  ch1.sweepOn = ch1.sweepPace || ch1.sweepShift;
  if (ch1.sweepShift) { // the DMG's trigger-time overflow check
    bool of = false;
    sweepCalc(of);
    if (of) ch1.enabled = false;
  }
}

void GbApu::trigger2()
{
  ch2.enabled = ch2.dacOn;
  ch2.timer = (2048 - ch2.period) * 4;
  ch2.volume = ch2.envVol;
  ch2.envTimer = ch2.envPace;
  if (ch2.lengthTimer == 0) ch2.lengthTimer = 64;
}

void GbApu::trigger3()
{
  ch3.enabled = ch3.dacOn;
  ch3.timer = (2048 - ch3.period) * 2;
  ch3.pos = 0;
  if (ch3.lengthTimer == 0) ch3.lengthTimer = 256;
}

void GbApu::trigger4()
{
  ch4.enabled = ch4.dacOn;
  ch4.timer = NOISE_DIVISOR[ch4.divisor] << ch4.shift;
  ch4.volume = ch4.envVol;
  ch4.envTimer = ch4.envPace;
  ch4.lfsr = 0x7FFF;
  if (ch4.lengthTimer == 0) ch4.lengthTimer = 64;
}

uint16_t GbApu::sweepCalc(bool& overflow)
{
  const uint16_t delta = static_cast<uint16_t>(ch1.sweepShadow >> ch1.sweepShift);
  const int next = ch1.sweepDir ? (ch1.sweepShadow - delta) : (ch1.sweepShadow + delta);
  overflow = next > 2047;
  return static_cast<uint16_t>(next & 0x7FF);
}

void GbApu::stepLength()
{
  auto len = [](int& t, bool enable, bool& on) {
    if (enable && t > 0 && --t == 0) on = false;
  };
  len(ch1.lengthTimer, ch1.lengthEnable, ch1.enabled);
  len(ch2.lengthTimer, ch2.lengthEnable, ch2.enabled);
  len(ch3.lengthTimer, ch3.lengthEnable, ch3.enabled);
  len(ch4.lengthTimer, ch4.lengthEnable, ch4.enabled);
}

void GbApu::stepEnvelopes()
{
  auto env = [](int& timer, uint8_t pace, uint8_t dir, int& vol) {
    if (pace == 0) return;              // pace 0 == OFF, not "fastest"
    if (--timer > 0) return;
    timer = pace;
    if (dir && vol < 15) ++vol;
    else if (!dir && vol > 0) --vol;
  };
  env(ch1.envTimer, ch1.envPace, ch1.envDir, ch1.volume);
  env(ch2.envTimer, ch2.envPace, ch2.envDir, ch2.volume);
  env(ch4.envTimer, ch4.envPace, ch4.envDir, ch4.volume);
}

void GbApu::stepSweep()
{
  if (--ch1.sweepTimer > 0) return;
  ch1.sweepTimer = ch1.sweepPace ? ch1.sweepPace : 8;
  if (!ch1.sweepOn || ch1.sweepPace == 0) return;

  bool of = false;
  const uint16_t next = sweepCalc(of);
  if (of) { ch1.enabled = false; return; }
  if (ch1.sweepShift) {
    ch1.sweepShadow = next;
    ch1.period = next;
    bool of2 = false;
    sweepCalc(of2);
    if (of2) ch1.enabled = false;
  }
}

void GbApu::stepFrameSequencer()
{
  switch (fsStep) {
    case 0: stepLength(); break;
    case 2: stepLength(); stepSweep(); break;
    case 4: stepLength(); break;
    case 6: stepLength(); stepSweep(); break;
    case 7: stepEnvelopes(); break;
    default: break;
  }
  fsStep = (fsStep + 1) & 7;
}

void GbApu::mixSample()
{
  if (accN == 0) return;

  const uint8_t nr50 = regs[0x24 - 0x10];
  const int volL = ((nr50 >> 4) & 0x07) + 1;
  const int volR = (nr50 & 0x07) + 1;

  // accL/accR are already the per-side sums scaled by NR51 in tick(); average and scale.
  const int64_t l = (accL / accN) * volL;
  const int64_t r = (accR / accN) * volR;

  // 4 channels x 15 levels x 8 volume steps -> keep well clear of clipping.
  const auto clamp16 = [](int64_t x) -> int16_t {
    return static_cast<int16_t>(std::clamp<int64_t>(x, -32768, 32767));
  };
  fifo.push_back(clamp16(l * 46));
  fifo.push_back(clamp16(r * 46));

  accL = accR = 0;
  accN = 0;
}

void GbApu::tick(int cycles)
{
  // Step in 2-cycle units: that is the wave channel's clock, the fastest thing here.
  for (int c = 0; c < cycles; c += 2) {
    if (powered) {
      // ---- frequency timers
      ch1.timer -= 2;
      if (ch1.timer <= 0) {
        ch1.timer += (2048 - ch1.period) * 4;
        ch1.dutyPos = (ch1.dutyPos + 1) & 7;
      }
      ch2.timer -= 2;
      if (ch2.timer <= 0) {
        ch2.timer += (2048 - ch2.period) * 4;
        ch2.dutyPos = (ch2.dutyPos + 1) & 7;
      }
      ch3.timer -= 2;
      if (ch3.timer <= 0) {
        ch3.timer += (2048 - ch3.period) * 2;
        ch3.pos = (ch3.pos + 1) & 31;
      }
      ch4.timer -= 2;
      if (ch4.timer <= 0) {
        ch4.timer += NOISE_DIVISOR[ch4.divisor] << ch4.shift;
        const uint16_t bit = (ch4.lfsr ^ (ch4.lfsr >> 1)) & 1;
        ch4.lfsr = static_cast<uint16_t>((ch4.lfsr >> 1) | (bit << 14));
        if (ch4.width7)
          ch4.lfsr = static_cast<uint16_t>((ch4.lfsr & ~0x40) | (bit << 6));
      }

      // ---- the 512 Hz frame sequencer
      fsCounter += 2;
      if (fsCounter >= 8192) {
        fsCounter -= 8192;
        stepFrameSequencer();
      }

      // ---- what each channel is putting out right now (0-15)
      const int o1 = (ch1.enabled && ch1.dacOn && ((DUTY[ch1.duty] >> ch1.dutyPos) & 1))
                         ? ch1.volume : 0;
      const int o2 = (ch2.enabled && ch2.dacOn && ((DUTY[ch2.duty] >> ch2.dutyPos) & 1))
                         ? ch2.volume : 0;
      int o3 = 0;
      if (ch3.enabled && ch3.dacOn && ch3.level) {
        const uint8_t byte = ch3.ram[ch3.pos >> 1];
        const int nib = (ch3.pos & 1) ? (byte & 0x0F) : (byte >> 4);
        o3 = nib >> (ch3.level - 1); // 1 = 100%, 2 = 50%, 3 = 25%
      }
      const int o4 = (ch4.enabled && ch4.dacOn && !(ch4.lfsr & 1)) ? ch4.volume : 0;

      // ---- NR51: which channels reach which side. This is how Gen 1 mutes a channel.
      const uint8_t nr51 = regs[0x25 - 0x10];
      int l = 0, r = 0;
      if (nr51 & 0x10) l += o1;
      if (nr51 & 0x20) l += o2;
      if (nr51 & 0x40) l += o3;
      if (nr51 & 0x80) l += o4;
      if (nr51 & 0x01) r += o1;
      if (nr51 & 0x02) r += o2;
      if (nr51 & 0x04) r += o3;
      if (nr51 & 0x08) r += o4;

      accL += l;
      accR += r;
    }
    ++accN;

    // ---- box-averaged downsample to 48 kHz
    sampleAcc += SAMPLE_HZ * 2;
    if (sampleAcc >= CPU_HZ) {
      sampleAcc -= CPU_HZ;
      mixSample();
    }
  }
}

int GbApu::render(int16_t* out, int frames)
{
  const int have = std::min(frames, static_cast<int>(fifo.size() / 2));
  std::memcpy(out, fifo.data(), static_cast<size_t>(have) * 2 * sizeof(int16_t));
  fifo.erase(fifo.begin(), fifo.begin() + static_cast<long>(have) * 2);
  return have;
}

} // namespace pse::audio
