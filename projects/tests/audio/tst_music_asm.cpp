/*
  * Copyright 2026 Fairy Fox
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
 * @file
 * @brief **The `.asm` we ship must assemble to the bytes the cartridge has.** Nothing is on trust.
 *
 * The chain of custody, end to end:
 *
 * ```
 *   a real Red cartridge
 *        │   scripts/import_music.py --check   byte-diffs every command stream against it
 *        ▼
 *   bank02.bin / bank08.bin / bank1f.bin / waves.bin      ← the FIXTURES, in this test only
 *        │   THIS TEST demands they are byte-identical to...
 *        ▼
 *   Gen1MusicAsm::parse()  ←  the .asm we actually ship
 * ```
 *
 * So the fixtures are not "some blob we generated": they are the console's own bytes, proven against
 * the console, and this test proves our shipped sheet music assembles to exactly them.
 *
 * ⚠️ **The BYTE LAYOUT is the feature, not an implementation detail.** A track's id is *computed from
 * its header's address* (`id = (addr − SFX_Headers_N) / 3`), so one entry out of place shifts every
 * id after it — and 105 of our 151 tracks (the "inner voices") exist *only* because a misaligned id
 * lands mid-header. A parser that produced "the right notes" but the wrong addresses would silently
 * delete two thirds of the music and still sound perfect on the 46 tracks anybody checks.
 *
 * That is why this test compares **bytes**, not notes.
 */
#include <QtTest>
#include <QFile>

#include <pse-audio/gen1musicasm.h>

class TestMusicAsm : public QObject
{
  Q_OBJECT

private slots:
  void parses();
  void isByteIdenticalToTheCartridgesOwnBytes_data();
  void isByteIdenticalToTheCartridgesOwnBytes();
  void wavesMatch();
  void theHeaderTableIsWhereTheIdsComeFrom();

private:
  Gen1MusicAsm::Result r;

  static QByteArray fixture(const QString& name)
  {
    QFile f(QStringLiteral(PSE_MUSIC_FIXTURES) + QLatin1Char('/') + name);
    if (!f.open(QIODevice::ReadOnly))
      return QByteArray();

    return f.readAll();
  }
};

/// It reads the shipped `.asm` at all -- and says why, loudly, if it cannot.
void TestMusicAsm::parses()
{
  r = Gen1MusicAsm::parse(QStringLiteral(PSE_MUSIC_ASM));

  QVERIFY2(r.ok, qPrintable(r.error));

  // Each bank is the 256-entry header table ($300 bytes) plus every stream it reaches.
  QVERIFY(r.bank2.data.size() > 0x300);
  QVERIFY(r.bank8.data.size() > 0x300);
  QVERIFY(r.bank31.data.size() > 0x300);
}

void TestMusicAsm::isByteIdenticalToTheCartridgesOwnBytes_data()
{
  QTest::addColumn<QString>("file");
  QTest::addColumn<int>("bank");

  QTest::newRow("bank 2 (overworld)") << "bank02.bin" << 2;
  QTest::newRow("bank 8 (battle)")    << "bank08.bin" << 8;
  QTest::newRow("bank 31 (the rest)") << "bank1f.bin" << 31;
}

/// 🔴 THE ONE THAT MATTERS. Byte for byte, or the music is not the game's music.
void TestMusicAsm::isByteIdenticalToTheCartridgesOwnBytes()
{
  QFETCH(QString, file);
  QFETCH(int, bank);

  const Gen1MusicAsm::Result res = Gen1MusicAsm::parse(QStringLiteral(PSE_MUSIC_ASM));
  QVERIFY2(res.ok, qPrintable(res.error));

  const Gen1MusicAsm::Bank& b = (bank == 2)  ? res.bank2
                              : (bank == 8)  ? res.bank8
                                             : res.bank31;

  const QByteArray want = fixture(file);
  QVERIFY2(!want.isEmpty(), qPrintable(QStringLiteral("missing fixture %1").arg(file)));

  const QByteArray got(reinterpret_cast<const char*>(b.data.data()),
                       static_cast<qsizetype>(b.data.size()));

  QCOMPARE(got.size(), want.size());

  // Say WHERE, not just "they differ". A byte offset in a bank is an address, and an address is a
  // song -- so a bare "not equal" would leave the next person exactly where they started.
  for (qsizetype i = 0; i < want.size(); i++) {
    if (got.at(i) == want.at(i))
      continue;

    QFAIL(qPrintable(QStringLiteral("bank %1 differs at $%2: the .asm assembles to $%3, "
                                    "the cartridge has $%4")
                       .arg(bank)
                       .arg(Gen1MusicAsm::Base + i, 4, 16, QLatin1Char('0'))
                       .arg(quint8(got.at(i)), 2, 16, QLatin1Char('0'))
                       .arg(quint8(want.at(i)), 2, 16, QLatin1Char('0'))));
  }
}

/// The wave instruments -- including the `.wave5` **bug**, which is Lavender Town's instrument and
/// is different in every bank. A "fixed" wave5 is a wrong wave5.
void TestMusicAsm::wavesMatch()
{
  const Gen1MusicAsm::Result res = Gen1MusicAsm::parse(QStringLiteral(PSE_MUSIC_ASM));
  QVERIFY2(res.ok, qPrintable(res.error));

  const QByteArray want = fixture(QStringLiteral("waves.bin"));
  QVERIFY(!want.isEmpty());

  const QByteArray got(reinterpret_cast<const char*>(res.waves.data()),
                       static_cast<qsizetype>(res.waves.size()));

  QCOMPARE(got, want);
}

/// The header table is where the ids come from, so it has to be exactly where the game puts it: the
/// first `$300` bytes, id 0 being the `$ff $ff $ff` padding.
void TestMusicAsm::theHeaderTableIsWhereTheIdsComeFrom()
{
  const Gen1MusicAsm::Result res = Gen1MusicAsm::parse(QStringLiteral(PSE_MUSIC_ASM));
  QVERIFY2(res.ok, qPrintable(res.error));

  for (const Gen1MusicAsm::Bank* b : { &res.bank2, &res.bank8, &res.bank31 }) {
    // id 0 -- the padding the game leaves at the head of the table.
    QCOMPARE(int(b->data.at(0)), 0xFF);
    QCOMPARE(int(b->data.at(1)), 0xFF);
    QCOMPARE(int(b->data.at(2)), 0xFF);

    // The SFX headers come FIRST, and MAX_SFX_ID_N is the last of them. If this drifts, PlaySound
    // takes the wrong branch (partial vs full init) and the music comes in half-initialised.
    QVERIFY(b->maxSfxId > 0);
    QVERIFY(b->maxSfxId < 256);
  }

  // Pallet Town is bank 2, id 186 (music.json). It is 3 channels, so it eats ids 186/187/188 -- and
  // 187 is its bassline, alone. That is the whole "151 tracks for the price of 46" trick, and it is
  // a fact about the BYTE LAYOUT. Prove the header for 186 is where we think it is.
  const int at = 186 * 3;
  QVERIFY(at + 2 < int(res.bank2.data.size()));

  // The first channel of a header carries (channelCount - 1) in its top two bits.
  const int chans = (res.bank2.data.at(at) >> 6) + 1;
  QCOMPARE(chans, 3);
}

QTEST_MAIN(TestMusicAsm)
#include "tst_music_asm.moc"
