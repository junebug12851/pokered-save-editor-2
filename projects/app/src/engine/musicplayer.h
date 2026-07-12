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

#include <QIODevice>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <memory>

#include <pse-audio/gbapu.h>
#include <pse-audio/gen1soundengine.h>

class QAudioSink;

/**
 * @brief The thing that actually makes a noise: `brg.music`.
 *
 * It owns a @ref pse::audio::Gen1SoundEngine and a @ref pse::audio::GbApu, and hands the sound card
 * a QIODevice that *runs the game's engine on demand* -- a frame of the sequencer, a frame of the
 * chip, drain the samples, repeat. There is no decoding and no file: switching track costs one call
 * to the game's own `PlaySound`, which is why hover-preview can be instant.
 *
 * **Two levels of "what is playing", and the difference matters:**
 * - @ref selectedBank / @ref selectedId -- what the SAVE says. Changed only by @ref select().
 * - @ref playingBank / @ref playingId -- what you are HEARING. @ref preview() changes only this.
 *
 * So hovering auditions and clicking commits, and the UI can always tell you when the two differ.
 * Audio never starts on its own: something has to call @ref play().
 *
 * See notes/plans/music.md §6.
 */
class MusicPlayer : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playingChanged)     ///< Is any sound coming out?
  Q_PROPERTY(bool previewing READ previewing NOTIFY playingChanged)   ///< Is what you hear NOT what's saved?
  Q_PROPERTY(int playingBank READ playingBank NOTIFY playingChanged)  ///< Bank of the track being heard.
  Q_PROPERTY(int playingId READ playingId NOTIFY playingChanged)      ///< Id of the track being heard.
  Q_PROPERTY(int selectedBank READ selectedBank WRITE setSelectedBank NOTIFY selectedChanged) ///< Bank in the save.
  Q_PROPERTY(int selectedId READ selectedId WRITE setSelectedId NOTIFY selectedChanged)       ///< Id in the save.
  Q_PROPERTY(QString playingName READ playingName NOTIFY playingChanged) ///< Human name of what's heard.
  Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged) ///< 0..1.
  Q_PROPERTY(bool dataReady READ dataReady CONSTANT)                  ///< Did the music data load?

public:
  explicit MusicPlayer(QObject* parent = nullptr);
  ~MusicPlayer() override;

  /// A bank the real game can actually play. Anything else executes cartridge bytes as CODE and
  /// hangs the console (verified -- notes/reference/glitch-music.md), so the UI must never offer it.
  Q_INVOKABLE static bool isPlayableBank(int bank)
  {
    return bank == 2 || bank == 8 || bank == 31;
  }

  /// The name of a track id, including the "inner voices" -- e.g. 187 -> "Pallet Town (channel 2)".
  Q_INVOKABLE static QString describe(int bank, int id);

  /// The track list, for QML. MusicDBEntry is a plain struct (not a QObject/gadget), so QML cannot
  /// walk it -- these hand it over as plain values instead of QObject-ifying the database.
  Q_INVOKABLE static int trackCount();
  Q_INVOKABLE static QVariantMap track(int i);   ///< { name, bank, id }

  Q_INVOKABLE void play();                       ///< Start, on the SELECTED track. A deliberate act.
  Q_INVOKABLE void stop();                       ///< Silence. Nothing keeps humming behind a screen.
  Q_INVOKABLE void preview(int bank, int id);    ///< Audition a track. Does NOT touch the save.
  Q_INVOKABLE void unpreview();                  ///< Snap back to the selected track (the truth).
  Q_INVOKABLE void select(int bank, int id);     ///< Commit: this is now the save's track.

  [[nodiscard]] bool isPlaying() const { return playing; }
  [[nodiscard]] bool previewing() const;
  [[nodiscard]] int playingBank() const { return curBank; }
  [[nodiscard]] int playingId() const { return curId; }
  [[nodiscard]] int selectedBank() const { return selBank; }
  [[nodiscard]] int selectedId() const { return selId; }
  [[nodiscard]] QString playingName() const { return describe(curBank, curId); }
  [[nodiscard]] qreal volume() const { return vol; }
  [[nodiscard]] bool dataReady() const { return ready; }

  void setSelectedBank(int b);
  void setSelectedId(int i);
  void setVolume(qreal v);

signals:
  void playingChanged();
  void selectedChanged();
  void volumeChanged();
  /// Emitted when select() commits -- the save layer listens and writes the bytes.
  void trackSelected(int bank, int id);

private:
  friend class MusicDevice;

  void startTrack(int bank, int id);
  void openSink();
  void closeSink();

  /// Runs the engine + the chip to fill @p out. Called from the audio thread.
  qint64 generate(char* out, qint64 maxBytes);

  pse::audio::GbApu apu;
  pse::audio::Gen1SoundEngine engine{&apu};

  QMutex lock;                 ///< The UI thread changes the track; the audio thread reads it.
  std::unique_ptr<QAudioSink> sink;
  QIODevice* device = nullptr; ///< Owned by us; handed to the sink.

  bool ready = false;
  bool playing = false;
  int curBank = 0, curId = 0;
  int selBank = 0, selId = 0;
  qreal vol = 0.6;
};
