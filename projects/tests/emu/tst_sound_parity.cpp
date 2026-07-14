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
 * @file tst_sound_parity.cpp
 * @brief The sound engine, judged by the actual Game Boy.
 *
 * `tst_audio` asks "is our engine consistent with itself?" -- which a confidently wrong engine also
 * passes. This one asks the only question that settles it: **does the console agree, frame by
 * frame?**
 *
 * It can, because Gen 1's entire sound engine is **243 bytes at `$C000`** -- every command pointer,
 * note-delay counter, octave, duty, volume, vibrato phase and pitch-slide accumulator. So:
 *
 *   1. `scripts/emu/dump_sound_state.py` boots the real cartridge with a track patched into a save,
 *      waits until it is genuinely playing, and photographs `$C000-$C0FF` on **every frame**.
 *   2. We seed our C++ engine from the FIRST photograph -- the console's own state -- and then run
 *      it forward.
 *   3. Every subsequent frame must match. Byte for byte. If our engine is thinking the same thoughts
 *      as the console 240 frames running, it is writing the same registers, and the music is right.
 *
 * **The one translation.** The console's command pointers are addresses in the cartridge; ours are
 * addresses in the relocated data we import. So the pointers are compared *through* a ROM-address ->
 * our-address map, which `scripts/import_music.py` emits from the same lockstep walk that proves our
 * bytes match the cartridge. Everything else is compared raw.
 *
 * **This test does not need our engine to boot the game, fade music, or handle the front end** -- it
 * isolates exactly the thing we ported: `UpdateMusic`, one frame at a time.
 *
 * Local-only. Needs `assets/references/backup.gb` (gitignored, never committed, never shipped) and
 * the PyBoy venv. Without either, every case SKIPs and the suite stays green.
 *
 * @see notes/reference/gen1-sound-engine.md, notes/reference/emulator-verification.md
 */
#include <QtTest>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

#include <pse-audio/gbapu.h>
#include <pse-audio/gen1musicasm.h>
#include <pse-audio/gen1soundengine.h>
#include <pse-db/db.h>
#include <pse-db/music.h>

using namespace pse::audio;

namespace {

QString repoRoot()
{
  return QDir(QStringLiteral(PSE_ASSETS_DIR)).absoluteFilePath("..");
}

QByteArray res(const QString& p)
{
  QFile f(p);
  return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

/// ⚠️ THE SHIPPED SHEET MUSIC, assembled exactly the way the app assembles it.
///
/// This test used to read three `.bin` blobs from the qrc. Those are gone: we ship **pret/pokered's
/// `.asm`** now (@see Gen1MusicAsm). And it matters *here* more than anywhere, because this is the
/// test that puts our engine next to a **real cartridge**, frame by frame. If it fed itself from a
/// different source than the app does, it would be proving the wrong thing.
///
/// So the chain is: the `.asm` we ship -> this parser -> our engine -> compared against the console.
const Gen1MusicAsm::Result& sheet()
{
  static const Gen1MusicAsm::Result r =
    Gen1MusicAsm::parse(QStringLiteral(":/assets/data/music/pokered"));

  return r;
}

Gen1SoundEngine::Bank toBank(const Gen1MusicAsm::Bank& src)
{
  Gen1SoundEngine::Bank b;
  b.data = src.data;
  b.maxSfxId = src.maxSfxId;
  return b;
}

/// Run a python script from the repo's PyBoy venv. Returns its exit code (2 == "no ROM, skip").
int runPy(const QStringList& args, QString* err = nullptr)
{
  const QString py = QDir(repoRoot()).absoluteFilePath("tmp/emu-venv/Scripts/python.exe");
  if (!QFile::exists(py))
    return 2;

  QProcess p;
  p.setWorkingDirectory(repoRoot());
  p.start(py, args);
  if (!p.waitForFinished(180000))
    return 1;
  if (err)
    *err = QString::fromLocal8Bit(p.readAllStandardError());
  return p.exitCode();
}

} // namespace

class TstSoundParity : public QObject
{
  Q_OBJECT

  QHash<int, QHash<quint16, quint16>> ptrMap;   // bank -> (ROM address -> our address)
  bool haveRom = false;

private slots:

  void initTestCase()
  {
    DB::inst();   // pins db.dll (and with it the music resources)

    haveRom = QFile::exists(QDir(repoRoot()).absoluteFilePath("assets/references/backup.gb"));
    if (!haveRom)
      return;

    // The importer emits the ROM->our address map from the very same lockstep walk that proves our
    // imported bytes match the cartridge. Regenerate it so it can never be stale.
    const QString py = QDir(repoRoot()).absoluteFilePath("tmp/emu-venv/Scripts/python.exe");
    QProcess p;
    p.setWorkingDirectory(repoRoot());
    p.start(QFile::exists(py) ? py : QStringLiteral("python"),
            {QStringLiteral("scripts/import_music.py"), QStringLiteral("--check")});
    p.waitForFinished(120000);

    const QByteArray raw = res(QDir(repoRoot()).absoluteFilePath("tmp/music_ptrmap.json"));
    const QJsonObject root = QJsonDocument::fromJson(raw).object();
    for (auto bIt = root.begin(); bIt != root.end(); ++bIt) {
      const int bank = bIt.key().toInt();
      const QJsonObject m = bIt.value().toObject();
      QHash<quint16, quint16> h;
      for (auto it = m.begin(); it != m.end(); ++it)
        h.insert(static_cast<quint16>(it.key().toUInt()),
                 static_cast<quint16>(it.value().toInt()));
      ptrMap.insert(bank, h);
    }
  }

  void engineMatchesTheConsole_data()
  {
    QTest::addColumn<int>("bank");
    QTest::addColumn<int>("id");

    // EVERY track in the game -- not a flattering sample. Booting the emulator turns out to cost
    // well under a second per track, so there is no excuse for testing three of them.
    MusicDB* db = MusicDB::inst();
    for (int i = 0; i < db->getStoreSize(); ++i) {
      MusicDBEntry* e = db->getStoreAt(i);
      if (!e || e->id == 255)
        continue;   // 255 is SFX_STOP_ALL_MUSIC -- silence, and it never reaches the engine
      QTest::newRow(qPrintable(e->name)) << static_cast<int>(e->bank) << static_cast<int>(e->id);
    }

    // And an INNER VOICE: id 187 is not a track in any table -- it is Pallet Town's channel 2,
    // read as a header of its own. The console plays it as exactly that, and so must we.
    // (notes/reference/glitch-music.md)
    QTest::newRow("Pallet Town — channel 2 alone") << 2 << 187;
  }

  /**
   * The whole point. Seed our engine from the console's own 243 bytes, then demand it evolves
   * identically for 240 frames.
   */
  void engineMatchesTheConsole()
  {
    QFETCH(int, bank);
    QFETCH(int, id);

    if (!haveRom)
      QSKIP("no cartridge here -- this check is local-only (see emulator-verification.md)");

    // ---- 1. the console plays it, and we photograph its mind every frame
    QString err;
    const int rc = runPy({QStringLiteral("scripts/emu/dump_sound_state.py"),
                          QStringLiteral("--bank"), QString::number(bank),
                          QStringLiteral("--id"), QString::number(id),
                          QStringLiteral("--frames"), QStringLiteral("240")}, &err);
    if (rc == 2)
      QSKIP("no ROM / no PyBoy venv");
    QVERIFY2(rc == 0, qPrintable("the emulator could not play this track: " + err));

    const QString dir = QDir(repoRoot()).absoluteFilePath(
        QStringLiteral("tmp/sound/%1_%2").arg(bank).arg(id));
    const QByteArray state = res(dir + "/state.bin");
    const QJsonObject meta =
        QJsonDocument::fromJson(res(dir + "/meta.json")).object();
    const int frames = meta.value("frames").toInt();

    QCOMPARE(state.size(), frames * 0x100);
    QVERIFY(frames > 100);

    const auto console = [&](int f, int off) -> quint8 {
      return static_cast<quint8>(state[f * 0x100 + off]);
    };
    const auto consolePtr = [&](int f, int base, int c) -> quint16 {
      return static_cast<quint16>(console(f, base + c * 2) | (console(f, base + c * 2 + 1) << 8));
    };

    // ---- 2. seed our engine with the console's state, translating the command pointers
    GbApu apu;
    Gen1SoundEngine eng(&apu);
    QVERIFY2(sheet().ok, qPrintable(sheet().error));

    eng.setData(toBank(sheet().bank2),
                toBank(sheet().bank8),
                toBank(sheet().bank31),
                sheet().waves);
    eng.playMusic(static_cast<uint8_t>(bank), static_cast<uint8_t>(id));  // selects the bank

    const QHash<quint16, quint16>& m = ptrMap[bank];
    QVERIFY2(!m.isEmpty(), "no ROM->our address map -- did scripts/import_music.py run?");

    const auto translate = [&](quint16 romAddr, bool* ok) -> quint16 {
      if (romAddr == 0) { *ok = true; return 0; }
      const auto it = m.constFind(romAddr);
      *ok = (it != m.constEnd());
      return *ok ? it.value() : 0;
    };

    for (int off = 0; off < 0x100; ++off)
      eng.ram[off] = console(0, off);

    for (int c = 0; c < 8; ++c) {
      for (int base : {static_cast<int>(Gen1SoundEngine::W_CMD_PTRS),
                       static_cast<int>(Gen1SoundEngine::W_RET_ADDRS)}) {
        bool ok = false;
        const quint16 mine = translate(consolePtr(0, base, c), &ok);
        if (!ok)
          continue;   // an address outside the tracks we imported (a stale return slot): leave it
        eng.ram[base + c * 2] = mine & 0xFF;
        eng.ram[base + c * 2 + 1] = (mine >> 8) & 0xFF;
      }
    }

    // ---- 3. run forward, and compare EVERY frame
    int verified = 0;
    for (int f = 1; f < frames; ++f) {
      // The GAME can call PlaySound again mid-track (the Title Screen theme gets restarted on the
      // overworld after ~80 frames): every channel is re-pointed and every return address zeroed in
      // one go. That is the game, not UpdateMusic -- and UpdateMusic is the only thing this test is
      // entitled to judge. So we stop cleanly at that point and report how far we got.
      const bool restarted = console(f, Gen1SoundEngine::W_SOUND_ID) == id
                             && console(f - 1, Gen1SoundEngine::W_SOUND_ID) != id;
      if (restarted)
        break;

      eng.updateFrame();
      apu.tick(GbApu::CYCLES_PER_FRAME);
      ++verified;

      // The command pointers FIRST. If these diverge we are no longer even reading the same byte
      // of the same stream, and every other difference downstream is just a symptom of that -- so
      // this is the failure worth being told about.
      for (int c = 0; c < 8; ++c) {
        bool ok = false;
        const quint16 want = translate(consolePtr(f, Gen1SoundEngine::W_CMD_PTRS, c), &ok);
        if (!ok)
          continue;
        const quint16 got = static_cast<quint16>(
            eng.ram[Gen1SoundEngine::W_CMD_PTRS + c * 2]
            | (eng.ram[Gen1SoundEngine::W_CMD_PTRS + c * 2 + 1] << 8));
        QVERIFY2(want == got,
                 qPrintable(QString("frame %1, channel %2: the console is reading $%3, we are "
                                    "reading $%4 (track: bank %5 id %6)")
                                .arg(f).arg(c + 1)
                                .arg(want, 4, 16, QChar('0'))
                                .arg(got, 4, 16, QChar('0'))
                                .arg(bank).arg(id)));
      }

      QStringList diffs;
      for (int off = 0; off < 0x100; ++off) {
        const bool isPtr = (off >= Gen1SoundEngine::W_CMD_PTRS && off < Gen1SoundEngine::W_SOUND_IDS);
        if (isPtr)
          continue;   // compared above, through the address map
        if (off > Gen1SoundEngine::W_TEMPO_MOD)
          continue;   // past the engine's state (the section's tail padding)
        if (off == Gen1SoundEngine::W_AUDIO_ROM_BANK
            || off == Gen1SoundEngine::W_AUDIO_SAVED_BANK)
          continue;   // the console's bank NUMBER; we don't bank-switch, we index

        if (eng.ram[off] != console(f, off))
          diffs << QString("$C0%1 console=$%2 ours=$%3")
                       .arg(off, 2, 16, QChar('0'))
                       .arg(console(f, off), 2, 16, QChar('0'))
                       .arg(eng.ram[off], 2, 16, QChar('0'));
      }
      QVERIFY2(diffs.isEmpty(),
               qPrintable(QString("bank %1 id %2, frame %3 -- %4 byte(s) differ:\n    %5")
                              .arg(bank).arg(id).arg(f).arg(diffs.size())
                              .arg(diffs.join("\n    "))));
    }

    // A test that verified two frames would pass and mean nothing.
    QVERIFY2(verified >= 60,
             qPrintable(QString("only %1 frames were comparable for bank %2 id %3 -- too few to "
                                "prove anything").arg(verified).arg(bank).arg(id)));
  }
};

QTEST_MAIN(TstSoundParity)
#include "tst_sound_parity.moc"
