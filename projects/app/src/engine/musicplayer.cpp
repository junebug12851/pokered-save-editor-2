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
 * @file musicplayer.cpp
 * @brief Implementation of MusicPlayer -- see musicplayer.h.
 */
#include "./musicplayer.h"

#include <QAudioFormat>
#include <QAudioSink>
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaDevices>
#include <QMutexLocker>

#include <pse-audio/gen1musicasm.h>
#include <pse-db/music.h>

using namespace pse::audio;

namespace {

QByteArray readRes(const QString& path)
{
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly))
    return {};
  return f.readAll();
}

Gen1SoundEngine::Bank loadBank(const QString& path, int maxSfxId)
{
  Gen1SoundEngine::Bank b;
  const QByteArray d = readRes(path);
  b.data.assign(d.begin(), d.end());
  b.maxSfxId = maxSfxId;
  return b;
}

/**
 * The music.json key -> what a human should read, and where it belongs in the list.
 *
 * The keys are internal ids and stay untouched; this is display text only -- which is why Poké and
 * Pokémon are spelled properly here and nowhere near the data.
 */
struct TrackInfo {
  const char* key;
  const char* name;
  const char* group;
};

const TrackInfo TRACKS[] = {
    // Towns & Cities
    {"PalletTown",        QT_TRANSLATE_NOOP("MusicPlayer", "Pallet Town"),           "Towns & Cities"},
    {"Cities1",           QT_TRANSLATE_NOOP("MusicPlayer", "Cities 1"),              "Towns & Cities"},
    {"Cities2",           QT_TRANSLATE_NOOP("MusicPlayer", "Cities 2"),              "Towns & Cities"},
    {"Celadon",           QT_TRANSLATE_NOOP("MusicPlayer", "Celadon City"),          "Towns & Cities"},
    {"Cinnabar",          QT_TRANSLATE_NOOP("MusicPlayer", "Cinnabar Island"),       "Towns & Cities"},
    {"Vermilion",         QT_TRANSLATE_NOOP("MusicPlayer", "Vermilion City"),        "Towns & Cities"},
    {"Lavender",          QT_TRANSLATE_NOOP("MusicPlayer", "Lavender Town"),         "Towns & Cities"},
    {"IndigoPlateau",     QT_TRANSLATE_NOOP("MusicPlayer", "Indigo Plateau"),        "Towns & Cities"},

    // Routes
    {"Routes1",           QT_TRANSLATE_NOOP("MusicPlayer", "Routes 1"),              "Routes"},
    {"Routes2",           QT_TRANSLATE_NOOP("MusicPlayer", "Routes 2"),              "Routes"},
    {"Routes3",           QT_TRANSLATE_NOOP("MusicPlayer", "Routes 3"),              "Routes"},
    {"Routes4",           QT_TRANSLATE_NOOP("MusicPlayer", "Routes 4"),              "Routes"},
    {"BikeRiding",        QT_TRANSLATE_NOOP("MusicPlayer", "Bike Riding"),           "Routes"},
    {"Surfing",           QT_TRANSLATE_NOOP("MusicPlayer", "Surfing"),               "Routes"},

    // Places
    {"Pokecenter",        QT_TRANSLATE_NOOP("MusicPlayer", "Poké Center"),           "Places"},
    {"Gym",               QT_TRANSLATE_NOOP("MusicPlayer", "Gym"),                   "Places"},
    {"OaksLab",           QT_TRANSLATE_NOOP("MusicPlayer", "Oak's Lab"),             "Places"},
    {"SSAnne",            QT_TRANSLATE_NOOP("MusicPlayer", "S.S. Anne"),             "Places"},
    {"SafariZone",        QT_TRANSLATE_NOOP("MusicPlayer", "Safari Zone"),           "Places"},
    {"GameCorner",        QT_TRANSLATE_NOOP("MusicPlayer", "Game Corner"),           "Places"},
    {"CinnabarMansion",   QT_TRANSLATE_NOOP("MusicPlayer", "Pokémon Mansion"),       "Places"},
    {"PokemonTower",      QT_TRANSLATE_NOOP("MusicPlayer", "Pokémon Tower"),         "Places"},
    {"SilphCo",           QT_TRANSLATE_NOOP("MusicPlayer", "Silph Co."),             "Places"},
    {"Dungeon1",          QT_TRANSLATE_NOOP("MusicPlayer", "Dungeon 1"),             "Places"},
    {"Dungeon2",          QT_TRANSLATE_NOOP("MusicPlayer", "Dungeon 2"),             "Places"},
    {"Dungeon3",          QT_TRANSLATE_NOOP("MusicPlayer", "Dungeon 3"),             "Places"},
    {"HallOfFame",        QT_TRANSLATE_NOOP("MusicPlayer", "Hall of Fame"),          "Places"},

    // Battle
    {"WildBattle",        QT_TRANSLATE_NOOP("MusicPlayer", "Wild Battle"),           "Battle"},
    {"TrainerBattle",     QT_TRANSLATE_NOOP("MusicPlayer", "Trainer Battle"),        "Battle"},
    {"GymLeaderBattle",   QT_TRANSLATE_NOOP("MusicPlayer", "Gym Leader Battle"),     "Battle"},
    {"FinalBattle",       QT_TRANSLATE_NOOP("MusicPlayer", "Final Battle"),          "Battle"},
    {"IntroBattle",       QT_TRANSLATE_NOOP("MusicPlayer", "Intro Battle"),          "Battle"},
    {"DefeatedWildMon",   QT_TRANSLATE_NOOP("MusicPlayer", "Defeated a Wild Pokémon"), "Battle"},
    {"DefeatedTrainer",   QT_TRANSLATE_NOOP("MusicPlayer", "Defeated a Trainer"),    "Battle"},
    {"DefeatedGymLeader", QT_TRANSLATE_NOOP("MusicPlayer", "Defeated a Gym Leader"), "Battle"},

    // Encounters
    {"MeetProfOak",       QT_TRANSLATE_NOOP("MusicPlayer", "Meet Professor Oak"),    "Encounters"},
    {"MeetRival",         QT_TRANSLATE_NOOP("MusicPlayer", "Meet Rival"),            "Encounters"},
    {"MeetEvilTrainer",   QT_TRANSLATE_NOOP("MusicPlayer", "Meet Evil Trainer"),     "Encounters"},
    {"MeetFemaleTrainer", QT_TRANSLATE_NOOP("MusicPlayer", "Meet Female Trainer"),   "Encounters"},
    {"MeetMaleTrainer",   QT_TRANSLATE_NOOP("MusicPlayer", "Meet Male Trainer"),     "Encounters"},
    {"MuseumGuy",         QT_TRANSLATE_NOOP("MusicPlayer", "Follow Me"),             "Encounters"},

    // Special
    {"TitleScreen",       QT_TRANSLATE_NOOP("MusicPlayer", "Title Screen"),          "Special"},
    {"Credits",           QT_TRANSLATE_NOOP("MusicPlayer", "Credits"),               "Special"},
    {"JigglypuffSong",    QT_TRANSLATE_NOOP("MusicPlayer", "Jigglypuff's Song"),     "Special"},
    {"PkmnHealed",        QT_TRANSLATE_NOOP("MusicPlayer", "Pokémon Healed"),        "Special"},
    {"None",              QT_TRANSLATE_NOOP("MusicPlayer", "No music"),              "Special"},
};

const TrackInfo* infoFor(const QString& key)
{
  for (const TrackInfo& t : TRACKS)
    if (key == QLatin1String(t.key))
      return &t;
  return nullptr;
}

} // namespace

/**
 * @brief The QIODevice the sound card pulls on.
 *
 * Qt asks for bytes; we answer by running the Game Boy. Nothing is buffered ahead beyond what the
 * sink asks for, so a track change is heard within one engine frame (~17 ms) -- which is what makes
 * hover-preview feel like a cut rather than a load.
 */
class MusicDevice : public QIODevice
{
public:
  explicit MusicDevice(MusicPlayer* p) : player(p) {}

  qint64 readData(char* data, qint64 maxSize) override
  {
    return player->generate(data, maxSize);
  }
  qint64 writeData(const char*, qint64) override { return 0; }
  [[nodiscard]] qint64 bytesAvailable() const override
  {
    return 4096 + QIODevice::bytesAvailable();
  }
  [[nodiscard]] bool isSequential() const override { return true; }

private:
  MusicPlayer* player;
};

MusicPlayer::MusicPlayer(QObject* parent) : QObject(parent)
{
  // ⚠️ NOTHING IS PARSED HERE. @see ensureLoaded.
}

void MusicPlayer::ensureLoaded()
{
  if (loaded)
    return;

  loaded = true;

  // 🔴 THE GAME'S OWN SHEET MUSIC, READ AS TEXT.
  //
  // We ship `pret/pokered`'s **`.asm`** -- their files, their macros, their words -- and assemble it
  // here. We used to ship three `.bin` blobs in a container **we invented**, built by a script from
  // a clone that wasn't even in the repository. You could not read them, you could not review them,
  // and you could not regenerate them. (The standing rule, and project leadership's, 2026-07-13: *"where
  // pret/pokered has a format, WE USE THAT FORMAT."*)
  //
  // ⚠️ It still comes out as BYTES, and that is not laziness -- **the byte layout IS the feature.**
  // A track's id is *computed from its header's address*, so 105 of our 151 tracks (the inner voices)
  // exist only because a misaligned id lands mid-header. A tidy list of "commands" has nothing to
  // misalign, and would silently delete two thirds of the music. @see Gen1MusicAsm.
  //
  // ⚠️ And it is done HERE, on first use -- not in the constructor. It costs ~85 ms, and the ▶ is a
  // thing you press: nobody who never touches the music should pay for it at boot.
  const Gen1MusicAsm::Result m = Gen1MusicAsm::parse(QStringLiteral(":/assets/data/music/pokered"));

  // ⚠️ THE NOTIFY IS QUEUED, AND IT HAS TO BE.
  //
  // `tracks` and `dataReady` are Q_PROPERTYs that NOTIFY on `dataReadyChanged` -- and **reading them
  // is what triggers this load**. So emitting the signal synchronously, from inside the getter, tells
  // QML "the value you are in the middle of reading has changed", and QML calls it what it is: a
  // **binding loop**. Queued, it lands after the current evaluation finishes, and the binding simply
  // re-runs once with the loaded data.
  auto notifyReady = [this] {
    QMetaObject::invokeMethod(this, [this] { emit dataReadyChanged(); }, Qt::QueuedConnection);
  };

  if (!m.ok) {
    qWarning().noquote() << "[music] the sheet music did not assemble:" << m.error;
    ready = false;
    notifyReady();
    return;
  }

  Gen1SoundEngine::Bank b2;
  b2.data = m.bank2.data;
  b2.maxSfxId = m.bank2.maxSfxId;

  Gen1SoundEngine::Bank b8;
  b8.data = m.bank8.data;
  b8.maxSfxId = m.bank8.maxSfxId;

  Gen1SoundEngine::Bank b31;
  b31.data = m.bank31.data;
  b31.maxSfxId = m.bank31.maxSfxId;

  ready = !b2.data.empty() && !b8.data.empty() && !b31.data.empty() && !m.waves.empty();

  banks[0] = b2;
  banks[1] = b8;
  banks[2] = b31;

  engine.setData(b2, b8, b31, m.waves);

  buildTrackList();

  notifyReady();
}

MusicPlayer::~MusicPlayer()
{
  closeSink();
}

QString MusicPlayer::describe(int bank, int id)
{
  ensureLoaded();

  // One source of truth: the same list the panel shows, so a name can never disagree with itself.
  for (const QVariant& v : entries) {
    const QVariantMap m = v.toMap();
    if (m.value("bank").toInt() == bank && m.value("id").toInt() == id)
      return m.value("name").toString();
  }

  // A save can hold an id that is not any track and not any of their inner voices. We say so
  // plainly rather than rounding it to something that looks tidy -- the save is the truth.
  return tr("Unknown — id %1, bank %2").arg(id).arg(bank);
}

int MusicPlayer::channelCount(int bank, int id) const
{
  // Straight out of the header table we imported, at exactly the address the game reads:
  // SFX_Headers_N + id * 3, top two bits of the first byte.
  const int idx = bank == 2 ? 0 : (bank == 8 ? 1 : 2);
  const std::vector<uint8_t>& d = banks[idx].data;
  const size_t off = static_cast<size_t>(id) * 3;
  if (off >= d.size())
    return 1;
  return ((d[off] & 0xC0) >> 6) + 1;
}

void MusicPlayer::buildTrackList()
{
  entries.clear();
  MusicDB* db = MusicDB::inst();

  QString lastGroup;

  // ── SILENCE, FIRST ────────────────────────────────────────────────────────────────────────
  //
  // "No music" is a real value a map can hold and it is the one you reach for most often -- so it
  // goes at the TOP, not buried at the bottom of "Special" where it was (project leadership, 2026-07-13). It
  // has no group heading: it is not part of a group, it is the absence of one.
  for (int i = 0; i < db->getStoreSize(); ++i) {
    MusicDBEntry* e = db->getStoreAt(i);
    if (e == nullptr || e->name != QLatin1String("None"))
      continue;

    QVariantMap none;
    none["name"]    = QCoreApplication::translate("MusicPlayer", "No music");
    none["group"]   = QString();
    none["header"]  = QString();
    none["bank"]    = e->bank;
    none["id"]      = e->id;
    none["inner"]   = false;
    none["channel"] = 0;
    entries.append(none);
    break;
  }

  // Walk the groups in display order, and inside each, the tracks in the order they're listed.
  const QStringList order = {QStringLiteral("Towns & Cities"), QStringLiteral("Routes"),
                             QStringLiteral("Places"), QStringLiteral("Battle"),
                             QStringLiteral("Encounters"), QStringLiteral("Special")};

  for (const QString& group : order) {
    for (const TrackInfo& t : TRACKS) {
      if (group != QLatin1String(t.group))
        continue;

      // ...and NOT a second time, down in "Special". @see above.
      if (QLatin1String(t.key) == QLatin1String("None"))
        continue;

      // Find it in the DB (which owns the real bank/id).
      MusicDBEntry* e = nullptr;
      for (int i = 0; i < db->getStoreSize(); ++i) {
        MusicDBEntry* c = db->getStoreAt(i);
        if (c && c->name == QLatin1String(t.key)) {
          e = c;
          break;
        }
      }
      if (!e)
        continue;

      const int bank = e->bank;
      const int id = e->id;
      const QString name = QCoreApplication::translate("MusicPlayer", t.name);

      QVariantMap row;
      row["name"] = name;
      row["group"] = group;
      row["header"] = (group == lastGroup) ? QString() : group;
      row["bank"] = bank;
      row["id"] = id;
      row["inner"] = false;
      row["channel"] = 0;
      entries.append(row);
      lastGroup = group;

      // ---- and its INNER VOICES.
      //
      // A header is 3 bytes per channel and a track's id IS its header's address / 3 -- so a
      // 3-channel song occupies three consecutive ids, and the spare two are that song's channel 2
      // and channel 3, read as one-channel headers in their own right. The console plays them as
      // exactly that. They are real, they are reachable in a real save, and they cost us nothing:
      // 105 more pieces of music from the data we already have.
      if (id == 255)
        continue;   // "No music" is SFX_STOP_ALL_MUSIC -- it has no channels to take apart
      const int chans = channelCount(bank, id);
      for (int k = 1; k < chans; ++k) {
        QVariantMap voice;
        voice["name"] = tr("%1 — channel %2 alone").arg(name).arg(k + 1);
        voice["group"] = group;
        voice["header"] = QString();
        voice["bank"] = bank;
        voice["id"] = id + k;
        voice["inner"] = true;
        voice["channel"] = k + 1;
        entries.append(voice);
      }
    }
  }
}

bool MusicPlayer::previewing() const
{
  return playing && (curBank != selBank || curId != selId);
}

void MusicPlayer::setSelectedBank(int b)
{
  if (selBank == b) return;
  selBank = b;
  emit selectedChanged();
  emit playingChanged();
}

void MusicPlayer::setSelectedId(int i)
{
  if (selId == i) return;
  selId = i;
  emit selectedChanged();
  emit playingChanged();
}

void MusicPlayer::setVolume(qreal v)
{
  vol = qBound(0.0, v, 1.0);
  if (sink) sink->setVolume(static_cast<float>(vol));
  emit volumeChanged();
}

void MusicPlayer::startTrack(int bank, int id)
{
  QMutexLocker guard(&lock);
  curBank = bank;
  curId = id;
  apu.reset();
  apu.clearAudio();
  if (isPlayableBank(bank))
    engine.playMusic(static_cast<uint8_t>(bank), static_cast<uint8_t>(id));
}

void MusicPlayer::play()
{
  ensureLoaded();   // the ▶ is the LAST moment we could still get away with not having the music

  if (!ready || !isPlayableBank(selBank))
    return;
  startTrack(selBank, selId);
  openSink();
  playing = true;
  emit playingChanged();
}

void MusicPlayer::stop()
{
  closeSink();
  playing = false;
  emit playingChanged();
}

void MusicPlayer::preview(int bank, int id)
{
  if (!playing || !ready || !isPlayableBank(bank))
    return;   // hovering NEVER starts audio -- see notes/plans/music.md §6.3
  if (bank == curBank && id == curId)
    return;
  startTrack(bank, id);
  emit playingChanged();
}

void MusicPlayer::unpreview()
{
  if (!playing) return;
  if (curBank == selBank && curId == selId) return;
  startTrack(selBank, selId);
  emit playingChanged();
}

void MusicPlayer::select(int bank, int id)
{
  ensureLoaded();

  selBank = bank;
  selId = id;
  emit selectedChanged();
  emit trackSelected(bank, id);
  if (playing && (curBank != bank || curId != id))
    startTrack(bank, id);
  emit playingChanged();
}

void MusicPlayer::openSink()
{
  if (sink)
    return;

  QAudioFormat fmt;
  fmt.setSampleRate(GbApu::SAMPLE_HZ);
  fmt.setChannelCount(2);
  fmt.setSampleFormat(QAudioFormat::Int16);

  const QAudioDevice out = QMediaDevices::defaultAudioOutput();
  if (out.isNull())
    return;

  sink = std::make_unique<QAudioSink>(out, fmt);
  sink->setVolume(static_cast<float>(vol));

  device = new MusicDevice(this);
  device->open(QIODevice::ReadOnly);
  sink->start(device);
}

void MusicPlayer::closeSink()
{
  if (sink) {
    sink->stop();
    sink.reset();
  }
  if (device) {
    device->close();
    delete device;
    device = nullptr;
  }
}

qint64 MusicPlayer::generate(char* out, qint64 maxBytes)
{
  QMutexLocker guard(&lock);

  const qint64 wantFrames = maxBytes / 4;   // 2 channels x int16
  if (wantFrames <= 0)
    return 0;

  auto* pcm = reinterpret_cast<int16_t*>(out);
  qint64 done = 0;

  while (done < wantFrames) {
    // Run the Game Boy until there is something to hand over. One engine frame = one VBlank; the
    // chip then advances by exactly one LCD frame's worth of cycles. That is the real thing, at the
    // real rate -- no resampling of a recording, because there is no recording.
    if (apu.available() == 0) {
      engine.updateFrame();
      apu.tick(GbApu::CYCLES_PER_FRAME);
    }
    const int n = apu.render(pcm + done * 2, static_cast<int>(wantFrames - done));
    if (n <= 0)
      break;
    done += n;
  }

  return done * 4;
}
