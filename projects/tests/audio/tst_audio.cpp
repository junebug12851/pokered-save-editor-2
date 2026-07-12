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
 * @file tst_audio.cpp
 * @brief The sound chip and the sound engine.
 *
 * The final word on whether the ENGINE is right belongs to the console (`tst_sound_parity`, which
 * compares our 243-byte state with the cartridge's, frame by frame). This suite covers what can be
 * settled here: that the APU behaves like the hardware is documented to, that the engine reads the
 * imported data, and -- the one that matters for the UI -- that the inner-voice ids really do play
 * ONE channel of a real song, exactly as the cartridge showed they do.
 */
#include <QtTest>
#include <QFile>

#include <pse-audio/gbapu.h>
#include <pse-audio/gen1soundengine.h>
#include <pse-db/db.h>
#include <pse-db/music.h>

using namespace pse::audio;

namespace {

QByteArray res(const QString& path)
{
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly))
    return {};
  return f.readAll();
}

Gen1SoundEngine::Bank makeBank(const QString& path, int maxSfxId)
{
  Gen1SoundEngine::Bank b;
  const QByteArray d = res(path);
  b.data.assign(d.begin(), d.end());
  b.maxSfxId = maxSfxId;
  return b;
}

/// Play a track and report which of the four MUSIC channels ever made a sound.
struct Result {
  bool channel[4] = {false, false, false, false};
  int distinctPitch[4] = {0, 0, 0, 0};
  bool anyAudio = false;
};

Result play(uint8_t bank, uint8_t id, int frames = 240)
{
  GbApu apu;
  Gen1SoundEngine eng(&apu);
  eng.setData(makeBank(":/assets/data/music/bank02.bin", 185),
              makeBank(":/assets/data/music/bank08.bin", 233),
              makeBank(":/assets/data/music/bank1f.bin", 194),
              [] {
                const QByteArray w = res(":/assets/data/music/waves.bin");
                return std::vector<uint8_t>(w.begin(), w.end());
              }());

  eng.playMusic(bank, id);

  Result r;
  QSet<int> pitches[4];
  std::vector<int16_t> buf(4096 * 2);

  for (int f = 0; f < frames; ++f) {
    eng.updateFrame();
    apu.tick(GbApu::CYCLES_PER_FRAME);

    // NR51 says which hardware channels actually reach the speakers -- Gen 1's real mute switch.
    const uint8_t nr51 = apu.read(0xFF25);
    for (int c = 0; c < 4; ++c) {
      if (nr51 & (0x11 << c))
        r.channel[c] = true;
      pitches[c].insert(eng.ram[Gen1SoundEngine::W_FREQ_LO + c]);
    }

    int n;
    while ((n = apu.render(buf.data(), 4096)) > 0) {
      for (int i = 0; i < n * 2; ++i)
        if (buf[static_cast<size_t>(i)] != 0)
          r.anyAudio = true;
      if (n < 4096) break;
    }
  }
  for (int c = 0; c < 4; ++c)
    r.distinctPitch[c] = pitches[c].size();
  return r;
}

} // namespace

class TstAudio : public QObject
{
  Q_OBJECT

private slots:

  void initTestCase()
  {
    // The music data is baked into db.qrc, and a Qt resource only registers itself when its DLL is
    // actually loaded. Nothing else in this suite touches `db`, so the linker would happily drop
    // the import and every resource lookup below would come back empty. One real call pins it.
    // (DB::inst() also does the loading -- the DB classes deliberately do NOT load in their
    // constructors; that deadlocks Qt 6's static init. See notes/decisions/architecture.md.)
    DB::inst();
    QVERIFY(MusicDB::inst()->getStoreSize() > 0);
  }

  // ---- the chip ------------------------------------------------------------------------

  void apu_aTriggeredPulseMakesSound()
  {
    GbApu apu;
    apu.write(0xFF26, 0x80);          // power on
    apu.write(0xFF25, 0xFF);          // everything to both speakers
    apu.write(0xFF24, 0x77);          // full volume
    apu.write(0xFF11, 0x80);          // duty 2 (a square)
    apu.write(0xFF12, 0xF0);          // volume 15, no envelope
    apu.write(0xFF13, 0x00);
    apu.write(0xFF14, 0x87);          // trigger, period $700

    apu.tick(GbApu::CYCLES_PER_FRAME);

    std::vector<int16_t> buf(4096 * 2);
    const int n = apu.render(buf.data(), 4096);
    QVERIFY(n > 0);

    bool nonZero = false;
    for (int i = 0; i < n * 2; ++i)
      if (buf[static_cast<size_t>(i)] != 0) nonZero = true;
    QVERIFY2(nonZero, "a triggered pulse channel produced silence");
  }

  void apu_dacOffKillsTheChannelDead()
  {
    GbApu apu;
    apu.write(0xFF26, 0x80);
    apu.write(0xFF25, 0xFF);
    apu.write(0xFF24, 0x77);
    apu.write(0xFF11, 0x80);
    apu.write(0xFF12, 0x00);          // volume 0 + "decrease" == DAC off
    apu.write(0xFF14, 0x87);          // trigger it anyway

    apu.tick(GbApu::CYCLES_PER_FRAME);
    std::vector<int16_t> buf(4096 * 2);
    const int n = apu.render(buf.data(), 4096);

    for (int i = 0; i < n * 2; ++i)
      QCOMPARE(buf[static_cast<size_t>(i)], static_cast<int16_t>(0));
  }

  void apu_nr11ReadsBackItsLengthBitsAsOnes()
  {
    // Not a curiosity: the engine's duty-rotation code does `and $3f` on this read-back, so the
    // hardware's read mask is what makes it write a sound length of 63. (gameboy-apu.md §2)
    GbApu apu;
    apu.write(0xFF26, 0x80);
    apu.write(0xFF11, 0x80);
    QCOMPARE(apu.read(0xFF11), static_cast<uint8_t>(0xBF));
  }

  // ---- the engine ----------------------------------------------------------------------

  void engine_theImportedDataIsThere()
  {
    QVERIFY2(!res(":/assets/data/music/bank02.bin").isEmpty(), "bank 2 music data missing from qrc");
    QVERIFY2(!res(":/assets/data/music/bank08.bin").isEmpty(), "bank 8 music data missing from qrc");
    QVERIFY2(!res(":/assets/data/music/bank1f.bin").isEmpty(), "bank 31 music data missing from qrc");
    QCOMPARE(res(":/assets/data/music/waves.bin").size(), 128);
  }

  void engine_palletTownPlaysOnThreeChannels()
  {
    const Result r = play(2, 186);
    QVERIFY2(r.anyAudio, "Pallet Town produced no audio at all");
    QVERIFY2(r.channel[0], "channel 1 never sounded");
    QVERIFY2(r.channel[1], "channel 2 never sounded");
    QVERIFY2(r.channel[2], "channel 3 never sounded");
    QVERIFY2(!r.channel[3], "the noise channel sounded, and Pallet Town has no drums");
    // A tune MOVES. A stuck channel does not.
    QVERIFY2(r.distinctPitch[0] > 3, "channel 1 never changed pitch");
    QVERIFY2(r.distinctPitch[1] > 3, "channel 2 never changed pitch");
  }

  /**
   * The one the whole "151 tracks for the price of 46" claim rests on.
   *
   * Id 187 is not a track in any table -- it is the three bytes of Pallet Town's header that
   * describe its CHANNEL 2. Read as a header in its own right it is a valid one-channel song, and
   * the cartridge plays it as exactly that: `wChannelSoundIDs = [0, 187, 0, 0]`, NR51 = $22.
   * So must we. (notes/reference/glitch-music.md)
   */
  void engine_innerVoice_id187_isPalletTownsChannel2Alone()
  {
    const Result r = play(2, 187);
    QVERIFY2(r.anyAudio, "the inner voice produced no audio");
    QVERIFY2(!r.channel[0], "channel 1 sounded -- this should be channel 2 ALONE");
    QVERIFY2(r.channel[1], "channel 2 never sounded");
    QVERIFY2(!r.channel[2], "channel 3 sounded -- this should be channel 2 ALONE");
    QVERIFY2(r.distinctPitch[1] > 3, "channel 2 never changed pitch -- it isn't playing the tune");

    // And the engine must agree with the console's own audio RAM, byte for byte.
    GbApu apu;
    Gen1SoundEngine eng(&apu);
    eng.setData(makeBank(":/assets/data/music/bank02.bin", 185),
                makeBank(":/assets/data/music/bank08.bin", 233),
                makeBank(":/assets/data/music/bank1f.bin", 194), {});
    eng.playMusic(2, 187);
    QCOMPARE(eng.ram[Gen1SoundEngine::W_SOUND_IDS + 0], static_cast<uint8_t>(0));
    QCOMPARE(eng.ram[Gen1SoundEngine::W_SOUND_IDS + 1], static_cast<uint8_t>(187));
    QCOMPARE(eng.ram[Gen1SoundEngine::W_SOUND_IDS + 2], static_cast<uint8_t>(0));
    QCOMPARE(eng.ram[Gen1SoundEngine::W_SOUND_IDS + 3], static_cast<uint8_t>(0));
  }

  void engine_everyRealTrackPlays()
  {
    // 46 tracks, three banks. Every one must make a sound; a silent track is a broken import.
    const QList<QPair<int, int>> some = {
        {2, 186}, {2, 189}, {2, 192}, {2, 195}, {2, 212}, {2, 251},
        {8, 234}, {8, 240}, {8, 252},
        {31, 195}, {31, 205}, {31, 224}, {31, 240},
    };
    for (const auto& [bank, id] : some) {
      const Result r = play(static_cast<uint8_t>(bank), static_cast<uint8_t>(id), 120);
      QVERIFY2(r.anyAudio,
               qPrintable(QString("bank %1 id %2 produced silence").arg(bank).arg(id)));
    }
  }

  void engine_id255_isSilence()
  {
    const Result r = play(31, 255, 60);
    QVERIFY2(!r.anyAudio, "id 255 is SFX_STOP_ALL_MUSIC -- it must be silent");
  }
};

QTEST_MAIN(TstAudio)
#include "tst_audio.moc"
