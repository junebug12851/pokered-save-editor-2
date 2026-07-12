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
 * @file musicplayer.cpp
 * @brief Implementation of MusicPlayer -- see musicplayer.h.
 */
#include "./musicplayer.h"

#include <QAudioFormat>
#include <QAudioSink>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaDevices>
#include <QMutexLocker>

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
  // The three audio banks, exactly as the cartridge lays them out (the header table at $4000 and
  // every stream it can reach). maxSfxId comes from the importer -- PlaySound branches on it.
  const QByteArray idxRaw = readRes(":/assets/data/music/index.json");
  int max2 = 185, max8 = 233, max31 = 194;   // the values the importer computed
  if (!idxRaw.isEmpty()) {
    const QJsonObject banks = QJsonDocument::fromJson(idxRaw).object().value("banks").toObject();
    max2 = banks.value("2").toObject().value("maxSfxId").toInt(max2);
    max8 = banks.value("8").toObject().value("maxSfxId").toInt(max8);
    max31 = banks.value("31").toObject().value("maxSfxId").toInt(max31);
  }

  const QByteArray w = readRes(":/assets/data/music/waves.bin");
  Gen1SoundEngine::Bank b2 = loadBank(":/assets/data/music/bank02.bin", max2);
  Gen1SoundEngine::Bank b8 = loadBank(":/assets/data/music/bank08.bin", max8);
  Gen1SoundEngine::Bank b31 = loadBank(":/assets/data/music/bank1f.bin", max31);

  ready = !b2.data.empty() && !b8.data.empty() && !b31.data.empty() && !w.isEmpty();
  engine.setData(b2, b8, b31, std::vector<uint8_t>(w.begin(), w.end()));
}

MusicPlayer::~MusicPlayer()
{
  closeSink();
}

QString MusicPlayer::describe(int bank, int id)
{
  if (id == 255)
    return tr("No music");

  MusicDB* db = MusicDB::inst();
  const int n = db->getStoreSize();

  // An exact hit is a real track.
  for (int i = 0; i < n; ++i) {
    MusicDBEntry* e = db->getStoreAt(i);
    if (e && e->bank == bank && e->id == id)
      return e->name;
  }

  // Otherwise: it is one of the INNER VOICES -- a real song's channel, played alone, because a
  // header is 3 bytes per channel and so a 3-channel song eats 3 ids. Name it honestly.
  // (notes/reference/glitch-music.md)
  int bestId = -1;
  QString bestName;
  for (int i = 0; i < n; ++i) {
    MusicDBEntry* e = db->getStoreAt(i);
    if (!e || e->bank != bank || e->id > id || e->id == 255)
      continue;
    if (e->id > bestId) {
      bestId = e->id;
      bestName = e->name;
    }
  }
  if (bestId >= 0 && id - bestId >= 1 && id - bestId <= 3)
    return tr("%1 — channel %2 alone").arg(bestName).arg(id - bestId + 1);

  return tr("Unknown — id %1, bank %2").arg(id).arg(bank);
}

int MusicPlayer::trackCount()
{
  return MusicDB::inst()->getStoreSize();
}

QVariantMap MusicPlayer::track(int i)
{
  MusicDBEntry* e = MusicDB::inst()->getStoreAt(i);
  if (!e)
    return {};
  return QVariantMap{{"name", e->name}, {"bank", static_cast<int>(e->bank)},
                     {"id", static_cast<int>(e->id)}};
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
